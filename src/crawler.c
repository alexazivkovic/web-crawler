#include "crawler.h"
#include "fetcher.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int crawler_init(crawler_t* crawler, const crawler_config_t* config)
{
    memset(crawler, 0, sizeof(*crawler));
    crawler->config = *config;

    if (url_queue_init(&crawler->queue) != 0) {
        return -1;
    }
    if (visited_set_init(&crawler->visited, 4096) != 0) {
        url_queue_destroy(&crawler->queue);
        return -1;
    }
    if (pthread_mutex_init(&crawler->stats.mutex, NULL) != 0) {
        url_queue_destroy(&crawler->queue);
        visited_set_destroy(&crawler->visited);
        return -1;
    }
    if (pthread_mutex_init(&crawler->print_mutex, NULL) != 0) {
        url_queue_destroy(&crawler->queue);
        visited_set_destroy(&crawler->visited);
        pthread_mutex_destroy(&crawler->stats.mutex);
        return -1;
    }

    crawler->stats.max_pages = config->max_pages;
    return 0;
}

void crawler_destroy(crawler_t* crawler)
{
    url_queue_destroy(&crawler->queue);
    visited_set_destroy(&crawler->visited);
    pthread_mutex_destroy(&crawler->stats.mutex);
    pthread_mutex_destroy(&crawler->print_mutex);
}

static void* worker_thread(void* arg)
{
    crawler_t* crawler = (crawler_t*)arg;

    for (;;) {
        char* url = NULL;
        int depth = 0;

        if (!url_queue_pop(&crawler->queue, &url, &depth)) {
            break; /* nema više posla - graceful shutdown */
        }

        pthread_mutex_lock(&crawler->stats.mutex);
        int already_over_limit = crawler->stats.limit_reached;
        pthread_mutex_unlock(&crawler->stats.mutex);

        if (already_over_limit) {
            free(url);
            url_queue_task_done(&crawler->queue);
            continue;
        }

        fetch_result_t result;
        int ok = fetcher_get(url, crawler->config.fetch_timeout_seconds, &result) == 0;

        int just_hit_limit = 0;

        pthread_mutex_lock(&crawler->stats.mutex);
        if (ok) {
            crawler->stats.pages_fetched++;
        } else {
            crawler->stats.pages_failed++;
        }
        if (!crawler->stats.limit_reached && crawler->stats.max_pages > 0
            && crawler->stats.pages_fetched >= crawler->stats.max_pages) {
            crawler->stats.limit_reached = 1;
            just_hit_limit = 1;
        }
        int limit_reached_now = crawler->stats.limit_reached;
        pthread_mutex_unlock(&crawler->stats.mutex);

        if (crawler->config.verbose) {
            pthread_mutex_lock(&crawler->print_mutex);
            if (ok) {
                printf("[depth=%d] [%ld] %s\n", depth, result.http_status, url);
            } else {
                printf("[depth=%d] [FAIL] %s\n", depth, url);
            }
            pthread_mutex_unlock(&crawler->print_mutex);
        }

        if (ok && !limit_reached_now && depth < crawler->config.max_depth) {
            char** links = NULL;
            size_t link_count = 0;

            if (parser_extract_links(result.body, url, &links, &link_count) == 0) {
                size_t new_links = 0;
                for (size_t i = 0; i < link_count; i++) {
                    if (visited_set_try_add(&crawler->visited, links[i])) {
                        url_queue_push(&crawler->queue, links[i], depth + 1);
                        new_links++;
                    }
                }
                if (new_links > 0) {
                    pthread_mutex_lock(&crawler->stats.mutex);
                    crawler->stats.links_discovered += new_links;
                    pthread_mutex_unlock(&crawler->stats.mutex);
                }
                parser_free_links(links, link_count);
            }
        }

        if (ok) {
            fetch_result_free(&result);
        }
        free(url);
        url_queue_task_done(&crawler->queue);

        if (just_hit_limit) {
            /* Probudi sve worker-e odmah, ne čekaj da se red isprazni prirodno. */
            url_queue_shutdown(&crawler->queue);
        }
    }

    return NULL;
}

int crawler_run(crawler_t* crawler, const char** seed_urls, size_t seed_count)
{
    for (size_t i = 0; i < seed_count; i++) {
        if (visited_set_try_add(&crawler->visited, seed_urls[i])) {
            url_queue_push(&crawler->queue, seed_urls[i], 0);
        }
    }

    int num_threads = crawler->config.num_threads > 0 ? crawler->config.num_threads : 1;
    pthread_t* threads = (pthread_t*)calloc((size_t)num_threads, sizeof(pthread_t));
    if (threads == NULL) {
        return -1;
    }

    int spawned = 0;
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, worker_thread, crawler) != 0) {
            break;
        }
        spawned++;
    }

    if (spawned < num_threads) {
        /* Nismo uspeli da pokrenemo sve thread-ove; ugasi sve i sačekaj one koji rade. */
        url_queue_shutdown(&crawler->queue);
    }

    for (int i = 0; i < spawned; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    return spawned;
}

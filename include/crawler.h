#ifndef CRAWLER_H
#define CRAWLER_H

#include "url_queue.h"
#include "visited_set.h"

#include <pthread.h>
#include <stddef.h>

typedef struct crawler_config {
    int num_threads;                 /* veličina thread pool-a */
    int max_depth;                   /* 0 = samo seed stranice, bez pratenja linkova */
    size_t max_pages;                /* 0 = bez ograničenja */
    unsigned int fetch_timeout_seconds;
    int verbose;                     /* ispis reda kako se stranice preuzimaju */
} crawler_config_t;

typedef struct crawler_stats {
    pthread_mutex_t mutex;
    size_t pages_fetched;
    size_t pages_failed;
    size_t links_discovered;
    size_t max_pages;    /* kopija limita iz config-a, 0 = bez ograničenja */
    int limit_reached;
} crawler_stats_t;

typedef struct crawler {
    url_queue_t queue;
    visited_set_t visited;
    crawler_stats_t stats;
    crawler_config_t config;
    pthread_mutex_t print_mutex; /* sprečava mešanje log linija iz različitih thread-ova */
} crawler_t;

int crawler_init(crawler_t* crawler, const crawler_config_t* config);
void crawler_destroy(crawler_t* crawler);

/*
 * Dodaje seed URL-ove (dubina 0) u red i pokreće pool worker thread-ova.
 * Blokira dok se čitav crawl ne završi (red prazan i sav posao gotov,
 * ili je dostignut max_pages limit). Vraća broj uspešno pokrenutih
 * worker thread-ova (== config.num_threads) ili -1 na grešku inicijalizacije.
 */
int crawler_run(crawler_t* crawler, const char** seed_urls, size_t seed_count);

#endif

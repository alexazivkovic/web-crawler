#include "visited_set.h"

#include <stdlib.h>
#include <string.h>

static unsigned long hash_string(const char* s)
{
    /* djb2 */
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*s++) != 0) {
        hash = ((hash << 5) + hash) + (unsigned long)c;
    }
    return hash;
}

int visited_set_init(visited_set_t* set, size_t bucket_count)
{
    if (bucket_count == 0) {
        bucket_count = 1;
    }

    set->buckets = (visited_node_t**)calloc(bucket_count, sizeof(visited_node_t*));
    if (set->buckets == NULL) {
        return -1;
    }
    set->bucket_count = bucket_count;
    set->count = 0;

    if (pthread_mutex_init(&set->mutex, NULL) != 0) {
        free(set->buckets);
        return -1;
    }
    return 0;
}

void visited_set_destroy(visited_set_t* set)
{
    for (size_t i = 0; i < set->bucket_count; i++) {
        visited_node_t* node = set->buckets[i];
        while (node != NULL) {
            visited_node_t* next = node->next;
            free(node->url);
            free(node);
            node = next;
        }
    }
    free(set->buckets);
    pthread_mutex_destroy(&set->mutex);
}

int visited_set_try_add(visited_set_t* set, const char* url)
{
    unsigned long h = hash_string(url) % set->bucket_count;

    pthread_mutex_lock(&set->mutex);

    for (visited_node_t* node = set->buckets[h]; node != NULL; node = node->next) {
        if (strcmp(node->url, url) == 0) {
            pthread_mutex_unlock(&set->mutex);
            return 0;
        }
    }

    visited_node_t* node = (visited_node_t*)malloc(sizeof(*node));
    if (node == NULL) {
        pthread_mutex_unlock(&set->mutex);
        return 0;
    }
    node->url = strdup(url);
    node->next = set->buckets[h];
    set->buckets[h] = node;
    set->count++;

    pthread_mutex_unlock(&set->mutex);
    return 1;
}

size_t visited_set_size(visited_set_t* set)
{
    pthread_mutex_lock(&set->mutex);
    size_t n = set->count;
    pthread_mutex_unlock(&set->mutex);
    return n;
}

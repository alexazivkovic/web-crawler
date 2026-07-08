#include "url_queue.h"

#include <stdlib.h>
#include <string.h>

int url_queue_init(url_queue_t* q)
{
    memset(q, 0, sizeof(*q));

    if (pthread_mutex_init(&q->mutex, NULL) != 0) {
        return -1;
    }
    if (pthread_cond_init(&q->cond_not_empty, NULL) != 0) {
        pthread_mutex_destroy(&q->mutex);
        return -1;
    }
    return 0;
}

void url_queue_destroy(url_queue_t* q)
{
    url_queue_node_t* node = q->head;
    while (node != NULL) {
        url_queue_node_t* next = node->next;
        free(node->url);
        free(node);
        node = next;
    }
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond_not_empty);
}

int url_queue_push(url_queue_t* q, const char* url, int depth)
{
    url_queue_node_t* node = (url_queue_node_t*)malloc(sizeof(*node));
    if (node == NULL) {
        return -1;
    }

    node->url = strdup(url);
    if (node->url == NULL) {
        free(node);
        return -1;
    }
    node->depth = depth;
    node->next = NULL;

    pthread_mutex_lock(&q->mutex);

    if (q->tail == NULL) {
        q->head = node;
    } else {
        q->tail->next = node;
    }
    q->tail = node;
    q->size++;
    q->pending_count++;

    pthread_cond_broadcast(&q->cond_not_empty);
    pthread_mutex_unlock(&q->mutex);
    return 0;
}

int url_queue_pop(url_queue_t* q, char** out_url, int* out_depth)
{
    pthread_mutex_lock(&q->mutex);

    while (q->head == NULL && q->pending_count > 0 && !q->shutdown) {
        pthread_cond_wait(&q->cond_not_empty, &q->mutex);
    }

    if (q->shutdown || (q->head == NULL && q->pending_count == 0)) {
        /* Nema više posla - probudi ostale i javi pozivaocu da se ugasi. */
        pthread_cond_broadcast(&q->cond_not_empty);
        pthread_mutex_unlock(&q->mutex);
        return 0;
    }

    url_queue_node_t* node = q->head;
    q->head = node->next;
    if (q->head == NULL) {
        q->tail = NULL;
    }
    q->size--;

    pthread_mutex_unlock(&q->mutex);

    *out_url = node->url;
    *out_depth = node->depth;
    free(node);
    return 1;
}

void url_queue_task_done(url_queue_t* q)
{
    pthread_mutex_lock(&q->mutex);
    if (q->pending_count > 0) {
        q->pending_count--;
    }
    if (q->pending_count == 0) {
        pthread_cond_broadcast(&q->cond_not_empty);
    }
    pthread_mutex_unlock(&q->mutex);
}

void url_queue_shutdown(url_queue_t* q)
{
    pthread_mutex_lock(&q->mutex);
    q->shutdown = 1;
    pthread_cond_broadcast(&q->cond_not_empty);
    pthread_mutex_unlock(&q->mutex);
}

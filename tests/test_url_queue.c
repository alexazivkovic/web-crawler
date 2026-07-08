#include "url_queue.h"

#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <pthread.h>

Test(url_queue, fifo_order)
{
    url_queue_t q;
    cr_assert(eq(int, url_queue_init(&q), 0));

    url_queue_push(&q, "https://a.example", 0);
    url_queue_push(&q, "https://b.example", 1);

    char* url = NULL;
    int depth = -1;

    cr_assert(eq(int, url_queue_pop(&q, &url, &depth), 1));
    cr_assert(eq(str, url, "https://a.example"));
    cr_assert(eq(int, depth, 0));
    free(url);
    url_queue_task_done(&q);

    cr_assert(eq(int, url_queue_pop(&q, &url, &depth), 1));
    cr_assert(eq(str, url, "https://b.example"));
    cr_assert(eq(int, depth, 1));
    free(url);
    url_queue_task_done(&q);

    url_queue_destroy(&q);
}

Test(url_queue, pop_returns_zero_when_work_is_done)
{
    url_queue_t q;
    url_queue_init(&q);

    url_queue_push(&q, "https://a.example", 0);

    char* url = NULL;
    int depth = -1;
    cr_assert(eq(int, url_queue_pop(&q, &url, &depth), 1));
    free(url);

    /* task_done pre nego sto je pop pozvan opet: pending_count -> 0, pa
     * sledeci pop odmah vraca 0 (nema vise posla), umesto da blokira. */
    url_queue_task_done(&q);

    cr_assert(eq(int, url_queue_pop(&q, &url, &depth), 0));

    url_queue_destroy(&q);
}

typedef struct producer_arg {
    url_queue_t* q;
    int start;
    int count;
} producer_arg_t;

static void* producer(void* raw)
{
    producer_arg_t* arg = (producer_arg_t*)raw;
    char buf[64];
    for (int i = 0; i < arg->count; i++) {
        snprintf(buf, sizeof(buf), "https://x.example/%d", arg->start + i);
        url_queue_push(arg->q, buf, 0);
    }
    return NULL;
}

Test(url_queue, concurrent_producers_no_lost_or_duplicated_items)
{
    url_queue_t q;
    url_queue_init(&q);

    const int num_producers = 4;
    const int per_producer = 500;
    pthread_t threads[4];
    producer_arg_t args[4];

    for (int i = 0; i < num_producers; i++) {
        args[i].q = &q;
        args[i].start = i * per_producer;
        args[i].count = per_producer;
        pthread_create(&threads[i], NULL, producer, &args[i]);
    }
    for (int i = 0; i < num_producers; i++) {
        pthread_join(threads[i], NULL);
    }

    int consumed = 0;
    char* url = NULL;
    int depth = 0;
    while (url_queue_pop(&q, &url, &depth) == 1) {
        consumed++;
        free(url);
        url_queue_task_done(&q);
    }

    cr_assert(eq(int, consumed, num_producers * per_producer));

    url_queue_destroy(&q);
}

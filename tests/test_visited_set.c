#include "visited_set.h"

#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <pthread.h>
#include <stdio.h>

Test(visited_set, first_add_succeeds_second_fails)
{
    visited_set_t set;
    visited_set_init(&set, 16);

    cr_assert(eq(int, visited_set_try_add(&set, "https://example.com"), 1));
    cr_assert(eq(int, visited_set_try_add(&set, "https://example.com"), 0));
    cr_assert(eq(u64, (uint64_t)visited_set_size(&set), (uint64_t)1));

    visited_set_destroy(&set);
}

Test(visited_set, distinct_urls_are_independent)
{
    visited_set_t set;
    visited_set_init(&set, 16);

    cr_assert(eq(int, visited_set_try_add(&set, "https://a.example"), 1));
    cr_assert(eq(int, visited_set_try_add(&set, "https://b.example"), 1));
    cr_assert(eq(u64, (uint64_t)visited_set_size(&set), (uint64_t)2));

    visited_set_destroy(&set);
}

typedef struct racer_arg {
    visited_set_t* set;
    const char* url;
    int* success_count;
    pthread_mutex_t* counter_mutex;
} racer_arg_t;

static void* racer(void* raw)
{
    racer_arg_t* arg = (racer_arg_t*)raw;
    int added = visited_set_try_add(arg->set, arg->url);
    if (added) {
        pthread_mutex_lock(arg->counter_mutex);
        (*arg->success_count)++;
        pthread_mutex_unlock(arg->counter_mutex);
    }
    return NULL;
}

Test(visited_set, only_one_thread_wins_race_on_same_url)
{
    visited_set_t set;
    visited_set_init(&set, 16);

    const int num_threads = 32;
    pthread_t threads[32];
    int success_count = 0;
    pthread_mutex_t counter_mutex;
    pthread_mutex_init(&counter_mutex, NULL);

    racer_arg_t arg = {
        .set = &set,
        .url = "https://same-url.example/page",
        .success_count = &success_count,
        .counter_mutex = &counter_mutex,
    };

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, racer, &arg);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    /* Bez obzira na broj konkurentnih pokusaja, URL sme biti "dodat" tacno jednom. */
    cr_assert(eq(int, success_count, 1));
    cr_assert(eq(u64, (uint64_t)visited_set_size(&set), (uint64_t)1));

    pthread_mutex_destroy(&counter_mutex);
    visited_set_destroy(&set);
}

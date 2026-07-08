#ifndef VISITED_SET_H
#define VISITED_SET_H

#include <pthread.h>
#include <stddef.h>

/*
 * Deljeni, thread-safe hash skup posećenih URL-ova.
 *
 * Koristi se da nijedan URL ne bude obiđen dva puta, čak i kada više
 * worker-a istovremeno otkrije isti link na različitim stranicama.
 * Ključna operacija je visited_set_try_add(): ona atomično proverava
 * da li URL već postoji i, ako ne postoji, odmah ga dodaje - sve unutar
 * jednog zaključavanja mutex-a, čime se izbegava race-condition između
 * "proveri" i "dodaj".
 */

typedef struct visited_node {
    char* url;
    struct visited_node* next;
} visited_node_t;

typedef struct visited_set {
    visited_node_t** buckets;
    size_t bucket_count;
    size_t count;
    pthread_mutex_t mutex;
} visited_set_t;

int visited_set_init(visited_set_t* set, size_t bucket_count);
void visited_set_destroy(visited_set_t* set);

/*
 * Vraća 1 ako je URL upravo dodat (nije ranije postojao), a 0 ako je
 * URL već bio u skupu (poziv nema efekta). Thread-safe i atomično.
 */
int visited_set_try_add(visited_set_t* set, const char* url);

/* Trenutni broj posećenih URL-ova. */
size_t visited_set_size(visited_set_t* set);

#endif

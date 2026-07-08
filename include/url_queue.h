#ifndef URL_QUEUE_H
#define URL_QUEUE_H

#include <pthread.h>

/*
 * Deljeni, thread-safe FIFO red URL-ova.
 *
 * Pored same liste čekanja, red prati i broj "aktivnih zadataka"
 * (pending_count) = broj URL-ova koji su u redu ILI se trenutno obrađuju
 * od strane nekog worker thread-a. To je neophodno da bi worker-i mogli
 * ispravno da detektuju kraj posla: red može trenutno biti prazan dok
 * neki drugi worker upravo parsira stranicu i uskoro će dodati nove linkove.
 * Kada pending_count padne na 0, posao je zaista završen i svi worker-i
 * mogu bezbedno da se ugase (graceful shutdown).
 */

typedef struct url_queue_node {
    char* url;
    int depth;
    struct url_queue_node* next;
} url_queue_node_t;

typedef struct url_queue {
    url_queue_node_t* head;
    url_queue_node_t* tail;
    size_t size;

    /* broj URL-ova u redu ili u obradi; 0 znači "posao gotov" */
    size_t pending_count;

    int shutdown;

    pthread_mutex_t mutex;
    pthread_cond_t cond_not_empty;
} url_queue_t;

int url_queue_init(url_queue_t* q);
void url_queue_destroy(url_queue_t* q);

/* Dodaje URL na kraj reda i uvećava pending_count. */
int url_queue_push(url_queue_t* q, const char* url, int depth);

/*
 * Skida URL sa početka reda. Blokira se dok red ne dobije element ili
 * dok pending_count ne padne na 0 (nema više posla). Vraća 1 ako je URL
 * uspešno preuzet (u out_url / out_depth), a 0 ako je posao završen
 * (pozivalac treba da se ugasi).
 */
int url_queue_pop(url_queue_t* q, char** out_url, int* out_depth);

/*
 * Poziva se kada worker završi obradu jednog URL-a (bez obzira da li je
 * dodao nove linkove). Smanjuje pending_count; ako padne na 0, budi sve
 * čekajuce thread-ove da mogu da se ugase.
 */
void url_queue_task_done(url_queue_t* q);

/*
 * Prisilno gašenje (npr. dostignut limit stranica ili Ctrl+C). Budi sve
 * blokirane worker-e da odmah izađu iz url_queue_pop, bez obzira na
 * pending_count.
 */
void url_queue_shutdown(url_queue_t* q);

#endif

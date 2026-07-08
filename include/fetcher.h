#ifndef FETCHER_H
#define FETCHER_H

#include <stddef.h>

typedef struct fetch_result {
    char* body;         /* HTML sadržaj stranice, NUL-terminated */
    size_t size;        /* dužina bez terminatora */
    long http_status;   /* HTTP status kod (0 ako konekcija nije uspela) */
    char* content_type; /* vrednost Content-Type header-a, može biti NULL */
} fetch_result_t;

/* Poziva se jednom pre bilo kakvog fetch-a (curl_global_init). Nije thread-safe. */
int fetcher_global_init(void);
void fetcher_global_cleanup(void);

/*
 * Preuzima sadržaj sa zadatog URL-a. Thread-safe (svaki poziv koristi
 * sopstveni CURL handle), pa se sme pozivati paralelno iz više thread-ova.
 * Vraća 0 na uspeh (nezavisno od HTTP status koda) i popunjava *out.
 * Vraća -1 ako konekcija/transfer nije uspeo (npr. DNS greška, timeout).
 * Pozivalac je odgovoran da oslobodi *out preko fetch_result_free().
 */
int fetcher_get(const char* url, unsigned int timeout_seconds, fetch_result_t* out);

void fetch_result_free(fetch_result_t* result);

#endif

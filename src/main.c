#include "crawler.h"
#include "fetcher.h"
#include "utils/watchdog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void print_usage(const char* prog_name)
{
    printf("Konkurentni web crawler - deljeni red URL-ova i skup posecenih stranica\n\n");
    printf("Upotreba: %s [OPCIJE] <seed-url> [seed-url2 ...]\n\n", prog_name);
    printf("Opcije:\n");
    printf("  -t, --threads N     broj worker thread-ova (podrazumevano: 4)\n");
    printf("  -d, --max-depth N   maksimalna dubina pracenja linkova (podrazumevano: 2)\n");
    printf("  -p, --max-pages N   maksimalan broj stranica, 0 = bez limita (podrazumevano: 50)\n");
    printf("  -T, --timeout SEC   timeout po HTTP zahtevu u sekundama (podrazumevano: 10)\n");
    printf("  -w, --watchdog SEC  tvrdi limit trajanja programa, 0 = iskljuceno (podrazumevano: 120)\n");
    printf("  -v, --verbose       ispisuje svaku preuzetu stranicu tokom rada\n");
    printf("  -h, --help          prikazuje ovu poruku\n\n");
    printf("Primer:\n");
    printf("  %s -t 8 -d 2 -p 100 https://example.com\n", prog_name);
}

static double elapsed_seconds(struct timespec start, struct timespec end)
{
    return (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / 1e9;
}

int main(int argc, char** argv)
{
    int num_threads = 4;
    int max_depth = 2;
    long max_pages = 50;
    unsigned int timeout_seconds = 10;
    unsigned int watchdog_seconds = 120;
    int verbose = 0;

    const char* seed_urls[256];
    size_t seed_count = 0;

    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];

        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if ((strcmp(arg, "-t") == 0 || strcmp(arg, "--threads") == 0) && i + 1 < argc) {
            num_threads = atoi(argv[++i]);
        } else if ((strcmp(arg, "-d") == 0 || strcmp(arg, "--max-depth") == 0) && i + 1 < argc) {
            max_depth = atoi(argv[++i]);
        } else if ((strcmp(arg, "-p") == 0 || strcmp(arg, "--max-pages") == 0) && i + 1 < argc) {
            max_pages = atol(argv[++i]);
        } else if ((strcmp(arg, "-T") == 0 || strcmp(arg, "--timeout") == 0) && i + 1 < argc) {
            timeout_seconds = (unsigned int)atoi(argv[++i]);
        } else if ((strcmp(arg, "-w") == 0 || strcmp(arg, "--watchdog") == 0) && i + 1 < argc) {
            watchdog_seconds = (unsigned int)atoi(argv[++i]);
        } else if (strcmp(arg, "-v") == 0 || strcmp(arg, "--verbose") == 0) {
            verbose = 1;
        } else if (arg[0] == '-') {
            fprintf(stderr, "Nepoznata opcija: %s\n\n", arg);
            print_usage(argv[0]);
            return 1;
        } else {
            if (seed_count >= sizeof(seed_urls) / sizeof(seed_urls[0])) {
                fprintf(stderr, "Previse seed URL-ova (max %zu)\n", sizeof(seed_urls) / sizeof(seed_urls[0]));
                return 1;
            }
            seed_urls[seed_count++] = arg;
        }
    }

    if (seed_count == 0) {
        fprintf(stderr, "Greska: potrebno je navesti bar jedan seed URL.\n\n");
        print_usage(argv[0]);
        return 1;
    }
    if (num_threads < 1) {
        num_threads = 1;
    }
    if (max_depth < 0) {
        max_depth = 0;
    }

    if (watchdog_seconds > 0) {
        /* Sigurnosna mreza: ako program iz bilo kog razloga ne zavrsi na vreme
         * (npr. greska u sinhronizaciji dovede do deadlock-a), proces se
         * prisilno gasi umesto da visi zauvek. */
        watchdog_start_exit_after_us(watchdog_seconds * 1000000u, 124);
    }

    if (fetcher_global_init() != 0) {
        fprintf(stderr, "Greska: ne mogu da inicijalizujem libcurl.\n");
        return 1;
    }

    crawler_config_t config = {
        .num_threads = num_threads,
        .max_depth = max_depth,
        .max_pages = max_pages > 0 ? (size_t)max_pages : 0,
        .fetch_timeout_seconds = timeout_seconds,
        .verbose = verbose,
    };

    crawler_t crawler;
    if (crawler_init(&crawler, &config) != 0) {
        fprintf(stderr, "Greska: ne mogu da inicijalizujem crawler.\n");
        fetcher_global_cleanup();
        return 1;
    }

    printf("Pokrecem crawl sa %d thread-ova, max-depth=%d, max-pages=%zu ...\n",
        num_threads, max_depth, config.max_pages);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int spawned = crawler_run(&crawler, seed_urls, seed_count);

    clock_gettime(CLOCK_MONOTONIC, &end);

    if (spawned <= 0) {
        fprintf(stderr, "Greska: nijedan worker thread nije uspesno pokrenut.\n");
        crawler_destroy(&crawler);
        fetcher_global_cleanup();
        return 1;
    }

    printf("\n=== Rezultat ===\n");
    printf("Preuzeto stranica (uspesno):  %zu\n", crawler.stats.pages_fetched);
    printf("Neuspesnih preuzimanja:       %zu\n", crawler.stats.pages_failed);
    printf("Otkriveno novih linkova:      %zu\n", crawler.stats.links_discovered);
    printf("Ukupno jedinstvenih URL-ova:  %zu\n", visited_set_size(&crawler.visited));
    printf("Vreme izvrsavanja:            %.2f s\n", elapsed_seconds(start, end));

    crawler_destroy(&crawler);
    fetcher_global_cleanup();
    return 0;
}

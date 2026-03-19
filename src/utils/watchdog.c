#include "utils/watchdog.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct watchdog_cfg {
    unsigned int timeout_us;
    int exit_code;
} watchdog_cfg_t;

static void* watchdog_runner(void* arg)
{
    watchdog_cfg_t* cfg = (watchdog_cfg_t*)arg;

    usleep(cfg->timeout_us);
    _exit(cfg->exit_code);
}

int watchdog_start_exit_after_us(unsigned int timeout_us,
    int exit_code)
{
    pthread_t thread;
    watchdog_cfg_t* cfg = (watchdog_cfg_t*)calloc(1, sizeof(*cfg));


    if (cfg == NULL) {
        return -1;
    }

    cfg->timeout_us = timeout_us;
    cfg->exit_code = exit_code;

    if (pthread_create(&thread, NULL, watchdog_runner, cfg) != 0) {
        free(cfg);
        return -1;
    }

    pthread_detach(thread);
    return 0;
}

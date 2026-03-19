#ifndef WATCHDOG_H
#define WATCHDOG_H

int watchdog_start_exit_after_us(unsigned int timeout_us,
    int exit_code);

#endif


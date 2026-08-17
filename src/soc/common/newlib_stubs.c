/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * This file is part of Espressif's BIST (Built-In Self Test) Library.
 *
 * BIST library is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * BIST library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with BIST library. If not, see
 * <https://www.gnu.org/licenses/lgpl-3.0.html>.
 */

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "bist_log.h"

extern unsigned char _heap_start, _heap_end;

/* Satisfy linker --undefined=uxTopUsedPriority (FreeRTOS not used in BIST). */
uint32_t uxTopUsedPriority;

/* libc calls this to get the current thread’s reent struct.
 * In a single‑threaded system we always return the one global copy. */
struct _reent *__getreent(void)
{
    return _GLOBAL_REENT;      // == &_impure_data by default
}

void *_sbrk(int incr)
{
    static unsigned char *cur = &_heap_start;
    unsigned char *prev = cur;
    if (cur + incr > &_heap_end) {
        ESP_LOGE("heap", "Heap out of memory");
        return (void *) -1;
    }
    cur += incr;
    return prev;
}

void *_sbrk_r(struct _reent *r, int incr)
{
    (void)r; /* unused */
    return _sbrk(incr);
}

/* Stubs for newlib stdio/posix so -lc links (bare metal, no OS). */
int _read_r(struct _reent *r, int fd, void *buf, size_t cnt)
{
    (void)r;
    (void)fd;
    (void)buf;
    (void)cnt;
    return -1;
}

int _write_r(struct _reent *r, int fd, const void *buf, size_t cnt)
{
    (void)r;
    (void)fd;
    (void)buf;
    (void)cnt;
    return (int)cnt;
}

off_t _lseek_r(struct _reent *r, int fd, off_t offset, int whence)
{
    (void)r;
    (void)fd;
    (void)offset;
    (void)whence;
    return (off_t) -1;
}

int _close_r(struct _reent *r, int fd)
{
    (void)r;
    (void)fd;
    return -1;
}

/* Stub for newlib stdio when built with pthread (bare metal: no threads). */
int pthread_setcancelstate(int state, int *oldstate)
{
    (void)state;
    if (oldstate != NULL) {
        *oldstate = 0;
    }
    return 0;
}

void _exit(int status)
{
    (void)status;
    while (1) {}
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    return -1;
}

int _getpid(void)
{
    return 1;
}

/* Stubs for IDF v6.0 esp_sleep_sub_mode functions (bare metal: no sleep). */
#include <stdbool.h>
#include <stddef.h>

typedef enum { ESP_SLEEP_RTC_USE_RC_FAST_MODE = 0 } esp_sleep_sub_mode_t;

bool *esp_sleep_sub_mode_dump_config(bool *out)
{
    static bool dummy[1] = { false };
    (void)out;
    return dummy;
}

void esp_sleep_sub_mode_config(esp_sleep_sub_mode_t mode, bool activate)
{
    (void)mode;
    (void)activate;
}

void esp_sleep_sub_mode_force_disable(esp_sleep_sub_mode_t mode)
{
    (void)mode;
}

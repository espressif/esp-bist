/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * NuttX platform adapter for the Host Diagnostic Agent.
 *
 * Uses pthreads for the high-priority worker and a POSIX message queue
 * for the LP status bridge between the agent and the application.
 */

#include "bist_hd_platform.h"

#include <nuttx/config.h>

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <unistd.h>

#include <nuttx/clock.h>

#ifndef CONFIG_ESP_BIST_HD_AGENT_TASK_STACK
#define CONFIG_ESP_BIST_HD_AGENT_TASK_STACK 3072
#endif

#ifndef CONFIG_ESP_BIST_HD_AGENT_TASK_PRIO_NUTTX
#define CONFIG_ESP_BIST_HD_AGENT_TASK_PRIO_NUTTX 254
#endif

#define LP_STATUS_QUEUE_NAME "/bist_hd_lp_status"
#define LP_STATUS_QUEUE_LEN  4

static mqd_t s_lp_status_q = (mqd_t)-1;

int bist_hd_platform_init(void)
{
    struct mq_attr attr;

    if (s_lp_status_q != (mqd_t)-1) {
        return 0;
    }

    attr.mq_flags   = 0;
    attr.mq_maxmsg  = LP_STATUS_QUEUE_LEN;
    attr.mq_msgsize = sizeof(uint32_t);
    attr.mq_curmsgs = 0;

    s_lp_status_q = mq_open(LP_STATUS_QUEUE_NAME,
                             O_CREAT | O_RDWR, 0600, &attr);
    return (s_lp_status_q != (mqd_t)-1) ? 0 : -1;
}

struct worker_arg
{
    bist_hd_worker_fn_t fn;
    void *arg;
};

static void *worker_trampoline(void *opaque)
{
    struct worker_arg *wa = (struct worker_arg *)opaque;

    wa->fn(wa->arg);
    return NULL;
}

static struct worker_arg s_worker_arg;

int bist_hd_platform_start_worker(bist_hd_worker_fn_t fn, void *arg)
{
    pthread_t tid;
    pthread_attr_t attr;
    struct sched_param param;

    if (fn == NULL) {
        return -1;
    }

    s_worker_arg.fn  = fn;
    s_worker_arg.arg = arg;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr,
                              CONFIG_ESP_BIST_HD_AGENT_TASK_STACK);

    param.sched_priority = CONFIG_ESP_BIST_HD_AGENT_TASK_PRIO_NUTTX;
    pthread_attr_setschedparam(&attr, &param);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

    if (pthread_create(&tid, &attr, worker_trampoline,
                       &s_worker_arg) != 0) {
        pthread_attr_destroy(&attr);
        return -1;
    }

    pthread_detach(tid);
    pthread_attr_destroy(&attr);
    return 0;
}

int bist_hd_platform_lp_status_push(uint32_t status)
{
    if (s_lp_status_q == (mqd_t)-1) {
        return -1;
    }

    return (mq_send(s_lp_status_q, (const char *)&status,
                    sizeof(status), 0) == 0) ? 0 : -1;
}

int bist_hd_platform_lp_status_pop(uint32_t *status_out, int32_t timeout_ms)
{
    struct timespec now;
    struct timespec delay;
    struct timespec timeout;
    ssize_t n;

    if (status_out == NULL || s_lp_status_q == (mqd_t)-1) {
        return -1;
    }

    if (timeout_ms < 0) {
        n = mq_receive(s_lp_status_q, (char *)status_out,
                       sizeof(*status_out), NULL);
    } else {
        clock_gettime(CLOCK_REALTIME, &now);
        delay.tv_sec  = timeout_ms / 1000;
        delay.tv_nsec = (timeout_ms % 1000) * 1000000L;
        clock_timespec_add(&now, &delay, &timeout);
        n = mq_timedreceive(s_lp_status_q, (char *)status_out,
                            sizeof(*status_out), NULL, &timeout);
    }

    return (n == (ssize_t)sizeof(*status_out)) ? 0 : -1;
}

uint32_t bist_hd_platform_time_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

void bist_hd_platform_sleep_ms(uint32_t ms)
{
    usleep(ms * 1000u);
}

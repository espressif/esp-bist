/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * HP-core side of the Host Diagnostics fail-closed validation app (NuttX).
 *
 * This is not an integration example; samples/nuttx is. Here the host
 * deliberately misbehaves so the LP companion must enter safe state, stop
 * feeding the LP watchdog and reset the chip.
 *
 * The ESP-BIST library is built exactly as a product would build it. The fault
 * comes from this app: either the image carries a challenge key the companion
 * does not share (see CMakeLists.txt) or main() withholds the agent past the
 * challenge window (starve_agent below).
 */

#include <nuttx/config.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "bist_hd_agent.h"
#include "bist_hd_protocol.h"
#include "ulp/ulp/ulp_code.h"

#if !defined(CONFIG_BIST_HD_TEST_KEY_MISMATCH) && !defined(CONFIG_BIST_HD_TEST_STARVE_AGENT)
#error "Select a fault: CONFIG_BIST_HD_TEST_KEY_MISMATCH or CONFIG_BIST_HD_TEST_STARVE_AGENT"
#endif

#define MAILBOX_TIMEOUT_MS 10000

#ifdef CONFIG_BIST_HD_TEST_STARVE_AGENT
/* Outlasts the challenge window plus the companion LP watchdog timeout. */
#define STARVE_US 500000

/*
 * Busy-wait without yielding. usleep() would block and let the lower-priority
 * agent run; a CLOCK_MONOTONIC spin at SCHED_PRIORITY_MAX does not.
 */
static void busy_wait_us(useconds_t usec)
{
    struct timespec now;
    struct timespec end;

    clock_gettime(CLOCK_MONOTONIC, &now);
    end.tv_sec = now.tv_sec + (time_t)(usec / 1000000u);
    end.tv_nsec = now.tv_nsec + (long)(usec % 1000000u) * 1000L;
    if (end.tv_nsec >= 1000000000L) {
        end.tv_sec++;
        end.tv_nsec -= 1000000000L;
    }

    do {
        clock_gettime(CLOCK_MONOTONIC, &now);
    } while (now.tv_sec < end.tv_sec ||
             (now.tv_sec == end.tv_sec && now.tv_nsec < end.tv_nsec));
}

/*
 * Model a host too busy to answer: run above the agent task priority and never
 * yield, so the agent is not scheduled and no ANSWER reaches the companion
 * before the window closes. Nothing in the library participates.
 *
 * The NuttX ulp board configs used by this test are uniprocessor, so occupying
 * the current core is enough. (ESP-IDF pins extra starve tasks on SMP.)
 */
static void starve_agent(void)
{
    struct sched_param param;

    fflush(stdout);

    param.sched_priority = SCHED_PRIORITY_MAX;
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0) {
        printf("test_HD_starve:FAIL\n");
        return;
    }

    busy_wait_us(STARVE_US);
}
#endif

int main(int argc, FAR char *argv[])
{
    uint32_t status;
    int ulp_fd;
    int lp_uart_fd;
    int err;

    (void)argc;
    (void)argv;

    lp_uart_fd = open("/dev/ttyS1", O_WRONLY);
    if (lp_uart_fd < 0) {
        printf("Failed to open LP-UART: %d\n", errno);
        return -1;
    }

    if (write(lp_uart_fd, "\n", 1) < 0) {
        printf("Failed to initialize LP-UART: %d\n", errno);
        return -1;
    }

    ulp_fd = open("/dev/ulp", O_WRONLY);
    if (ulp_fd < 0) {
        printf("Failed to open ULP: %d\n", errno);
        return -1;
    }

    if (write(ulp_fd, hd_nuttx_test_bin, hd_nuttx_test_bin_len) < 0) {
        printf("Failed to load ULP binary: %d\n", errno);
        close(ulp_fd);
        return -1;
    }

    close(ulp_fd);

    /* Allow LP core to initialize the SW mailbox context. */
    sleep(1);

    err = bist_hd_agent_start();
    if (err != 0) {
        printf("test_HD_agent_ready:FAIL\n");
        return -1;
    }
    printf("test_HD_agent_ready:PASS\n");

    err = bist_hd_agent_wait_lp_status(&status, BIST_HD_BIT_POSTBOOT, MAILBOX_TIMEOUT_MS);
    printf("test_BIST_postboot:%s\n", (err == 0) ? "PASS" : "TIMEOUT");

    /*
     * One runtime LP_STATUS is reported before the first challenge, so this
     * marker confirms the supervision loop is live before the fault lands.
     */
    err = bist_hd_agent_wait_lp_status(&status, BIST_HD_BIT_RUNTIME, MAILBOX_TIMEOUT_MS);
    printf("test_HD_fail_closed_armed:%s\n", (err == 0) ? "PASS" : "FAIL");

#ifdef CONFIG_BIST_HD_TEST_STARVE_AGENT
    starve_agent();
#endif

    /*
     * The key-mismatch build has no busy-wait: every answer is wrong, so the
     * companion reaches its safe-state threshold while this task idles.
     */
    while (1) {
        sleep(1);
    }

    return 0;
}

/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Host Diagnostics platform adapter (HP side).
 *
 * OS-specific implementation lives in host/idf (Zephyr deferred).
 * The common agent only uses this API plus bist_hd_transport.
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Worker entry point started by bist_hd_platform_start_worker(). */
typedef void (*bist_hd_worker_fn_t)(void *arg);

/**
 * @brief Initialize platform resources (queues, etc.).
 *
 * @return 0 on success, negative errno-style value on failure
 */
int bist_hd_platform_init(void);

/**
 * @brief Start the high-priority agent worker thread/task.
 *
 * @param fn Worker function (never returns on the normal path)
 * @param arg Opaque argument passed to @p fn
 * @return 0 on success, negative errno-style value on failure
 */
int bist_hd_platform_start_worker(bist_hd_worker_fn_t fn, void *arg);

/**
 * @brief Push an LP status word for the application to wait on (non-blocking).
 *
 * @return 0 on success, negative errno-style value if the queue is full / unavailable
 */
int bist_hd_platform_lp_status_push(uint32_t status);

/**
 * @brief Pop the next LP status word.
 *
 * @param status_out Receives the status bitmask
 * @param timeout_ms Timeout in milliseconds (-1 = wait forever)
 * @return 0 on success, negative errno-style value on failure / timeout
 */
int bist_hd_platform_lp_status_pop(uint32_t *status_out, int32_t timeout_ms);

/**
 * @brief Monotonic millisecond tick for timeout accounting.
 */
uint32_t bist_hd_platform_time_ms(void);

/**
 * @brief Block the calling thread for @p ms milliseconds.
 *
 * Used on agent error paths: a transport that fails without blocking would
 * otherwise spin the high-priority agent task and starve the system.
 */
void bist_hd_platform_sleep_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

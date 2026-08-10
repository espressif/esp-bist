/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Host Diagnostics companion OS port (LP side).
 *
 * The companion state machine is OS-neutral; every LP-core framework
 * (ESP-IDF ULP, Zephyr lpcore) supplies one implementation of this API.
 */

#pragma once

#include <stdint.h>

#include "bist_hd_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the companion transport and time base.
 *
 * @return 0 on success, negative errno-style value on failure
 */
int bist_hd_comp_port_init(void);

/**
 * @brief Send an encoded Host Diagnostics message to the host agent.
 *
 * @param msg Message to send
 * @return 0 on success, negative errno-style value on failure
 */
int bist_hd_comp_port_send(const bist_hd_msg_t *msg);

/**
 * @brief Receive and decode a Host Diagnostics message from the host agent.
 *
 * @param msg Output message
 * @param timeout_us Timeout in microseconds (-1 = wait forever)
 * @return 0 on success, negative errno-style value on failure / timeout
 */
int bist_hd_comp_port_recv(bist_hd_msg_t *msg, int32_t timeout_us);

/**
 * @brief Sample the free-running hardware tick counter.
 *
 * The unit is port-defined; pair it with bist_hd_comp_port_elapsed_us().
 */
uint32_t bist_hd_comp_port_tick(void);

/**
 * @brief Microseconds elapsed since @p start_tick.
 *
 * The subtraction happens in the native tick domain, so a single wrap of the
 * counter is accounted for correctly.
 */
uint32_t bist_hd_comp_port_elapsed_us(uint32_t start_tick);

/**
 * @brief Busy-wait for @p us microseconds.
 *
 * Companion pacing must not depend on an OS tick: the LP image may run
 * without a system clock.
 */
void bist_hd_comp_port_delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif

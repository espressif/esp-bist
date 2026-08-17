/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Host Diagnostics transport API (HP side). OS-specific implementation
 * lives in host/idf (Zephyr deferred).
 */

#pragma once

#include "bist_hd_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the HP↔LP transport (mailbox).
 *
 * @return 0 on success, negative errno-style value on failure
 */
int bist_hd_transport_init(void);

/**
 * @brief Discard receive state left over from a previous HP session.
 *
 * The LP companion keeps running across an HP reset and the mailbox keeps its
 * pending state, so whatever is already in the pipe predates this session and
 * would be consumed as part of the next frame. Call once after
 * bist_hd_transport_init() and before AGENT_READY: the companion only talks
 * after AGENT_READY, so nothing legitimate can be lost here.
 */
void bist_hd_transport_flush(void);

/**
 * @brief Send an encoded Host Diagnostics message.
 *
 * @param msg Message to send
 * @param timeout_ms Timeout in milliseconds (-1 = wait forever)
 * @return 0 on success, negative errno-style value on failure
 */
int bist_hd_transport_send(const bist_hd_msg_t *msg, int32_t timeout_ms);

/**
 * @brief Receive and decode a Host Diagnostics message.
 *
 * @param msg Output message
 * @param timeout_ms Timeout in milliseconds (-1 = wait forever)
 * @return 0 on success, negative errno-style value on failure
 */
int bist_hd_transport_recv(bist_hd_msg_t *msg, int32_t timeout_ms);

#ifdef __cplusplus
}
#endif

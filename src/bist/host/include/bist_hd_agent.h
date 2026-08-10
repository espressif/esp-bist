/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Host Diagnostic Agent API (HP / QM host OS).
 *
 * The companion (LP) owns safe-state decisions. This agent only collaborates:
 * answers Q&A challenges for now. Catalog DIAG audits and checkpoints are
 * deferred. This is not a Safety OS.
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the Host Diagnostic Agent.
 *
 * Initializes transport, sends AGENT_READY, and starts a high-priority
 * receive task. LP status messages are queued for
 * bist_hd_agent_wait_lp_status(). Q&A CHALLENGE frames are handled on the
 * worker path when CONFIG_ESP_BIST_HD_AUDIT_QA is enabled.
 *
 * @return 0 on success, negative errno-style value on failure
 */
int bist_hd_agent_start(void);

/**
 * @brief Wait for an LP BIST status word that includes @p expect_flag.
 *
 * @param status_out Receives the status bitmask
 * @param expect_flag Required flag bit (e.g. BIST_HD_BIT_POSTBOOT)
 * @param timeout_ms Timeout in milliseconds
 * @return 0 on success, negative errno-style value on failure / timeout
 */
int bist_hd_agent_wait_lp_status(uint32_t *status_out, uint32_t expect_flag, int32_t timeout_ms);

/**
 * @brief Report that a logical / alive checkpoint was reached.
 *
 * Stub until the checkpoint audit phase; always returns -1.
 *
 * @param checkpoint_id Application-defined checkpoint identifier
 * @return -1 (not implemented yet)
 */
int bist_hd_checkpoint_reached(uint32_t checkpoint_id);

#ifdef __cplusplus
}
#endif

/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Host Diagnostic Agent API (HP / QM host OS).
 *
 * The companion (LP) owns safe-state decisions. This agent collaborates:
 * answers Q&A challenges and reports alive checkpoints. Catalog DIAG audits
 * are deferred. This is not a Safety OS.
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
 * bist_hd_agent_wait_lp_status(). Q&A CHALLENGE and DIAG_REQ frames are
 * handled on the worker path.
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
 * Marks a pending checkpoint with an internally managed sequence counter.
 * The agent worker sends it to the LP companion on the next mailbox
 * opportunity (after receiving a CHALLENGE).  The companion tracks
 * received checkpoints and enters safe state if they stop arriving
 * within the configured deadline.
 *
 * Must be called periodically from the host main loop.  If the main
 * loop stops calling this function the pending counter stays stale,
 * no new checkpoints are sent, and the companion triggers safe state
 * (at most +1 companion-loop detection latency from the last call).
 *
 * @return 0 on success, -1 if checkpoint audit is disabled
 */
int bist_hd_checkpoint_reached(void);

#ifdef __cplusplus
}
#endif

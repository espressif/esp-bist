/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Host Diagnostics companion API (LP safety companion).
 *
 * Owns LP BIST gating, status reporting, and safe-state decisions.
 * This is not a Safety OS; LP-on-die is not a fully independent channel.
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief One-shot companion setup.
 *
 * Order: mailbox init → wait AGENT_READY → post-boot LP BIST → report
 * LP_STATUS → LP WDT init. Any failure after mailbox init enters
 * bist_hd_safe_state(), which arms the LP WDT so pre-handshake and post-boot
 * failures also fail closed. Returns so the application can own the main loop.
 *
 * @return 0 on success, -1 on failure (safe-state entered once mailbox is up)
 */
int bist_hd_companion_init(void);

/**
 * @brief One runtime companion iteration.
 *
 * Feeds the LP WDT, drains checkpoint messages, runs runtime LP BIST, reports
 * LP_STATUS, issues host Q&A challenge, executes host catalog audits
 * (DIAG_REQ/DIAG_RSP), and checks checkpoint deadlines. Returns so the
 * application can interleave its own logic.
 *
 * Once safe state is latched, never feeds the LP WDT; if
 * 2 * CONFIG_ESP_BIST_WDT_TIMEOUT_US elapses without a system reset (e.g. an
 * illegal HP warm reboot disarmed LP_WDT), re-arms LP_WDT to force a full
 * system reset.
 *
 * @return 0 on success / already in safe state, -1 on failure (safe-state entered)
 */
int bist_hd_companion_loop(void);

/**
 * @brief Enter product safe state.
 *
 * Default weak implementation latches safe state (one-way), records the entry
 * tick, unconditionally arms LP_WDT, and sends a bounded best-effort
 * SAFE_STATE_NOTIFY. Applications may override to drive GPIOs or other
 * cut-offs before the WDT reset.
 */
void bist_hd_safe_state(void);

#ifdef __cplusplus
}
#endif

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
 * LP_STATUS → LP WDT init. Returns so the application can own the main loop.
 *
 * @return 0 on success, -1 on failure (safe-state entered)
 */
int bist_hd_companion_init(void);

/**
 * @brief One runtime companion iteration.
 *
 * Feeds the LP WDT, runs runtime LP BIST, reports LP_STATUS, and (when
 * CONFIG_ESP_BIST_HD_AUDIT_QA is enabled) issues one host Q&A challenge.
 * Returns so the application can interleave its own logic. No-op once in
 * safe state.
 *
 * @return 0 on success / already in safe state, -1 on failure (safe-state entered)
 */
int bist_hd_companion_loop(void);

/**
 * @brief Enter product safe state.
 *
 * Default weak implementation marks the companion as failed. Applications may
 * override to drive GPIOs or other cut-offs.
 */
void bist_hd_safe_state(void);

#ifdef __cplusplus
}
#endif

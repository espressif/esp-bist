/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Host-side Q&A challenge handler (catalog DIAG audits deferred).
 */

#pragma once

#include "bist_hd_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Handle a companion CHALLENGE (Q&A audit).
 *
 * Computes f(challenge, seq) and sends ANSWER when CONFIG_ESP_BIST_HD_AUDIT_QA
 * is enabled.
 *
 * @return 0 on success, negative on transport / encode failure / audit disabled
 */
int bist_hd_audit_handle_challenge(const bist_hd_msg_t *challenge);

#ifdef __cplusplus
}
#endif

/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Host-side diagnostic audit handlers.
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
 * @param challenge Pointer to incoming CHALLENGE message
 * @return 0 on success, negative on transport / encode failure / audit disabled
 */
int bist_hd_audit_handle_challenge(const bist_hd_msg_t *challenge);

/**
 * @brief Handle a companion DIAG_REQ (host catalog audit).
 *
 * Dispatches by audit_id, executes the corresponding diagnostic (or returns
 * BIST_HD_STATUS_NOT_CONFIGURED if disabled/unknown), and sends DIAG_RSP.
 *
 * @param req Pointer to incoming DIAG_REQ message
 * @return 0 on success, negative on transport / encode failure
 */
int bist_hd_audit_handle_diag(const bist_hd_msg_t *req);

#ifdef __cplusplus
}
#endif

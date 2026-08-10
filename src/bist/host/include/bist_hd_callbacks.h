/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Application callbacks for Host Diagnostics audits that need product wiring.
 * If an audit is Kconfig-enabled without the required callback, the handler
 * reports BIST_HD_STATUS_NOT_CONFIGURED (companion enters safe state).
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @return BIST_HD_STATUS_OK / FAIL / NOT_CONFIGURED */
typedef uint32_t (*bist_hd_gpio_audit_fn_t)(void);
typedef uint32_t (*bist_hd_adc_audit_fn_t)(void);
typedef uint32_t (*bist_hd_config_nvm_audit_fn_t)(void);
typedef uint32_t (*bist_hd_secure_boot_audit_fn_t)(void);
typedef uint32_t (*bist_hd_dual_channel_audit_fn_t)(void);

typedef struct {
    bist_hd_gpio_audit_fn_t gpio;
    bist_hd_adc_audit_fn_t adc;
    bist_hd_config_nvm_audit_fn_t config_nvm;
    bist_hd_secure_boot_audit_fn_t secure_boot;
    bist_hd_dual_channel_audit_fn_t dual_channel;
} bist_hd_callbacks_t;

/**
 * @brief Register (or clear with NULL fields) application audit callbacks.
 */
void bist_hd_register_callbacks(const bist_hd_callbacks_t *cbs);

/**
 * @brief Return the currently registered callbacks (never NULL; fields may be).
 */
const bist_hd_callbacks_t *bist_hd_get_callbacks(void);

#ifdef __cplusplus
}
#endif

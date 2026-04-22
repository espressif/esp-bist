/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Bare-metal stubs for symbols normally provided by ROM or optional IDF
 * components. Allows linking without full IDF/ROM when running BIST only.
 */

#include <stdint.h>
#include <stdbool.h>
#include "sdkconfig.h"

/* -------------------------------------------------------------------------
 * Modem clock (optional for BIST). Stubs so rtc_clk and clock_init link.
 * ------------------------------------------------------------------------- */
#include "esp_private/periph_ctrl.h"
#include "esp_private/esp_modem_clock.h"

__attribute__((weak)) void modem_clock_module_enable(shared_periph_module_t module)
{
    (void)module;
}

__attribute__((weak)) void modem_clock_module_disable(shared_periph_module_t module)
{
    (void)module;
}

/* -------------------------------------------------------------------------
 * Clock tree enable/disable (used by rtc_clk.c in new IDF).
 * In bare-metal BIST, no dynamic clock tree management is needed.
 * ------------------------------------------------------------------------- */
#include "esp_err.h"
#include "soc/clk_tree_defs.h"

__attribute__((weak)) esp_err_t esp_clk_tree_enable_src(soc_module_clk_t clk_src, bool enable)
{
    (void)clk_src;
    (void)enable;
    return ESP_OK;
}

/* -------------------------------------------------------------------------
 * Modem clock LP source deselect (used by IDF clk.c if it gets compiled).
 * ------------------------------------------------------------------------- */
__attribute__((weak)) void modem_clock_deselect_all_module_lp_clock_source(void)
{
}


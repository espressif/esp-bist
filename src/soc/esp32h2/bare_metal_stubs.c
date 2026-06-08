/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Bare-metal stubs for symbols normally provided by ROM or optional IDF
 * components. Allows linking without full IDF/ROM when running BIST only.
 */

#include <stdint.h>
#include <stdbool.h>

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

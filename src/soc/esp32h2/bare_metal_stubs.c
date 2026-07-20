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
#include <stdio.h>
#include "esp_log_level.h"
#include "esp_private/log_print.h"

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

bool esp_log_is_tag_loggable(esp_log_level_t level, const char *tag)
{
    (void)level;
    (void)tag;
    return true;
}

char *esp_log_system_timestamp(void)
{
    static char buffer = ' ';
    return &buffer;
}

vprintf_like_t esp_log_vprint_func = (vprintf_like_t)esp_rom_printf;

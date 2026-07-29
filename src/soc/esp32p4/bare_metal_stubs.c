/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
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

/* -------------------------------------------------------------------------
 * ESP32-P4 ROM does not export spi_flash_cache_enabled(), which esp_err.c
 * queries before printing strings that live in the flash cache. BIST images
 * always run with the flash cache mapped by the loader.
 * ------------------------------------------------------------------------- */
__attribute__((weak)) bool spi_flash_cache_enabled(void)
{
    return true;
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

vprintf_like_t esp_log_vprint_func = &vprintf;

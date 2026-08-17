/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * This file is part of Espressif's BIST (Built-In Self Test) Library.
 *
 * BIST library is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * BIST library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with BIST library. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "bist_wdt.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_cpu.h"
#include "rom/ets_sys.h"
#include "hal/wdt_hal.h"
#include "esp_private/periph_ctrl.h"
#include "esp_rom_sys.h"
#include "wdt.h"

#ifdef SOC_TARGET_ESP32P4
#define RESET_REASON_CORE RESET_REASON_CORE_MWDT
#else
#define RESET_REASON_CORE RESET_REASON_CORE_MWDT0
#endif

bist_esp_err_t bist_wdt_test(void)
{
    volatile uint32_t wdt_timeout_us = 10000;
    soc_reset_reason_t reset = esp_rom_get_reset_reason(0);

    if (reset == RESET_REASON_CORE) {
        return BIST_ESP_OK;
    }

    BIST_ADD_LABEL("bist_test_wdt_timeout");

    wdt_init(wdt_timeout_us);

    /* let watchdog callback happen or timeout */
    ets_delay_us(50000);

    wdt_deinit();

    return BIST_ESP_WDT_TEST_ERR;
}

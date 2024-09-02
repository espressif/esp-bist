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
#include "esp32c3/rtc.h"
#include "esp_rom_sys.h"
#include "wdt.h"

bist_esp_err_t bist_wdt_test(void)
{
    uint32_t wdt_timeout_ms = 100;
    soc_reset_reason_t reset = esp_rom_get_reset_reason(0);

    if (reset == RESET_REASON_CORE_MWDT0) {
        return BIST_ESP_OK;
    }

    BIST_ADD_LABEL("bist_test_wdt_timeout");

    wdt_init(wdt_timeout_ms);

    /* let watchdog callback happen or timeout */
    /* code below only works with real device */
    // uint32_t now = esp_rtc_get_time_us() + 1000000;
    // while (esp_rtc_get_time_us() < now) {
    //     ets_delay_us(100000);
    // }

    /* let watchdog callback happen or timeout */
    for (uint32_t k = 0; k < 1000; k++) {
        ets_delay_us(1000);
    }

    wdt_deinit();

    return BIST_ESP_WDT_TEST_ERR;
}

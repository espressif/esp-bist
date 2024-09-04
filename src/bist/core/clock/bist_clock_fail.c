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

#include "bist_clock_fail.h"
#include "esp_xt_wdt.h"
#include "esp_attr.h"
#include "soc/rtc.h"
#include "bist_log.h"
#include "math.h"
#include "rom/ets_sys.h"

static volatile bool test_failed = false;
static const char *TAG = "BIST_CLOCK";

static IRAM_ATTR void test_callback(void *arg)
{
    test_failed = true;
}

bist_esp_err_t bist_ext_crystal_fail_test(void)
{
    esp_err_t err;

    esp_xt_wdt_config_t cfg = {
        .timeout = 200,
        .auto_backup_clk_enable = false,
    };

    esp_xt_wdt_register_callback(test_callback, NULL);

    err = esp_xt_wdt_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize external watchdog timer");
        return BIST_ESP_CLOCK_TEST_ERR;
    }

    for (uint32_t k = 0; k < 500; k++) {
        ets_delay_us(1000);
    }

    if (test_failed) {
        return BIST_ESP_CLOCK_TEST_ERR;
    }

    return BIST_ESP_OK;
}

bist_esp_err_t bist_main_crystal_test(void)
{
    uint32_t expected_xtal_freq = rtc_clk_xtal_freq_get() * MHZ;

    /*
     * Returns the number of main XTAL cycles in one 32kHz XTAL cycle
     */
    uint32_t clk_ratio = rtc_clk_cal_ratio(RTC_CAL_32K_XTAL, 500);

    if (clk_ratio == 0) {
        return BIST_ESP_CLOCK_TEST_ERR;
    }

    /*
     * clk_ratio contains period of 32768 Hz clock in XTAL clock cycles
     * (shifted by RTC_CLK_CAL_FRACT bits).
     * Xtal frequency will be (clk_ratio / 2^19) * 32768
     */
    uint32_t xtal_freq = (clk_ratio >> RTC_CLK_CAL_FRACT) * 32768;
    float deviation = fabs((float)(int)(xtal_freq - expected_xtal_freq)) / expected_xtal_freq * 100;

    /*
     * Check if the calculated XTAL frequency is within
     * CONFIG_BIST_CLOCK_PERCENT_FREQUENCY_DRIFT
     */
    if (deviation > CONFIG_BIST_CLOCK_PERCENT_FREQUENCY_DRIFT) {
        return BIST_ESP_CLOCK_TEST_ERR;
    }

    return BIST_ESP_OK;
}

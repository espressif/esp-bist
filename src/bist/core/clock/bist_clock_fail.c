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
#include "soc/soc_caps.h"
#include "esp_attr.h"
#include "soc/rtc.h"
#include "bist_log.h"
#include "math.h"
#include "rom/ets_sys.h"
#include "bist_conf.h"

#if SOC_XT_WDT_SUPPORTED
#include "esp_xt_wdt.h"
static volatile bool test_failed = false;
#endif
static const char *TAG = "BIST_CLOCK";

#if SOC_XT_WDT_SUPPORTED
static void test_callback(void *arg)
{
    test_failed = true;
}
#endif

bist_esp_err_t bist_ext_crystal_fail_test(void)
{
#if SOC_XT_WDT_SUPPORTED
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

    ets_delay_us(2000);

    if (test_failed) {
        test_failed = false;
        return BIST_ESP_CLOCK_TEST_ERR;
    }

    return BIST_ESP_OK;
#else
    /* SoCs without XT WDT have no hardware to detect 32k crystal
     * failure; skip this test and report OK. */
    ESP_LOGD(TAG, "SoCs without XT WDT have no hardware to detect 32k crystal failure; skipping test.");
    return BIST_ESP_OK;
#endif
}

bist_esp_err_t bist_main_crystal_test(void)
{
    uint32_t xtal_freq_mhz = rtc_clk_xtal_freq_get();
    uint32_t expected_xtal_freq = xtal_freq_mhz * MHZ;

    /*
     * rtc_clk_cal returns the 32K XTAL clock period in fixed-point format (Q13.19).
     * If the main XTAL drifts, the returned value shifts proportionally.
     */
    uint32_t cal_val = rtc_clk_cal(CLK_CAL_32K_XTAL, 500);
    if (cal_val == 0) {
        return BIST_ESP_CLOCK_TEST_ERR;
    }

    /*
     * Derive actual XTAL frequency from the calibration result:
     */
    uint32_t xtal_freq = (uint32_t)(((uint64_t)cal_val * xtal_freq_mhz * 32768) >> RTC_CLK_CAL_FRACT);
    float deviation = fabs((float)(int)(xtal_freq - expected_xtal_freq)) / expected_xtal_freq * 100;

    /*
     * Check if the calculated XTAL frequency is within
     * CONFIG_ESP_BIST_CLOCK_PERCENT_FREQUENCY_DRIFT
     */
    if (deviation > CONFIG_ESP_BIST_CLOCK_PERCENT_FREQUENCY_DRIFT) {
        return BIST_ESP_CLOCK_TEST_ERR;
    }

    return BIST_ESP_OK;
}

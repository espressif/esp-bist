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

#include "esp_xt_wdt.h"
#include "sdkconfig.h"
#include "soc/soc_caps.h"

#include "esp_log.h"
#include "esp_check.h"
#include "esp_attr.h"
#include "esp_intr_alloc.h"

#if SOC_XT_WDT_SUPPORTED

#include "esp_private/rtc_ctrl.h"
#include "hal/xt_wdt_hal.h"
#include "hal/xt_wdt_ll.h"
#include "soc/rtc.h"

#include "esp_cpu.h"
#include "soc/interrupts.h"

#define RTC_CLK_CAL_CYCLES 500

const static char *TAG = "xt_wdt";

static xt_wdt_hal_context_t s_hal_ctx;

static esp_xt_callback_t s_callback_func;
static void *s_callback_arg;

static IRAM_ATTR void rtc_xt_wdt_default_isr_handler(void *arg)
{
    ESP_EARLY_LOGE(TAG, "XTAL32K watchdog timer got triggered");

    uint32_t status = REG_READ(RTC_CNTL_INT_ST_REG);
    REG_WRITE(RTC_CNTL_INT_CLR_REG, status);

    if (s_callback_func) {
        (*s_callback_func)(s_callback_arg);
    }
}

esp_err_t esp_xt_wdt_init(const esp_xt_wdt_config_t *cfg)
{
    xt_wdt_hal_config_t hal_config = {
        .timeout = cfg->timeout,
    };

    xt_wdt_hal_init(&s_hal_ctx, &hal_config);

    if (cfg->auto_backup_clk_enable) {
        /* Estimate frequency of internal RTC oscillator */
        uint32_t rtc_clk_frequency_khz = rtc_clk_freq_cal(rtc_clk_cal(RTC_CAL_INTERNAL_OSC, RTC_CLK_CAL_CYCLES)) / 1000;
        ESP_EARLY_LOGD(TAG, "Calibrating backup clock from rtc clock with frequency %" PRIu32, rtc_clk_frequency_khz);

        xt_wdt_hal_enable_backup_clk(&s_hal_ctx, rtc_clk_frequency_khz);
    }

    esp_cpu_intr_disable(1 << ETS_RTC_CORE_INTR_SOURCE);
    esp_rom_route_intr_matrix(esp_cpu_get_core_id(), ETS_RTC_CORE_INTR_SOURCE, ETS_RTC_CORE_INTR_SOURCE);

    esp_cpu_intr_set_type(ETS_RTC_CORE_INTR_SOURCE, 0);
    esp_cpu_intr_set_priority(ETS_RTC_CORE_INTR_SOURCE, SOC_INTERRUPT_LEVEL_MEDIUM);
    esp_cpu_intr_set_handler(ETS_RTC_CORE_INTR_SOURCE, rtc_xt_wdt_default_isr_handler, NULL);
    esp_cpu_intr_enable(1 << ETS_RTC_CORE_INTR_SOURCE);

    xt_wdt_hal_enable(&s_hal_ctx, 1);

    return ESP_OK;
}

void esp_xt_wdt_restore_clk(void)
{
    xt_wdt_hal_enable(&s_hal_ctx, false);

    REG_CLR_BIT(RTC_CNTL_EXT_XTL_CONF_REG, RTC_CNTL_XPD_XTAL_32K);
    REG_SET_BIT(RTC_CNTL_EXT_XTL_CONF_REG, RTC_CNTL_XPD_XTAL_32K);

    /* Needs some time after switching to 32khz XTAL before turning on WDT again */
    esp_rom_delay_us(300);

    xt_wdt_hal_enable(&s_hal_ctx, true);
}

void esp_xt_wdt_register_callback(esp_xt_callback_t func, void *arg)
{
    s_callback_func = func;
    s_callback_arg = arg;
}

#endif // SOC_XT_WDT_SUPPORTED

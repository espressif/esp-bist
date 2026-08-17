/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ESP32-P4 clock init aligned with IDF esp32p4/clk.c for MCUboot boot path.
 * Bootloader already runs rtc_clk_init(); app calls esp_rtc_init() then esp_clk_init().
 */

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_clk_internal.h"
#include "esp_cpu.h"
#include "esp_rom_serial_output.h"
#include "esp_rom_sys.h"
#include "esp_private/esp_clk.h"
#include "esp_private/esp_pmu.h"
#include "hal/clk_gate_ll.h"
#include "hal/wdt_hal.h"
#include "soc/clk_tree_defs.h"
#include "soc/rtc.h"
#include "soc/soc.h"

/* Stub for esp_sleep_pd_config (sleep power domain mgmt not used in BIST). */
#include "esp_sleep.h"
#include "esp_err.h"
esp_err_t esp_sleep_pd_config(esp_sleep_pd_domain_t domain, esp_sleep_pd_option_t option)
{
    (void)domain;
    (void)option;
    return ESP_OK;
}

#define SLOW_CLK_CAL_CYCLES  CONFIG_RTC_CLK_CAL_CYCLES
#define MHZ                  (1000000)

static const char *TAG = "clk";

static void select_rtc_slow_clk(soc_rtc_slow_clk_src_t rtc_slow_clk_src);

void IRAM_ATTR esp_rtc_init(void)
{
#if SOC_PMU_SUPPORTED
    pmu_init();
#endif
}

void esp_clk_init(void)
{
    assert(rtc_clk_xtal_freq_get() == SOC_XTAL_FREQ_40M);

    rtc_clk_8m_enable(true);
#if CONFIG_RTC_FAST_CLK_SRC_RC_FAST
    rtc_clk_fast_src_set(SOC_RTC_FAST_CLK_SRC_RC_FAST);
#elif CONFIG_RTC_FAST_CLK_SRC_XTAL
    rtc_clk_fast_src_set(SOC_RTC_FAST_CLK_SRC_XTAL);
    if (esp_sleep_sub_mode_dump_config(NULL)[ESP_SLEEP_RTC_FAST_USE_XTAL_MODE] == 0) {
        esp_sleep_sub_mode_config(ESP_SLEEP_RTC_FAST_USE_XTAL_MODE, true);
    }
#else
#  error "No RTC fast clock source configured"
#endif

#if defined(CONFIG_RTC_CLK_SRC_EXT_CRYS)
    select_rtc_slow_clk(SOC_RTC_SLOW_CLK_SRC_XTAL32K);
#else
    select_rtc_slow_clk(SOC_RTC_SLOW_CLK_SRC_RC_SLOW);
#endif

    rtc_cpu_freq_config_t old_config, new_config;
    rtc_clk_cpu_freq_get_config(&old_config);
    const uint32_t old_freq_mhz = old_config.freq_mhz;
    const uint32_t new_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;

    bool res = rtc_clk_cpu_freq_mhz_to_config(new_freq_mhz, &new_config);
    assert(res);

    esp_rom_output_tx_wait_idle(CONFIG_ESP_CONSOLE_UART_NUM);

    if (res) {
        rtc_clk_cpu_freq_set_config(&new_config);
    }

    esp_cpu_set_cycle_count((uint64_t)esp_cpu_get_cycle_count() * new_freq_mhz / old_freq_mhz);
}

static void select_rtc_slow_clk(soc_rtc_slow_clk_src_t rtc_slow_clk_src)
{
    uint32_t cal_val = 0;
    int retry_32k_xtal = 3;

    soc_rtc_slow_clk_src_t old_rtc_slow_clk_src = rtc_clk_slow_src_get();
    do {
        bool revoke_32k_enable = false;
        if (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_XTAL32K) {
            ESP_EARLY_LOGD(TAG, "waiting for 32k oscillator to start up");
            soc_clk_freq_calculation_src_t cal_sel = CLK_CAL_32K_XTAL;
            rtc_clk_32k_enable(true);
            if (SLOW_CLK_CAL_CYCLES > 0) {
                cal_val = rtc_clk_cal(cal_sel, SLOW_CLK_CAL_CYCLES);
                if (cal_val == 0) {
                    if (retry_32k_xtal-- > 0) {
                        continue;
                    }
                    ESP_EARLY_LOGW(TAG, "32 kHz clock not found, switching to internal 150 kHz oscillator");
                    rtc_slow_clk_src = SOC_RTC_SLOW_CLK_SRC_RC_SLOW;
                    revoke_32k_enable = true;
                }
            }
        } else if (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_RC32K) {
            rtc_clk_rc32k_enable(true);
        }
        rtc_clk_slow_src_set(rtc_slow_clk_src);

        if (revoke_32k_enable ||
                ((old_rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_XTAL32K) && rtc_slow_clk_src != SOC_RTC_SLOW_CLK_SRC_XTAL32K)) {
            rtc_clk_32k_enable(false);
        }
        if (rtc_slow_clk_src != SOC_RTC_SLOW_CLK_SRC_RC32K) {
            rtc_clk_rc32k_enable(false);
        }

        pmu_lp_power_t lp_clk_power = {
            .xpd_xtal32k = (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_XTAL32K),
            .xpd_rc32k = (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_RC32K),
            .xpd_fosc = 1,
            .pd_osc = 0
        };
        pmu_ll_lp_set_clk_power(&PMU, PMU_MODE_LP_ACTIVE, lp_clk_power.val);

        if (SLOW_CLK_CAL_CYCLES > 0) {
            cal_val = rtc_clk_cal(CLK_CAL_RTC_SLOW, SLOW_CLK_CAL_CYCLES);
        } else {
            const uint64_t cal_dividend = (1ULL << RTC_CLK_CAL_FRACT) * 1000000ULL;
            cal_val = (uint32_t)(cal_dividend / rtc_clk_slow_freq_get_hz());
        }
    } while (cal_val == 0);
    ESP_EARLY_LOGI(TAG, "RTC_SLOW_CLK calibration value: %d", cal_val);
    esp_clk_slowclk_cal_set(cal_val);
}

void esp_perip_clk_init(void)
{
    soc_reset_reason_t rst_reason = esp_rom_get_reset_reason(0);
    periph_ll_clk_gate_config_t clk_gate_config = {0};

    periph_ll_clk_gate_set_default(rst_reason, &clk_gate_config);
}

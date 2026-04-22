/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ESP32-C6 clock init: esp_clk_init() and esp_perip_clk_init() reimplemented
 * using only RTOS-agnostic IDF components (soc, hal, esp_rom, esp_hw_support).
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
#include "hal/clk_gate_ll.h"
#include "esp_private/esp_modem_clock.h"
#include "esp_private/esp_pmu.h"
#include "soc/clk_tree_defs.h"
#include "soc/rtc.h"
#include "soc/soc.h"
#include "ocode_init.h"

/* Stub for modem_clock_select_lp_clock_source (power domain not used in BIST). */
/* Must match IDF esp_sleep.h declaration to avoid conflicting types. */
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

static rtc_clk_config_t get_bist_rtc_clk_config(void)
{
    rtc_clk_config_t cfg = (rtc_clk_config_t) {
        .xtal_freq = CONFIG_XTAL_FREQ,
        .cpu_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
        .fast_clk_src = SOC_RTC_FAST_CLK_SRC_RC_FAST,
        .slow_clk_src = SOC_RTC_SLOW_CLK_SRC_RC_SLOW,
        .clk_rtc_clk_div = 0,
        .clk_8m_clk_div = 0,
        .slow_clk_dcap = RTC_CNTL_SCK_DCAP_DEFAULT,
        .clk_8m_dfreq = RTC_CNTL_CK8M_DFREQ_DEFAULT,
        .rc32k_dfreq = RTC_CNTL_RC32K_DFREQ_DEFAULT,
    };

    return cfg;
}

void esp_clk_init(void)
{
    pmu_init();

    /* Run full RTC clock init (normally done by bootloader) since BIST runs standalone */
    rtc_clk_init(get_bist_rtc_clk_config());

    if (esp_rom_get_reset_reason(0) == RESET_REASON_CHIP_POWER_ON) {
        esp_ocode_calib_init();
    }

    assert(rtc_clk_xtal_freq_get() == RTC_XTAL_FREQ_40M);

    rtc_clk_8m_enable(true);
    rtc_clk_fast_src_set(SOC_RTC_FAST_CLK_SRC_RC_FAST);


#if defined(CONFIG_RTC_CLK_SRC_EXT_CRYS)
    select_rtc_slow_clk(SOC_RTC_SLOW_CLK_SRC_XTAL32K);
#elif defined(CONFIG_RTC_CLK_SRC_EXT_OSC)
    select_rtc_slow_clk(SOC_RTC_SLOW_CLK_SRC_OSC_SLOW);
#elif defined(CONFIG_RTC_CLK_SRC_INT_RC32K)
    select_rtc_slow_clk(SOC_RTC_SLOW_CLK_SRC_RC32K);
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

    do {
        if (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_XTAL32K || rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_OSC_SLOW) {
            ESP_EARLY_LOGI(TAG, "waiting for 32k oscillator to start up");
            soc_clk_freq_calculation_src_t cal_sel = CLK_CAL_RC_SLOW;
            if (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_XTAL32K) {
                rtc_clk_32k_enable(true);
                cal_sel = CLK_CAL_32K_XTAL;
            } else if (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_OSC_SLOW) {
                rtc_clk_32k_enable_external();
                cal_sel = CLK_CAL_32K_OSC_SLOW;
            }
            if (SLOW_CLK_CAL_CYCLES > 0) {
                cal_val = rtc_clk_cal(cal_sel, SLOW_CLK_CAL_CYCLES);
                if (cal_val == 0) {
                    if (retry_32k_xtal-- > 0) {
                        continue;
                    }
                    ESP_EARLY_LOGW(TAG, "32 kHz clock not found, switching to internal 150 kHz oscillator");
                    rtc_slow_clk_src = SOC_RTC_SLOW_CLK_SRC_RC_SLOW;
                }
            }
        } else if (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_RC32K) {
            rtc_clk_rc32k_enable(true);
        }
        rtc_clk_slow_src_set(rtc_slow_clk_src);

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

    periph_ll_enable_clk_clear_rst(PERIPH_TIMG0_MODULE);
}

/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ESP32-C3 clock init: esp_clk_init() and esp_perip_clk_init() reimplemented
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
#include "esp_private/periph_ctrl.h"
#include "soc/rtc.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/reset_reasons.h"

#define SLOW_CLK_CAL_CYCLES  CONFIG_RTC_CLK_CAL_CYCLES
#define MHZ                  (1000000)
#define EXT_OSC_FLAG         BIT(3)

/*
 * Slow-clock selection enum — mirrors IDF clk.c for ESP32-C3.
 * Lower 2 bits correspond to soc_rtc_slow_clk_src_t values.
 */
typedef enum {
    SLOW_CLK_RTC          = SOC_RTC_SLOW_CLK_SRC_RC_SLOW,
    SLOW_CLK_32K_XTAL     = SOC_RTC_SLOW_CLK_SRC_XTAL32K,
    SLOW_CLK_8MD256       = SOC_RTC_SLOW_CLK_SRC_RC_FAST_D256,
    SLOW_CLK_32K_EXT_OSC  = SOC_RTC_SLOW_CLK_SRC_XTAL32K | EXT_OSC_FLAG
} slow_clk_sel_t;

static const char *TAG = "clk";

static void select_rtc_slow_clk(slow_clk_sel_t slow_clk);

static void setup_clock_glitch_reset(bool enable)
{
    REG_CLR_BIT(RTC_CNTL_FIB_SEL_REG, RTC_CNTL_FIB_GLITCH_RST);

    if (enable) {
        REG_SET_BIT(RTC_CNTL_ANA_CONF_REG, RTC_CNTL_GLITCH_RST_EN);
    } else {
        REG_CLR_BIT(RTC_CNTL_ANA_CONF_REG, RTC_CNTL_GLITCH_RST_EN);
    }
}

void esp_clk_init(void)
{
    /* Run full RTC init (normally done by bootloader) since BIST runs standalone */
    rtc_config_t cfg = RTC_CONFIG_DEFAULT();
    soc_reset_reason_t rst_reas = esp_rom_get_reset_reason(0);
    if (rst_reas == RESET_REASON_CHIP_POWER_ON) {
        cfg.cali_ocode = 1;
    }
    rtc_init(cfg);

    setup_clock_glitch_reset(true);

    assert(rtc_clk_xtal_freq_get() == SOC_XTAL_FREQ_40M);

    bool rc_fast_d256_is_enabled = rtc_clk_8md256_enabled();
    rtc_clk_8m_enable(true, rc_fast_d256_is_enabled);
    rtc_clk_fast_src_set(SOC_RTC_FAST_CLK_SRC_RC_FAST);

#if defined(CONFIG_RTC_CLK_SRC_EXT_CRYS)
    select_rtc_slow_clk(SLOW_CLK_32K_XTAL);
#elif defined(CONFIG_RTC_CLK_SRC_EXT_OSC)
    select_rtc_slow_clk(SLOW_CLK_32K_EXT_OSC);
#elif defined(CONFIG_RTC_CLK_SRC_INT_8MD256)
    select_rtc_slow_clk(SLOW_CLK_8MD256);
#else
    select_rtc_slow_clk(SLOW_CLK_RTC);
#endif

    rtc_cpu_freq_config_t old_config, new_config;
    rtc_clk_cpu_freq_get_config(&old_config);
    const uint32_t old_freq_mhz = old_config.freq_mhz;
    const uint32_t new_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;

    bool res = rtc_clk_cpu_freq_mhz_to_config(new_freq_mhz, &new_config);
    assert(res);

    esp_rom_output_tx_wait_idle(CONFIG_ESP_CONSOLE_ROM_SERIAL_PORT_NUM);

    if (res) {
        rtc_clk_cpu_freq_set_config(&new_config);
    }

    esp_cpu_set_cycle_count((uint64_t)esp_cpu_get_cycle_count() * new_freq_mhz / old_freq_mhz);
}

static void select_rtc_slow_clk(slow_clk_sel_t slow_clk)
{
    soc_rtc_slow_clk_src_t rtc_slow_clk_src = slow_clk & RTC_CNTL_ANA_CLK_RTC_SEL_V;
    uint32_t cal_val = 0;
    int retry_32k_xtal = 3;

    soc_rtc_slow_clk_src_t old_rtc_slow_clk_src = rtc_clk_slow_src_get();
    do {
        bool revoke_32k_enable = false;
        if (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_XTAL32K) {
            ESP_EARLY_LOGD(TAG, "waiting for 32k oscillator to start up");
            if (slow_clk == SLOW_CLK_32K_XTAL) {
                rtc_clk_32k_enable(true);
            } else if (slow_clk == SLOW_CLK_32K_EXT_OSC) {
                rtc_clk_32k_enable_external();
            }
            if (SLOW_CLK_CAL_CYCLES > 0) {
                cal_val = rtc_clk_cal(CLK_CAL_32K_XTAL, SLOW_CLK_CAL_CYCLES);
                if (cal_val == 0) {
                    if (retry_32k_xtal-- > 0) {
                        continue;
                    }
                    ESP_EARLY_LOGW(TAG, "32 kHz XTAL not found, switching to internal 150 kHz oscillator");
                    rtc_slow_clk_src = SOC_RTC_SLOW_CLK_SRC_RC_SLOW;
                    revoke_32k_enable = true;
                }
            }
        } else if (rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_RC_FAST_D256) {
            rtc_clk_8m_enable(true, true);
        }
        rtc_clk_slow_src_set(rtc_slow_clk_src);
        if (revoke_32k_enable ||
                ((old_rtc_slow_clk_src == SOC_RTC_SLOW_CLK_SRC_XTAL32K) && (rtc_slow_clk_src != SOC_RTC_SLOW_CLK_SRC_XTAL32K))) {
            rtc_clk_32k_enable(false);
            rtc_clk_32k_disable_external();
        }
        if (SLOW_CLK_CAL_CYCLES > 0) {
            cal_val = rtc_clk_cal(CLK_CAL_RTC_SLOW, SLOW_CLK_CAL_CYCLES);
        } else {
            const uint64_t cal_dividend = (1ULL << RTC_CLK_CAL_FRACT) * 1000000ULL;
            cal_val = (uint32_t)(cal_dividend / rtc_clk_slow_freq_get_hz());
        }
    } while (cal_val == 0);
    ESP_EARLY_LOGD(TAG, "RTC_SLOW_CLK calibration value: %" PRIu32, cal_val);
    esp_clk_slowclk_cal_set(cal_val);
}

void esp_perip_clk_init(void)
{
    periph_module_enable(PERIPH_TIMG0_MODULE);
    periph_module_enable(PERIPH_RNG_MODULE);
}

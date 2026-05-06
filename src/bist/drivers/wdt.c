/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * This file is part of Espressif's BIST (Built-In Self Test) Library.
 *
 * BIST library is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * BIST library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with BIST library. If not, see
 * <https://www.gnu.org/licenses/lgpl-3.0.html>.
 */

#include "sdkconfig.h"
#include "esp_memory_utils.h"
#include "bist_log.h"
#include "esp_attr.h"
#include "esp_cpu.h"
#include "rom/ets_sys.h"
#include "hal/wdt_hal.h"
#include "bist_esp_types.h"
#include "esp_timer.h"
#include "hal/clk_tree_ll.h"
#include "esp_private/periph_ctrl.h"
#include "soc/system_intr.h"

#define MWDT_DEFAULT_TICKS_PER_US       500

static const char *TAG = "WDT";

struct wdt_context {
    wdt_hal_context_t wdt_hal_ctx;
    uint32_t overflow_timeout;
    uint32_t underflow_timeout;
    bool is_windowed;
    void (*callback)(void *);
    void *arg;
    volatile bool window_open_flag;
    volatile bool stop_feed;
};

static struct wdt_context wdt_ctx = {0};

static esp_timer_handle_t periodic_timer;

void IRAM_ATTR mwdt_default_callback(void *args)
{
    ESP_LOGE(TAG, "WDT timeout");
    wdt_hal_write_protect_disable(&wdt_ctx.wdt_hal_ctx);
    mwdt_ll_clear_intr_status(wdt_ctx.wdt_hal_ctx.mwdt_dev);
    wdt_hal_write_protect_enable(&wdt_ctx.wdt_hal_ctx);

    if (wdt_ctx.callback) {
        wdt_ctx.callback(wdt_ctx.arg);
    }
}

void wdt_windowed_underflow_callback(void *arg)
{
    (void)arg;

    /* Set window flag to allow feeding */
    wdt_ctx.window_open_flag = true;
}

void wdt_deinit(void)
{
    wdt_hal_write_protect_disable(&wdt_ctx.wdt_hal_ctx);
    wdt_hal_disable(&wdt_ctx.wdt_hal_ctx);
    wdt_hal_write_protect_enable(&wdt_ctx.wdt_hal_ctx);
    wdt_hal_deinit(&wdt_ctx.wdt_hal_ctx);
}

int wdt_init(uint32_t timeout_us)
{
    if (timeout_us < MWDT_DEFAULT_TICKS_PER_US) {
        ESP_LOGE(TAG, "Timeout %lu us too small (minimum %lu us)",
                 (unsigned long)timeout_us, (unsigned long)MWDT_DEFAULT_TICKS_PER_US);
        return -1;
    }

    uint32_t stage_timeout_ticks = timeout_us / MWDT_DEFAULT_TICKS_PER_US;

    ESP_LOGI(TAG, "Enabling WDT(%lu us)", (unsigned long)timeout_us);
    ESP_LOGI(TAG, "WDT prescaler: %u", MWDT_LL_DEFAULT_CLK_PRESCALER);
    ESP_LOGI(TAG, "Stage timeout ticks: %u", stage_timeout_ticks);

    /* Guard avoids -Wdeprecated-declarations on SoCs where IDF retired
     * the legacy API (C5/C61/...); TIMG0 is on by reset default there. */
#ifdef __PERIPH_CTRL_ALLOW_LEGACY_API
    periph_module_enable(PERIPH_TIMG0_MODULE);
#endif

    esp_cpu_intr_disable(1 << ETS_INT_WDT_INUM);
    esp_rom_route_intr_matrix(esp_cpu_get_core_id(), SYS_TG0_WDT_INTR_SOURCE, ETS_INT_WDT_INUM);

    esp_cpu_intr_set_type(ETS_INT_WDT_INUM, 0);
    esp_cpu_intr_set_priority(ETS_INT_WDT_INUM, SOC_INTERRUPT_LEVEL_MEDIUM);
    esp_cpu_intr_set_handler(ETS_INT_WDT_INUM, mwdt_default_callback, NULL);
    esp_cpu_intr_enable(1 << ETS_INT_WDT_INUM);

    wdt_hal_init(&wdt_ctx.wdt_hal_ctx, WDT_MWDT0, MWDT_LL_DEFAULT_CLK_PRESCALER, true);
    wdt_hal_write_protect_disable(&wdt_ctx.wdt_hal_ctx);
    wdt_hal_config_stage(&wdt_ctx.wdt_hal_ctx, WDT_STAGE0, stage_timeout_ticks, WDT_STAGE_ACTION_INT);
    wdt_hal_config_stage(&wdt_ctx.wdt_hal_ctx, WDT_STAGE1, stage_timeout_ticks * 2, WDT_STAGE_ACTION_RESET_SYSTEM);
    wdt_hal_enable(&wdt_ctx.wdt_hal_ctx);
    wdt_hal_write_protect_enable(&wdt_ctx.wdt_hal_ctx);

    return 0;
}

void wdt_feed(void)
{
    if (wdt_ctx.stop_feed) {
        return;
    }

    if(wdt_ctx.is_windowed && !wdt_ctx.window_open_flag) {
        ESP_LOGE(TAG, "WDT underflow detected: feeding is not allowed (min required = %u us)",
                 wdt_ctx.underflow_timeout);
        wdt_ctx.stop_feed = true;
        return;
    }

    wdt_hal_write_protect_disable(&wdt_ctx.wdt_hal_ctx);
    wdt_hal_feed(&wdt_ctx.wdt_hal_ctx);
    wdt_hal_write_protect_enable(&wdt_ctx.wdt_hal_ctx);

    if (wdt_ctx.is_windowed) {
        wdt_ctx.window_open_flag = false;
        /* Stop timer if running, then start fresh */
        esp_timer_stop(periodic_timer);  /* Ignore error if not running */
        esp_err_t ret = esp_timer_start_once(periodic_timer, wdt_ctx.underflow_timeout);
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "Failed to start windowed timer: %d", ret);
        }
    }
}

void wdt_register_callback(void (*callback)(void *), void *arg)
{
    if (callback == NULL) {
        ESP_LOGE(TAG, "WDT Callback function cannot be NULL");
        return;
    }

    if (!esp_ptr_in_iram(callback)) {
        ESP_LOGE(TAG, "WDT Callback function must be in IRAM");
        return;
    }

    wdt_ctx.callback = callback;
    wdt_ctx.arg = arg;

    wdt_hal_write_protect_disable(&wdt_ctx.wdt_hal_ctx);
    esp_cpu_intr_set_handler(ETS_INT_WDT_INUM, mwdt_default_callback, wdt_ctx.arg);
    wdt_hal_write_protect_enable(&wdt_ctx.wdt_hal_ctx);
}

void wdt_windowed_deinit(void)
{
    if (wdt_ctx.is_windowed && periodic_timer != NULL) {
        esp_timer_stop(periodic_timer);
        esp_timer_delete(periodic_timer);
        periodic_timer = NULL;
    }

    wdt_ctx.is_windowed = false;
    wdt_ctx.underflow_timeout = 0;
    wdt_ctx.window_open_flag = false;
    wdt_ctx.stop_feed = false;
}

bool wdt_is_underflow_detected(void)
{
    return wdt_ctx.stop_feed;
}

int wdt_init_windowed(uint32_t underflow_timeout_us)
{
    if(underflow_timeout_us == 0) {
        ESP_LOGE(TAG, "Underflow timeout cannot be 0");
        return -1;
    }

    wdt_ctx.is_windowed = true;
    wdt_ctx.underflow_timeout = underflow_timeout_us;

    /* Initialize esp_timer library first (idempotent - safe to call multiple times) */
    esp_err_t ret = esp_timer_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to initialize esp_timer: %d", ret);
        return -1;
    }

    /* Create timer with proper arguments */
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = wdt_windowed_underflow_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_ISR,
        .name = "wdt_windowed",
        .skip_unhandled_events = false,
    };
    ret = esp_timer_create(&periodic_timer_args, &periodic_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create periodic timer: %d", ret);
        return -1;
    }

    ESP_LOGI(TAG, "WDT windowed mode initialized");

    /* Initialize window as open to allow first feed */
    wdt_ctx.window_open_flag = true;

    /* Start the timer */
    ret = esp_timer_start_once(periodic_timer, wdt_ctx.underflow_timeout);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start windowed timer: %d", ret);
        return -1;
    }

    return 0;
}

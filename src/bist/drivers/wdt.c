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

#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_cpu.h"
#include "rom/ets_sys.h"
#include "hal/wdt_hal.h"
#include "bist_esp_types.h"

const char *TAG = "WDT";

static wdt_hal_context_t wdt_ctx;

void __attribute__((weak)) mwdt_callback(void *args)
{
    ESP_EARLY_LOGE(TAG, "WDT timeout");
}

void wdt_deinit(void)
{
    wdt_hal_write_protect_disable(&wdt_ctx);
    wdt_hal_disable(&wdt_ctx);
    wdt_hal_write_protect_enable(&wdt_ctx);
    wdt_hal_deinit(&wdt_ctx);
}

void wdt_init(uint32_t timeout_ms)
{
    esp_cpu_intr_disable(1 << ETS_INT_WDT_INUM);
    esp_rom_route_intr_matrix(esp_cpu_get_core_id(), ETS_TG0_WDT_LEVEL_INTR_SOURCE, ETS_INT_WDT_INUM);

    esp_cpu_intr_set_type(ETS_INT_WDT_INUM, 0);
    esp_cpu_intr_set_priority(ETS_INT_WDT_INUM, SOC_INTERRUPT_LEVEL_MEDIUM);
    esp_cpu_intr_set_handler(ETS_INT_WDT_INUM, mwdt_callback, &wdt_ctx);
    esp_cpu_intr_enable(1 << ETS_INT_WDT_INUM);

    ESP_EARLY_LOGD(TAG, "Enabling WDT(%d ms)", timeout_ms);

    wdt_hal_init(&wdt_ctx, WDT_MWDT0, MWDT_LL_DEFAULT_CLK_PRESCALER, true);
    wdt_hal_write_protect_disable(&wdt_ctx);
    wdt_hal_config_stage(&wdt_ctx, WDT_STAGE0, timeout_ms, WDT_STAGE_ACTION_INT);
    wdt_hal_config_stage(&wdt_ctx, WDT_STAGE1, timeout_ms * 2, WDT_STAGE_ACTION_RESET_SYSTEM);
    wdt_hal_enable(&wdt_ctx);
    wdt_hal_write_protect_enable(&wdt_ctx);
}

void wdt_feed(void)
{
    wdt_hal_write_protect_disable(&wdt_ctx);
    wdt_hal_feed(&wdt_ctx);
    wdt_hal_write_protect_enable(&wdt_ctx);
}

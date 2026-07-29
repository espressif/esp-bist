/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "sdkconfig.h"
#include "rom/ets_sys.h"
#include "esp_rom_serial_output.h"
#include "esp_log.h"
#include "esp_clk_internal.h"
#include "esp_cpu.h"
#include "riscv/interrupt.h"
#include "soc/reset_reasons.h"
#include "hal/cache_hal.h"
#include "hal/mmu_hal.h"
#include "hal/timg_ll.h"
#include "wdt.h"
#include "loader.h"
#include "riscv/rv_utils.h"
#include "soc/soc.h"
#include "soc/hp_sys_clkrst_reg.h"

#define HDR_ATTR __attribute__((section(".entry_addr"))) __attribute__((used))

extern uint32_t _image_irom_start, _image_irom_size, _image_irom_vaddr;
extern uint32_t _image_drom_start, _image_drom_size, _image_drom_vaddr;
extern uint32_t _image_rtc_start, _image_rtc_size, _image_rtc_vaddr;
extern uint32_t _image_tcm_start, _image_tcm_size, _image_tcm_vaddr;
extern char _bss_start[];
extern char _bss_end[];
extern char _rtc_bss_start[];
extern char _rtc_bss_end[];
extern char _iram_bss_start[];
extern char _iram_bss_end[];
extern int _vector_table;
extern int _mtvt_table;

static const char *TAG = "start";

void __start(void);

static HDR_ATTR void (*_entry_point)(void) = &__start;

extern int main();

void __start(void)
{
    /* Configure the global pointer register
     * (This should be the first thing startup does,
     * as any other piece of code could be relaxed by
     * the linker to access something relative to __global_pointer$)
     */
    __asm__ __volatile__(".option push\n"
                         ".option norelax\n"
                         "la gp, __global_pointer$\n"
                         ".option pop");

    /* Set the stack pointer to the top of the stack */
    __asm__ __volatile__("la sp, _stack_top");

    rv_utils_enable_fpu();

    esp_cpu_branch_prediction_enable();

    esp_cpu_intr_set_ivt_addr(&_vector_table);
    esp_cpu_intr_set_xtvt_addr(&_mtvt_table);

    ESP_EARLY_LOGD(TAG, "Clearing .bss section");
    memset(_bss_start, 0, (size_t)(_bss_end - _bss_start));

    /* This makes sure we have clock running if JTAG is plugged in,
     *  otherwise it can crash the CPU.
     */
    _timg_ll_enable_bus_clock(0, true);
    _timg_ll_reset_register(0);

    soc_reset_reason_t reset_reason = esp_rom_get_reset_reason(0);
    ESP_EARLY_LOGI(TAG, "Reset reason: %d", reset_reason);

    if (reset_reason != RESET_REASON_CORE_DEEP_SLEEP) {
        ESP_EARLY_LOGD(TAG, "Clearing .rtc.bss section");
        memset(_rtc_bss_start, 0, (size_t)(_rtc_bss_end - _rtc_bss_start));
    }

    ESP_EARLY_LOGD(TAG, "Clearing .iram.bss section");
    memset(_iram_bss_start, 0, (size_t)(_iram_bss_end - _iram_bss_start));

    cache_hal_config_t config = {
        .core_nums = 1,
        .l2_cache_size = CONFIG_CACHE_L2_CACHE_SIZE,
        .l2_cache_line_size = CONFIG_CACHE_L2_CACHE_LINE_SIZE,
    };
    cache_hal_init(&config);
    mmu_hal_config_t mmu_config = {
        .core_nums = 1,
    };
    mmu_hal_ctx_init(&mmu_config);

    size_t _partition_offset = PARTITION_OFFSET;
    uint32_t _app_irom_start = (_partition_offset + (uint32_t)&_image_irom_start);
    uint32_t _app_irom_size = (uint32_t)&_image_irom_size;
    uint32_t _app_irom_vaddr = ((uint32_t)&_image_irom_vaddr);

    uint32_t _app_drom_start = (_partition_offset + (uint32_t)&_image_drom_start);
    uint32_t _app_drom_size = (uint32_t)&_image_drom_size;
    uint32_t _app_drom_vaddr = ((uint32_t)&_image_drom_vaddr);

    uint32_t _app_rtc_start = (_partition_offset + (uint32_t)&_image_rtc_start);
    uint32_t _app_rtc_size = (uint32_t)&_image_rtc_size;
    uint32_t _app_rtc_vaddr = ((uint32_t)&_image_rtc_vaddr);

    uint32_t _app_tcm_start = (_partition_offset + (uint32_t)&_image_tcm_start);
    uint32_t _app_tcm_size = (uint32_t)&_image_tcm_size;
    uint32_t _app_tcm_vaddr = ((uint32_t)&_image_tcm_vaddr);

    esp_rom_output_tx_wait_idle(0);

    map_rtc_segment(_app_rtc_start, _app_rtc_vaddr, _app_rtc_size);
    map_tcm_segment(_app_tcm_start, _app_tcm_vaddr, _app_tcm_size);
    map_rom_segments(_app_drom_start, _app_drom_vaddr, _app_drom_size, _app_irom_start, _app_irom_vaddr, _app_irom_size);

    REG_CLR_BIT(HP_SYS_CLKRST_CPU_WAITI_CTRL0_REG, HP_SYS_CLKRST_REG_CORE1_WAITI_ICG_EN);
    REG_CLR_BIT(HP_SYS_CLKRST_SOC_CLK_CTRL0_REG, HP_SYS_CLKRST_REG_CORE1_CPU_CLK_EN);
    REG_SET_BIT(HP_SYS_CLKRST_HP_RST_EN0_REG, HP_SYS_CLKRST_REG_RST_EN_CORE1_GLOBAL);

    esp_rtc_init();
    esp_clk_init();
    esp_perip_clk_init();

    core_intr_matrix_clear();
    esprv_int_set_threshold(0);

    ESP_EARLY_LOGI(TAG, "Initializing Stack pattern");
    init_stack_pattern();

    ESP_EARLY_LOGI(TAG, "Calling main...");
    main();

    while (1) {
        __asm__ __volatile__("wfi");
    }
}

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
#include "riscv/rv_utils.h"
#include "soc/reset_reasons.h"
#include "soc/soc.h"
#include "soc/pcr_reg.h"
#include "hal/timg_ll.h"
#include "wdt.h"
#include "loader.h"
#include "hal/mmu_hal.h"

#define HDR_ATTR __attribute__((section(".entry_addr"))) __attribute__((used))

extern uint32_t _image_irom_start, _image_irom_size, _image_irom_vaddr;
extern uint32_t _image_drom_start, _image_drom_size, _image_drom_vaddr;
extern uint32_t _bss_start, _bss_end;
extern uint32_t _iram_bss_start, _iram_bss_end;
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

    __asm__ __volatile__("la sp, _stack_top");

    rv_utils_enable_fpu();

    esp_cpu_branch_prediction_enable();

    esp_cpu_intr_set_ivt_addr(&_vector_table);
    esp_cpu_intr_set_xtvt_addr(&_mtvt_table);

    ESP_EARLY_LOGD(TAG, "Clearing .bss section");
    memset(&_bss_start, 0, (&_bss_end - &_bss_start) * sizeof(_bss_start));

    /* This makes sure we have clock running if JTAG is plugged in,
     *  otherwise it can crash the CPU.
     */
    _timg_ll_enable_bus_clock(0, true);
    _timg_ll_reset_register(0);

    soc_reset_reason_t reset_reason = esp_rom_get_reset_reason(0);

    ESP_EARLY_LOGI(TAG, "Reset reason: %d", reset_reason);

    ESP_EARLY_LOGD(TAG, "Clearing .iram.bss section");
    memset(&_iram_bss_start, 0, (&_iram_bss_end - &_iram_bss_start) * sizeof(_iram_bss_start));

    mmu_hal_config_t mmu_cfg = {
        .core_nums = SOC_CPU_CORES_NUM,
        .mmu_page_size = CONFIG_MMU_PAGE_SIZE,
    };
    mmu_hal_init(&mmu_cfg);

    size_t _partition_offset = PARTITION_OFFSET;
    uint32_t _app_irom_start = (_partition_offset + (uint32_t)&_image_irom_start);
    uint32_t _app_irom_size = (uint32_t)&_image_irom_size;
    uint32_t _app_irom_vaddr = ((uint32_t)&_image_irom_vaddr);

    uint32_t _app_drom_start = (_partition_offset + (uint32_t)&_image_drom_start);
    uint32_t _app_drom_size = (uint32_t)&_image_drom_size;
    uint32_t _app_drom_vaddr = ((uint32_t)&_image_drom_vaddr);

    esp_rom_output_tx_wait_idle(0);

    REG_SET_FIELD(PCR_MSPI_CLK_CONF_REG, PCR_MSPI_FUNC_CLK_SEL, 0);
    REG_SET_BIT(PCR_MSPI_CLK_CONF_REG, PCR_MSPI_FUNC_CLK_EN);
    REG_SET_BIT(PCR_MSPI_CONF_REG, PCR_MSPI_CLK_EN);

    map_rom_segments(_app_drom_start, _app_drom_vaddr, _app_drom_size, _app_irom_start, _app_irom_vaddr, _app_irom_size);

    /* Bare-metal single-core: keep core1 clock-gated and in reset (IDF cpu_start.c). */
    REG_CLR_BIT(PCR_CORE1_CONF_REG, PCR_CORE1_CLK_EN);
    REG_SET_BIT(PCR_CORE1_CONF_REG, PCR_CORE1_RST_EN);

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

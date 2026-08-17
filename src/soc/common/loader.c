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

#include <stdint.h>
#include <string.h>
#include "hal/cache_hal.h"
#include "hal/cache_ll.h"
#include "hal/mmu_hal.h"
#include "rom/ets_sys.h"
#include "esp_private/periph_ctrl.h"
#include "esp_log.h"
#include "esp_cpu.h"
#include "loader.h"

#define CACHE_LEVEL CACHE_LL_LEVEL_EXT_MEM

extern uint32_t _stack_top, _stack_overflow_protection_start;

static const char *TAG = "loader";

void map_rom_segments(uint32_t app_drom_start, uint32_t app_drom_vaddr, uint32_t app_drom_size,
                      uint32_t app_irom_start, uint32_t app_irom_vaddr, uint32_t app_irom_size)
{
    uint32_t app_irom_start_aligned = app_irom_start & MMU_FLASH_MASK;
    uint32_t app_irom_vaddr_aligned = app_irom_vaddr & MMU_FLASH_MASK;

    uint32_t app_drom_start_aligned = app_drom_start & MMU_FLASH_MASK;
    uint32_t app_drom_vaddr_aligned = app_drom_vaddr & MMU_FLASH_MASK;

    uint32_t drom_mapped_len = 0;
    uint32_t irom_mapped_len = 0;

    ESP_EARLY_LOGI(TAG, "DROM segment: paddr=0x%1X, vaddr=0x%1X, size=0x%1X",
                   app_drom_start, app_drom_vaddr, app_drom_size);
    ESP_EARLY_LOGI(TAG, "IROM segment: paddr=0x%1X, vaddr=0x%1X, size=0x%1X",
                   app_irom_start, app_irom_vaddr, app_irom_size);

    cache_hal_disable(CACHE_LEVEL, CACHE_TYPE_ALL);

    /* Clear the MMU entries that are already set up,
     * so the new app only has the mappings it creates.
     */
    mmu_hal_unmap_all();

    mmu_hal_map_region(0, MMU_TARGET_FLASH0, app_drom_vaddr_aligned,
                       app_drom_start_aligned, app_drom_size, &drom_mapped_len);

    mmu_hal_map_region(0, MMU_TARGET_FLASH0, app_irom_vaddr_aligned,
                       app_irom_start_aligned, app_irom_size, &irom_mapped_len);

    /* ----------------------Enable corresponding buses---------------- */
    cache_bus_mask_t bus_mask = cache_ll_l1_get_bus(0, app_drom_vaddr_aligned, app_drom_size);
    cache_ll_l1_enable_bus(0, bus_mask);

    bus_mask = cache_ll_l1_get_bus(0, app_irom_vaddr_aligned, app_irom_size);
    cache_ll_l1_enable_bus(0, bus_mask);

#if CONFIG_MP_MAX_NUM_CPUS > 1
    bus_mask = cache_ll_l1_get_bus(1, app_drom_vaddr_aligned, app_drom_size);
    cache_ll_l1_enable_bus(1, bus_mask);
    bus_mask = cache_ll_l1_get_bus(1, app_irom_vaddr_aligned, app_irom_size);
    cache_ll_l1_enable_bus(1, bus_mask);
#endif

    /* ----------------------Enable Cache---------------- */
    cache_hal_enable(CACHE_LEVEL, CACHE_TYPE_ALL);

    /* On a cold reset the cache tag/data RAM comes up undefined and some lines
     * may be tagged valid with garbage. Invalidate the freshly mapped flash
     * ranges so early reads (e.g. pmu_init) miss and fetch correct data from
     * flash instead of stale lines. (A warm/WDT reset retains coherent lines,
     * which is why only the first boot after flashing was affected.) */
    cache_hal_invalidate_addr(app_drom_vaddr_aligned, drom_mapped_len);
    cache_hal_invalidate_addr(app_irom_vaddr_aligned, irom_mapped_len);
}

void map_rtc_segment(uint32_t app_rtc_start, uint32_t app_rtc_vaddr, uint32_t app_rtc_size)
{
    uint32_t app_rtc_start_aligned = app_rtc_start & MMU_FLASH_MASK;
    uint32_t app_rtc_vaddr_aligned = app_rtc_vaddr & MMU_FLASH_MASK;
    uint32_t size_after_paddr_aligned = (app_rtc_start - app_rtc_start_aligned) + app_rtc_size;
    uint32_t actual_mapped_len = 0;

    ESP_EARLY_LOGI(TAG, "RTC segment: paddr=0x%1X, vaddr=0x%1X, size=0x%1X", app_rtc_start, app_rtc_vaddr,
                   app_rtc_size);

    cache_hal_disable(CACHE_LEVEL, CACHE_TYPE_ALL);

    /**
     * To load RTC content to its virtual address (0x50000000) we need to:
     *
     * 1. Map RTC content to DCache (SOC_DROM_LOW = 0x3c000000) to access it
     * 2. Copy RTC content to its virtual address (0x50000000)
     * 3. This mapping will be reverted in map_rom_segments()
     */

    mmu_hal_map_region(0, MMU_TARGET_FLASH0, SOC_DROM_LOW, app_rtc_start_aligned, size_after_paddr_aligned,
                       &actual_mapped_len);

    cache_bus_mask_t bus_mask = cache_ll_l1_get_bus(0, app_rtc_vaddr_aligned, app_rtc_size);
    cache_ll_l1_enable_bus(0, bus_mask);
    cache_hal_enable(CACHE_LEVEL, CACHE_TYPE_ALL);

    void *data = (void *)(SOC_DROM_LOW + (app_rtc_start - app_rtc_start_aligned));

    memcpy((void *)app_rtc_vaddr, data, app_rtc_size);
}

#if SOC_MEM_TCM_SUPPORTED
void map_tcm_segment(uint32_t app_tcm_start, uint32_t app_tcm_vaddr, uint32_t app_tcm_size)
{
    if (app_tcm_size == 0) {
        return;
    }

    uint32_t app_tcm_start_aligned = app_tcm_start & MMU_FLASH_MASK;
    uint32_t app_tcm_vaddr_aligned = app_tcm_vaddr & MMU_FLASH_MASK;
    uint32_t size_after_paddr_aligned = (app_tcm_start - app_tcm_start_aligned) + app_tcm_size;
    uint32_t actual_mapped_len = 0;

    ESP_EARLY_LOGI(TAG, "TCM segment: paddr=0x%1X, vaddr=0x%1X, size=0x%1X",
                   app_tcm_start, app_tcm_vaddr, app_tcm_size);

    cache_hal_disable(CACHE_LEVEL, CACHE_TYPE_ALL);

    /**
     * To load TCM content to its virtual address (0x30100000) we need to:
     *
     * 1. Map TCM content to DCache (SOC_DROM_LOW = 0x3c000000) to access it
     * 2. Copy TCM content to its virtual address (0x30100000)
     * 3. This mapping will be reverted in map_rom_segments()
     */

    mmu_hal_map_region(0, MMU_TARGET_FLASH0, SOC_DROM_LOW, app_tcm_start_aligned,
                       size_after_paddr_aligned, &actual_mapped_len);

    cache_bus_mask_t bus_mask = cache_ll_l1_get_bus(0, app_tcm_vaddr_aligned, app_tcm_size);
    cache_ll_l1_enable_bus(0, bus_mask);
    cache_hal_enable(CACHE_LEVEL, CACHE_TYPE_ALL);

    void *data = (void *)(SOC_DROM_LOW + (app_tcm_start - app_tcm_start_aligned));

    memcpy((void *)app_tcm_vaddr, data, app_tcm_size);
}
#endif

void core_intr_matrix_clear(void)
{
    uint32_t core_id = esp_cpu_get_core_id();

    for (int i = 0; i < ETS_MAX_INTR_SOURCE; i++) {
        esp_rom_route_intr_matrix(core_id, i, ETS_INVALID_INUM);
    }
}

void init_stack_pattern(void)
{
    uint32_t *stack_top = (uint32_t *)&_stack_top;
    uint32_t *stack_bottom = (uint32_t *)&_stack_overflow_protection_start;

    for (uint32_t *p = stack_bottom; p < stack_top; p++) {
        *p = 0xBADC0FFE;
    }
}

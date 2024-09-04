/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "sdkconfig.h"
#include "hal/cache_hal.h"
#include "hal/mmu_hal.h"
#include "hal/cache_ll.h"
#include "rom/ets_sys.h"
#include "esp_rom_uart.h"
#include "esp_log.h"
#include "esp_clk_internal.h"
#include "esp_cpu.h"
#include "esp_private/periph_ctrl.h"
#include "esp32c3/rtc.h"
#include "soc/rtc_cntl_reg.h"
#include "wdt.h"

#define CONFIG_MMU_PAGE_SIZE 0x10000
#define MMU_FLASH_MASK       (~(CONFIG_MMU_PAGE_SIZE - 1))
#define PARTITION_OFFSET     0x10000
#define HDR_ATTR             __attribute__((section(".entry_addr"))) __attribute__((used))

extern uint32_t _image_irom_start, _image_irom_size, _image_irom_vaddr;
extern uint32_t _image_drom_start, _image_drom_size, _image_drom_vaddr;
extern uint32_t _image_rtc_start, _image_rtc_size, _image_rtc_vaddr;
extern uint32_t _bss_start, _bss_end;
extern uint32_t _rtc_bss_start, _rtc_bss_end;

extern int _vector_table;

static const char *TAG = "start";

void __start(void);

static HDR_ATTR void (*_entry_point)(void) = &__start;

extern int main();

void map_rom_segments(uint32_t app_drom_start, uint32_t app_drom_vaddr, uint32_t app_drom_size, uint32_t app_irom_start,
    uint32_t app_irom_vaddr, uint32_t app_irom_size)

{
    uint32_t app_irom_start_aligned = app_irom_start & MMU_FLASH_MASK;
    uint32_t app_irom_vaddr_aligned = app_irom_vaddr & MMU_FLASH_MASK;

    uint32_t app_drom_start_aligned = app_drom_start & MMU_FLASH_MASK;
    uint32_t app_drom_vaddr_aligned = app_drom_vaddr & MMU_FLASH_MASK;

    uint32_t actual_mapped_len = 0;

    /* Show map segments continue using same log format as during MCUboot phase */
    ESP_EARLY_LOGD(TAG, "DROM segment: paddr=0x%1X, vaddr=0x%1X, size=0x%1X", app_drom_start, app_drom_vaddr,
        app_drom_size);
    ESP_EARLY_LOGD(TAG, "IROM segment: paddr=0x%1X, vaddr=0x%1X, size=0x%1X", app_irom_start, app_irom_vaddr,
        app_irom_size);

    cache_hal_disable(CACHE_TYPE_ALL);

    /* Clear the MMU entries that are already set up,
     * so the new app only has the mappings it creates.
     */
    mmu_hal_unmap_all();

    mmu_hal_map_region(0, MMU_TARGET_FLASH0, app_drom_vaddr_aligned, app_drom_start_aligned, app_drom_size,
        &actual_mapped_len);

    mmu_hal_map_region(0, MMU_TARGET_FLASH0, app_irom_vaddr_aligned, app_irom_start_aligned, app_irom_size,
        &actual_mapped_len);

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
    cache_hal_enable(CACHE_TYPE_ALL);
}

void map_rtc_segment(uint32_t app_rtc_start, uint32_t app_rtc_vaddr, uint32_t app_rtc_size)
{
    uint32_t app_rtc_start_aligned = app_rtc_start & MMU_FLASH_MASK;
    uint32_t app_rtc_vaddr_aligned = app_rtc_vaddr & MMU_FLASH_MASK;
    uint32_t size_after_paddr_aligned = (app_rtc_start - app_rtc_start_aligned) + app_rtc_size;
    uint32_t actual_mapped_len = 0;

    ESP_EARLY_LOGD(TAG, "RTC segment: paddr=0x%1X, vaddr=0x%1X, size=0x%1X", app_rtc_start, app_rtc_vaddr,
        app_rtc_size);

    cache_hal_disable(CACHE_TYPE_ALL);

    /**
     * 	To load RTC content to its virtual address (0x50000000) we need to do the following:
     *
     * 1. Map RTC content to DCache (SOC_DROM_LOW = 0x3c000000) first in order to access it
     * 2. Copy RTC content to its virtual address (0x50000000)
     * 3. This mapping will be reverted in map_rom_segments()
     */

    mmu_hal_map_region(0, MMU_TARGET_FLASH0, SOC_DROM_LOW, app_rtc_start_aligned, size_after_paddr_aligned,
        &actual_mapped_len);

    cache_bus_mask_t bus_mask = cache_ll_l1_get_bus(0, app_rtc_vaddr_aligned, app_rtc_size);
    cache_ll_l1_enable_bus(0, bus_mask);
    cache_hal_enable(CACHE_TYPE_ALL);

    /* Start of RTC content in DCache */
    void *data = (void *)(SOC_DROM_LOW + (app_rtc_start - app_rtc_start_aligned));

    /* Copy RTC content to its virtual address */
    memcpy((void *)app_rtc_vaddr_aligned, data, app_rtc_size);
}

static void core_intr_matrix_clear(void)
{
    uint32_t core_id = esp_cpu_get_core_id();

    for (int i = 0; i < ETS_MAX_INTR_SOURCE; i++) {
        esp_rom_route_intr_matrix(core_id, i, ETS_INVALID_INUM);
    }
}

static void setup_clock_glitch_reset(bool enable)
{
    REG_CLR_BIT(RTC_CNTL_FIB_SEL_REG, RTC_CNTL_FIB_GLITCH_RST);

    if (enable) {
        REG_SET_BIT(RTC_CNTL_ANA_CONF_REG, RTC_CNTL_GLITCH_RST_EN);
    }
    else {
        REG_CLR_BIT(RTC_CNTL_ANA_CONF_REG, RTC_CNTL_GLITCH_RST_EN);
    }
}

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

    esp_cpu_intr_set_ivt_addr(&_vector_table);

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

    esp_rom_uart_tx_wait_idle(0);

    map_rtc_segment(_app_rtc_start, _app_rtc_vaddr, _app_rtc_size);

    map_rom_segments(_app_drom_start, _app_drom_vaddr, _app_drom_size, _app_irom_start, _app_irom_vaddr,
        _app_irom_size);

    ESP_EARLY_LOGD(TAG, "Clearing .bss section");
    memset(&_bss_start, 0, (&_bss_end - &_bss_start) * sizeof(_bss_start));

    soc_reset_reason_t reset_reason = esp_rom_get_reset_reason(0);

    ESP_EARLY_LOGI(TAG, "Reset reason: %d", reset_reason);

    /* Unless waking from deep sleep (implying RTC memory is intact), clear RTC bss */
    if (reset_reason != RESET_REASON_CORE_DEEP_SLEEP) {
        ESP_EARLY_LOGD(TAG, "Clearing .rtc.bss section");
        memset(&_rtc_bss_start, 0, (&_rtc_bss_end - &_rtc_bss_start) * sizeof(_rtc_bss_start));
    }

    setup_clock_glitch_reset(true);

    esp_clk_init();
    esp_perip_clk_init();

    // Clear interrupt matrix for PRO CPU core
    core_intr_matrix_clear();

    // esp_cache_err_int_init();

    wdt_init(CONFIG_WDT_TIMEOUT_MS);

    /* Jump to application entry point. */
    ESP_EARLY_LOGI(TAG, "Calling main...");
    main();

    while (1) {
        wdt_feed();
    }
}

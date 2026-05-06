/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include "soc/soc.h"
#include "wdt.h"
#include "loader.h"

#define HDR_ATTR __attribute__((section(".entry_addr"))) __attribute__((used))

extern uint32_t _image_irom_start, _image_irom_size, _image_irom_vaddr;
extern uint32_t _image_drom_start, _image_drom_size, _image_drom_vaddr;
extern uint32_t _image_rtc_start, _image_rtc_size, _image_rtc_vaddr;
extern uint32_t _bss_start, _bss_end;
extern uint32_t _rtc_bss_start, _rtc_bss_end;
extern uint32_t _iram_bss_start, _iram_bss_end;
extern int _vector_table;

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

    esp_cpu_intr_set_ivt_addr(&_vector_table);

    ESP_EARLY_LOGD(TAG, "Clearing .bss section");
    memset(&_bss_start, 0, (&_bss_end - &_bss_start) * sizeof(_bss_start));

    ESP_EARLY_LOGD(TAG, "Clearing .iram.bss section");
    memset(&_iram_bss_start, 0, (&_iram_bss_end - &_iram_bss_start) * sizeof(_iram_bss_start));

    size_t _partition_offset = PARTITION_OFFSET;
    uint32_t _app_irom_start = (_partition_offset + (uint32_t)&_image_irom_start);
    uint32_t _app_irom_size = (uint32_t)&_image_irom_size;
    uint32_t _app_irom_vaddr = ((uint32_t)&_image_irom_vaddr);

    uint32_t _app_drom_start = (_partition_offset + (uint32_t)&_image_drom_start);
    uint32_t _app_drom_size = (uint32_t)&_image_drom_size;
    uint32_t _app_drom_vaddr = ((uint32_t)&_image_drom_vaddr);

    /* ESP32-C61 has no LP/RTC RAM: rtc image is empty (size 0) and rtc
     * mapping/bss-clear become runtime no-ops. */
    uint32_t _app_rtc_start = (_partition_offset + (uint32_t)&_image_rtc_start);
    uint32_t _app_rtc_size = (uint32_t)&_image_rtc_size;
    uint32_t _app_rtc_vaddr = ((uint32_t)&_image_rtc_vaddr);

    esp_rom_output_tx_wait_idle(0);

    if (_app_rtc_size != 0) {
        map_rtc_segment(_app_rtc_start, _app_rtc_vaddr, _app_rtc_size);
    }

    map_rom_segments(_app_drom_start, _app_drom_vaddr, _app_drom_size, _app_irom_start, _app_irom_vaddr,_app_irom_size);

    soc_reset_reason_t reset_reason = esp_rom_get_reset_reason(0);

    ESP_EARLY_LOGI(TAG, "Reset reason: %d", reset_reason);

    /* Unless waking from deep sleep (implying RTC memory is intact), clear RTC bss */
    if (reset_reason != RESET_REASON_CORE_DEEP_SLEEP) {
        ESP_EARLY_LOGD(TAG, "Clearing .rtc.bss section");
        memset(&_rtc_bss_start, 0, (&_rtc_bss_end - &_rtc_bss_start) * sizeof(_rtc_bss_start));
    }

    esp_clk_init();
    esp_perip_clk_init();

    core_intr_matrix_clear();
    esprv_int_set_threshold(0);
    ESP_EARLY_LOGI(TAG, "Initializing Stack pattern");
    init_stack_pattern();

    /* Jump to application entry point. */
    ESP_EARLY_LOGI(TAG, "Calling main...");
    main();

    while (1) {
        __asm__ __volatile__("wfi");
    }
}

/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>
#include "sdkconfig.h"

#define MMU_FLASH_MASK       (~(CONFIG_MMU_PAGE_SIZE - 1))
#define PARTITION_OFFSET     0x10000

void map_rom_segments(uint32_t app_drom_start, uint32_t app_drom_vaddr, uint32_t app_drom_size,
    uint32_t app_irom_start, uint32_t app_irom_vaddr, uint32_t app_irom_size);

void map_rtc_segment(uint32_t app_rtc_start, uint32_t app_rtc_vaddr, uint32_t app_rtc_size);

void core_intr_matrix_clear(void);

void init_stack_pattern(void);

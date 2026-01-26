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

/**
 * @file bist_cpu_regs.h
 * @brief CPU general-purpose register integrity test
 *
 * Tests all 32 RISC-V general-purpose registers (X1-X31) by writing
 * alternating bit patterns (0xAAAAAAAA and 0x55555555) to verify:
 * - No stuck-at-0 or stuck-at-1 faults
 * - All register bits functional
 * - Proper read/write operation
 */

#pragma once

#include "stdint.h"
#include "bist_esp_types.h"
#ifndef __ZEPHYR__
#include "bist_conf.h"
#endif

/**
 * @brief Test CPU general-purpose register integrity
 *
 * Verifies all 32 RISC-V general-purpose registers can retain both
 * 0 and 1 values without corruption. Uses checkerboard patterns
 * (0xAAAAAAAA and 0x55555555) to detect stuck-at faults.
 *
 * @return BIST_ESP_OK if all registers pass
 * @return BIST_ESP_CPU_TEST_ERR if any register fails
 */
bist_esp_err_t bist_cpu_regs_test(void);

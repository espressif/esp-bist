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

/**
 * @file bist_cpu_csr_regs.h
 * @brief CPU Control and Status Register (CSR) integrity test
 *
 * Tests RISC-V CSRs including MTVEC, MSCRATCH, MEPC, MCAUSE, MTVAL,
 * and all 16 PMP address registers (PMPADDR0-PMPADDR15) by writing
 * alternating bit patterns while respecting CSR-specific write masks.
 * Original CSR values are saved and restored after testing.
 */

#pragma once

#include "stdint.h"
#include "bist_esp_types.h"
#ifndef __ZEPHYR__
#include "bist_conf.h"
#endif

/**
 * @brief Test CPU Control and Status Register integrity
 *
 * Verifies 21 CSRs (5 main CSRs + 16 PMP address CSRs) can be
 * written and read reliably. Saves original values, tests with
 * alternating patterns, and restores originals.
 *
 * @return BIST_ESP_OK if all CSRs pass
 * @return BIST_ESP_CPU_CSR_TEST_ERR if any CSR fails
 */
bist_esp_err_t bist_cpu_csr_regs_test(void);

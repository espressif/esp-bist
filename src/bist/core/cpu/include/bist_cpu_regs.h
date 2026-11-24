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
 * @file bist_cpu_regs.h
 * @brief Stable API for the CPU register integrity test
 *
 * The STATIC and RECIPROCAL implementations expose the same
 * bist_cpu_regs_test() entry point. Both implementations exercise all
 * writable RISC-V general-purpose registers, x1 through x31; x0 is
 * architecturally fixed at zero and is not writable. Each tested register is
 * written and read back with the complementary 0xAAAAAAAA and 0x55555555
 * checkerboard patterns to exercise every bit as both 0 and 1. This checks
 * for stuck-at-0 and stuck-at-1 faults, bit functionality, and correct
 * read/write retention. When FPU register testing is enabled, all
 * floating-point registers, f0 through f31, are exercised with the same
 * patterns.
 *
 * STATIC performs a register walk on the calling core. RECIPROCAL performs a
 * walk on each HP core, records fixed-layout raw read-back evidence in two
 * shared slots, and has each core validate both its own and the other core's
 * read-backs.
 */

#pragma once

#include "stdint.h"
#include "bist_esp_types.h"
#ifndef __ZEPHYR__
#include "bist_conf.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Test CPU general-purpose register integrity
 *
 * In STATIC mode, performs one 0xAAAAAAAA/0x55555555 checkerboard walk over
 * x1-x31 and, when enabled, f0-f31. In RECIPROCAL mode, performs those walks
 * on both HP cores while preserving the raw pattern read-backs in the shared
 * evidence slots declared by bist_cpu_regs_reciprocal.h; each core then
 * validates the evidence produced by both cores.
 *
 * @return BIST_ESP_OK if all registers pass
 * @return BIST_ESP_CPU_TEST_ERR if any register fails
 */
bist_esp_err_t bist_cpu_regs_test(void);

#ifdef __cplusplus
}
#endif

#if defined(CONFIG_ESP_BIST_CPU_REG_TEST_RECIPROCAL)
#include "bist_cpu_regs_reciprocal.h"
#endif

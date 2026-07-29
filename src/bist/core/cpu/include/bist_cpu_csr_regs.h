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
 * Tests RISC-V CSRs by writing alternating bit patterns while respecting
 * CSR-specific write masks. Original values are saved and restored.
 *
 * Registers tested on all SOCs (except Zephyr builds):
 *   - Machine Trap Setup: MTVEC
 *   - Machine Trap Handling: MSCRATCH, MEPC, MCAUSE, MTVAL
 *
 * PMP registers on all SOCs (skipped on Zephyr):
 *   - PMP address: PMPADDR0-PMPADDR15
 *     - C3/C6/H2: full 32-bit mask (4-byte granularity)
 *     - C5/C61: mask 0x3FFFFFE0 (25 writable bits, 128-byte granularity)
 *   - PMP configuration: PMPCFG0-PMPCFG3
 *     - C3/C6/H2: mask 0x1D1D1D1D (excludes L, reserved, and W bits)
 *     - C5/C61: mask 0x0D0D0D0D (additionally excludes upper A-field bit
 *       because A=NA4 is not selectable at G=5 per RISC-V spec)
 *     W (bit 1) is excluded on all SOCs because the 0xAA test pattern
 *     sets R=0,W=1 which is a reserved RISC-V encoding — hardware
 *     WARL-clears W when R=0.
 *
 * PMA address registers on SOCs with PMA (C6, H2, C5; guarded by SOC_CPU_HAS_PMA):
 *   - PMA address: pma_addr0-11 (mask 0x3FFFFFE0, 25 writable bits with
 *     32-byte granularity). Entries 12-15 are skipped because the ROM
 *     bootloader may configure them as active regions whose NAPOT/TOR
 *     encoding forces address low-order bits.
 *
 * Espressif custom CSRs (C5 only; guarded by SOC_TARGET_ESP32C5):
 *   - Machine extension status: mexstatus at CSR 0x7E1 (mask 0x00102C00)
 *   - Machine hint: mhint at CSR 0x7C5 (mask 0x00100000)
 *   C6/H2 mexstatus only exposes SOFT_RST bits (unsafe to test);
 *   mhint (0x7C5) does not exist on C6/H2 (illegal instruction).
 *
 * FPU CSRs on SOCs with FPU (guarded by SOC_CPU_HAS_FPU):
 *   - fflags (mask 0x1F): accrued exception flags
 *   - frm (mask 0x07): dynamic rounding mode
 *   - fcsr (mask 0xFF): combined fflags and frm
 *
 * Note: PMA cfg registers are not tested because the PMA_L (Lock) bit
 * is write-once; any test pattern that sets bit 29 permanently locks
 * the entry until the next power-on reset.
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
 * Verifies CSRs can be written and read reliably using alternating
 * patterns (0xAAAAAAAA / 0x55555555) masked to each register's writable
 * bits. Saves original values before testing and restores them afterward.
 *
 * The number of CSRs tested varies by SOC:
 *   - ESP32-C3: 25 CSRs (5 trap + 16 pmpaddr + 4 pmpcfg)
 *   - ESP32-C6/H2: 37 CSRs (25 common + 12 pma_addr)
 *   - ESP32-C5: 39 CSRs (25 common + 12 pma_addr + mexstatus + mhint)
 *   - ESP32-H4/P4: 40 CSRs (25 common + 12 pma_addr + fflags + frm + fcsr)
 *
 * @return BIST_ESP_OK if all CSRs pass
 * @return BIST_ESP_CPU_CSR_TEST_ERR if any CSR fails
 */
bist_esp_err_t bist_cpu_csr_regs_test(void);

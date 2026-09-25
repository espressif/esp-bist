/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * This file is part of Espressif's BIST (Built-In Self Test) Library.
 *
 * BIST library is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * BIST library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with BIST library. If not, see
 * <https://www.gnu.org/licenses/lgpl-3.0.html>.
 */

/**
 * @file bist_cpu_csr_regs_reciprocal.h
 * @brief Fixed-layout evidence for the ESP32-P4 reciprocal CSR test
 *
 * Defines stable indices for the 40 CSRs in the reciprocal walk. Each entry
 * preserves the raw CSR value read after writing pattern A (0xAAAAAAAA) and
 * pattern 5 (0x55555555), with each write masked to that CSR's writable
 * bits. Validation applies the same per-CSR mask to the raw read-backs.
 *
 * The pointer-free result has a fixed 324-byte layout: one status word
 * followed by 40 pairs of checkerboard read-backs. Two shared slots hold the
 * independently produced results for HP cores 0 and 1.
 */

#pragma once

#include "stddef.h"
#include "stdint.h"

/** Number of CSRs tested by the ESP32-P4 reciprocal CSR walk. */
#define BIST_CPU_CSR_REGS_RESULT_CC_CSR_COUNT 40

/** Checkerboard pattern A: alternating 1010... bit pattern (0xAAAAAAAA). */
#define BIST_CPU_CSR_REGS_RESULT_CC_PATTERN_A UINT32_C(0xAAAAAAAA)

/** Checkerboard pattern 5: alternating 0101... bit pattern (0x55555555). */
#define BIST_CPU_CSR_REGS_RESULT_CC_PATTERN_5 UINT32_C(0x55555555)

/** Number of shared result slots, one for each HP core. */
#define BIST_CPU_CSR_REGS_RESULT_CC_SLOT_COUNT 2

/** Byte size of one CSR read-back entry. */
#define BIST_CPU_CSR_REGS_RESULT_CC_READBACK_SIZE 8

/** Byte offset of pattern_a within one CSR read-back entry. */
#define BIST_CPU_CSR_REGS_RESULT_CC_OFFSET_PATTERN_A 0

/** Byte offset of pattern_5 within one CSR read-back entry. */
#define BIST_CPU_CSR_REGS_RESULT_CC_OFFSET_PATTERN_5 4

/** Byte offset of status in bist_cpu_csr_regs_result_cc_t. */
#define BIST_CPU_CSR_REGS_RESULT_CC_OFFSET_STATUS 0

/** Byte offset of the CSR read-back array in bist_cpu_csr_regs_result_cc_t. */
#define BIST_CPU_CSR_REGS_RESULT_CC_OFFSET_CSR 4

/** Byte offset of CSR read-back entry n in bist_cpu_csr_regs_result_cc_t. */
#define BIST_CPU_CSR_REGS_RESULT_CC_OFFSET_CSR_ENTRY(index)                                                             \
    (BIST_CPU_CSR_REGS_RESULT_CC_OFFSET_CSR + ((index)*BIST_CPU_CSR_REGS_RESULT_CC_READBACK_SIZE))

/** Byte offset of pattern A for CSR read-back entry n. */
#define BIST_CPU_CSR_REGS_RESULT_CC_OFFSET_CSR_PATTERN_A(index)                                                         \
    (BIST_CPU_CSR_REGS_RESULT_CC_OFFSET_CSR_ENTRY(index) + BIST_CPU_CSR_REGS_RESULT_CC_OFFSET_PATTERN_A)

/** Byte offset of pattern 5 for CSR read-back entry n. */
#define BIST_CPU_CSR_REGS_RESULT_CC_OFFSET_CSR_PATTERN_5(index)                                                         \
    (BIST_CPU_CSR_REGS_RESULT_CC_OFFSET_CSR_ENTRY(index) + BIST_CPU_CSR_REGS_RESULT_CC_OFFSET_PATTERN_5)

/** Byte size of bist_cpu_csr_regs_result_cc_t. */
#define BIST_CPU_CSR_REGS_RESULT_CC_TOTAL_SIZE 324

/**
 * @brief Stable read-back indices for all ESP32-P4 CSRs in the walk
 */
typedef enum {
    BIST_CPU_CSR_REGS_CC_INDEX_MTVEC = 0,
    BIST_CPU_CSR_REGS_CC_INDEX_MSCRATCH,
    BIST_CPU_CSR_REGS_CC_INDEX_MEPC,
    BIST_CPU_CSR_REGS_CC_INDEX_MCAUSE,
    BIST_CPU_CSR_REGS_CC_INDEX_MTVAL,
    BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR0,
    BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR1,
    BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR2,
    BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR3,
    BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR4,
    BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR5,
    BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR6,
    BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR7,
    BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR8,
    BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR9,
    BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR10,
    BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR11,
    BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR12,
    BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR13,
    BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR14,
    BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR15,
    BIST_CPU_CSR_REGS_CC_INDEX_PMPCFG0,
    BIST_CPU_CSR_REGS_CC_INDEX_PMPCFG1,
    BIST_CPU_CSR_REGS_CC_INDEX_PMPCFG2,
    BIST_CPU_CSR_REGS_CC_INDEX_PMPCFG3,
    BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR0,
    BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR1,
    BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR2,
    BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR3,
    BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR4,
    BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR5,
    BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR6,
    BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR7,
    BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR8,
    BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR9,
    BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR10,
    BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR11,
    BIST_CPU_CSR_REGS_CC_INDEX_FFLAGS,
    BIST_CPU_CSR_REGS_CC_INDEX_FRM,
    BIST_CPU_CSR_REGS_CC_INDEX_FCSR,
} bist_cpu_csr_regs_index_cc_t;

/**
 * @brief Read-backs for one CSR
 *
 * These are the unprocessed 32-bit CSR values read after writing
 * 0xAAAAAAAA and 0x55555555, respectively, masked for the CSR under test.
 * Bits outside the writable mask remain present in the stored raw values and
 * are ignored by validation.
 */
typedef struct {
    uint32_t pattern_a; /**< Raw value read after the masked 0xAAAAAAAA write. */
    uint32_t pattern_5; /**< Raw value read after the masked 0x55555555 write. */
} bist_cpu_csr_reg_readback_cc_t;

/**
 * @brief Pointer-free, fixed-layout reciprocal CSR test result
 *
 * csr[n] uses bist_cpu_csr_regs_index_cc_t as its stable index. The wrapper
 * zeroes all 324 bytes before each walk, so entries not reached after an
 * early failure remain zero. status contains the bist_esp_err_t execution or
 * validation result represented as a uint32_t; it is not a validity bitmap
 * for individual entries.
 */
typedef struct {
    uint32_t status; /**< bist_esp_err_t return value represented as a uint32_t. */
    bist_cpu_csr_reg_readback_cc_t csr[BIST_CPU_CSR_REGS_RESULT_CC_CSR_COUNT]; /**< Read-backs by stable CSR index. */
} bist_cpu_csr_regs_result_cc_t;

/** @cond */
#if defined(__cplusplus)
#define BIST_CPU_CSR_REGS_CC_STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
#define BIST_CPU_CSR_REGS_CC_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#endif

BIST_CPU_CSR_REGS_CC_STATIC_ASSERT(sizeof(bist_cpu_csr_reg_readback_cc_t)
                                   == BIST_CPU_CSR_REGS_RESULT_CC_READBACK_SIZE,
                                   "CPU CSR read-back entry layout changed");
BIST_CPU_CSR_REGS_CC_STATIC_ASSERT(offsetof(bist_cpu_csr_reg_readback_cc_t, pattern_a)
                                   == BIST_CPU_CSR_REGS_RESULT_CC_OFFSET_PATTERN_A,
                                   "CPU CSR pattern A offset changed");
BIST_CPU_CSR_REGS_CC_STATIC_ASSERT(offsetof(bist_cpu_csr_reg_readback_cc_t, pattern_5)
                                   == BIST_CPU_CSR_REGS_RESULT_CC_OFFSET_PATTERN_5,
                                   "CPU CSR pattern 5 offset changed");
BIST_CPU_CSR_REGS_CC_STATIC_ASSERT(offsetof(bist_cpu_csr_regs_result_cc_t, status)
                                   == BIST_CPU_CSR_REGS_RESULT_CC_OFFSET_STATUS,
                                   "CPU CSR result status offset changed");
BIST_CPU_CSR_REGS_CC_STATIC_ASSERT(offsetof(bist_cpu_csr_regs_result_cc_t, csr)
                                   == BIST_CPU_CSR_REGS_RESULT_CC_OFFSET_CSR,
                                   "CPU CSR result read-back offset changed");
BIST_CPU_CSR_REGS_CC_STATIC_ASSERT(
    BIST_CPU_CSR_REGS_RESULT_CC_OFFSET_CSR_ENTRY(BIST_CPU_CSR_REGS_CC_INDEX_FCSR)
    + BIST_CPU_CSR_REGS_RESULT_CC_READBACK_SIZE
    == BIST_CPU_CSR_REGS_RESULT_CC_TOTAL_SIZE,
    "CPU CSR final entry offset changed");
BIST_CPU_CSR_REGS_CC_STATIC_ASSERT(sizeof(bist_cpu_csr_regs_result_cc_t)
                                   == BIST_CPU_CSR_REGS_RESULT_CC_TOTAL_SIZE,
                                   "CPU CSR result size changed");

#undef BIST_CPU_CSR_REGS_CC_STATIC_ASSERT
/** @endcond */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Shared fixed-layout result slots for HP cores 0 and 1
 *
 * The two 324-byte slots reside in shared HP SRAM. Slot 0 and slot 1 hold
 * evidence produced by HP core 0 and HP core 1, respectively, for reciprocal
 * local and cross-core read-back validation.
 */
extern volatile bist_cpu_csr_regs_result_cc_t
bist_cpu_csr_regs_shared_results_cc[BIST_CPU_CSR_REGS_RESULT_CC_SLOT_COUNT];

#ifdef __cplusplus
}
#endif

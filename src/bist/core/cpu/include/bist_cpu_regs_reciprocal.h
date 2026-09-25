/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
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
 * @file bist_cpu_regs_reciprocal.h
 * @brief Fixed-layout evidence for the reciprocal CPU register test
 *
 * For every register reached by the walk, the result stores the raw read-back
 * after writing pattern A (0xAAAAAAAA) and pattern 5 (0x55555555). The
 * pointer-free result is always 520 bytes, including storage for x0-x31 and
 * f0-f31, so its layout is unchanged when ESP_BIST_USE_FPU is disabled and
 * evidence can be exchanged between HP cores. Two shared slots hold the
 * independently produced results for HP cores 0 and 1.
 */

#pragma once

#include "stddef.h"
#include "stdint.h"

/** Number of architectural General-Purpose Registers (x0 through x31). */
#define BIST_CPU_REGS_RESULT_CC_GPR_COUNT 32

/** Number of architectural Floating-Point Registers (f0 through f31). */
#define BIST_CPU_REGS_RESULT_CC_FPR_COUNT 32

/** Checkerboard pattern A: alternating 1010... bit pattern (0xAAAAAAAA). */
#define BIST_CPU_REGS_RESULT_CC_PATTERN_A UINT32_C(0xAAAAAAAA)

/** Checkerboard pattern 5: alternating 0101... bit pattern (0x55555555). */
#define BIST_CPU_REGS_RESULT_CC_PATTERN_5 UINT32_C(0x55555555)

/**
 * feature_flags bit indicating that the f0-f31 register walk was executed.
 *
 * When this bit is clear, the FPR array is present for layout stability but
 * does not contain FPR test evidence.
 */
#define BIST_CPU_REGS_RESULT_CC_FEATURE_FPR_TESTED (UINT32_C(1) << 0)

/** Number of shared result slots, one for each HP core. */
#define BIST_CPU_REGS_RESULT_CC_SLOT_COUNT 2

/** Expected byte size of a single register read-back entry (pattern_a + pattern_5). */
#define BIST_CPU_REGS_RESULT_CC_READBACK_SIZE 8

/** Expected byte size of the entire bist_cpu_regs_result_cc_t structure. */
#define BIST_CPU_REGS_RESULT_CC_TOTAL_SIZE 520

/** Expected byte offset of feature_flags in bist_cpu_regs_result_cc_t. */
#define BIST_CPU_REGS_RESULT_CC_OFFSET_FEATURE_FLAGS 0

/** Expected byte offset of status in bist_cpu_regs_result_cc_t. */
#define BIST_CPU_REGS_RESULT_CC_OFFSET_STATUS        4

/** Expected byte offset of GPR read-back array in bist_cpu_regs_result_cc_t. */
#define BIST_CPU_REGS_RESULT_CC_OFFSET_GPR           8

/** Expected byte offset of FPR read-back array in bist_cpu_regs_result_cc_t. */
#define BIST_CPU_REGS_RESULT_CC_OFFSET_FPR           264

/** Expected byte offset of GPR read-back entry for register index n. */
#define BIST_CPU_REGS_RESULT_CC_OFFSET_GPR_ENTRY(index)                                                                 \
    (BIST_CPU_REGS_RESULT_CC_OFFSET_GPR + ((index)*BIST_CPU_REGS_RESULT_CC_READBACK_SIZE))

/** Expected byte offset of FPR read-back entry for register index n. */
#define BIST_CPU_REGS_RESULT_CC_OFFSET_FPR_ENTRY(index)                                                                 \
    (BIST_CPU_REGS_RESULT_CC_OFFSET_FPR + ((index)*BIST_CPU_REGS_RESULT_CC_READBACK_SIZE))

/**
 * @brief Raw read-backs for one architectural register
 *
 * The fields preserve the unprocessed 32-bit values read from the register
 * after writing exactly 0xAAAAAAAA and 0x55555555, respectively.
 */
typedef struct {
    uint32_t pattern_a; /**< Raw bits read after writing 0xAAAAAAAA (BIST_CPU_REGS_RESULT_CC_PATTERN_A). */
    uint32_t pattern_5; /**< Raw bits read after writing 0x55555555 (BIST_CPU_REGS_RESULT_CC_PATTERN_5). */
} bist_cpu_reg_readback_cc_t;

/**
 * @brief Fixed-layout CPU register test result
 *
 * This 520-byte structure contains only uint32_t-based values and no
 * pointers. Array indices match architectural register numbers: gpr[n]
 * corresponds to xn for n=0..31 and fpr[n] corresponds to fn for n=0..31.
 * The actual GPR walk covers x1-x31 because x0 is architecturally fixed at
 * zero; gpr[0] is therefore untouched by the walk. The FPR walk covers
 * f0-f31 only when BIST_CPU_REGS_RESULT_CC_FEATURE_FPR_TESTED is set. FPR
 * storage remains present when that feature is disabled to preserve the
 * fixed layout.
 *
 * The wrapper clears the entire structure before each run. After an early
 * failure, entries not reached by the register walk therefore remain zero;
 * gpr[0] also remains zero in a normally completed run. There is no validity
 * mask, and a zero value does not unambiguously identify an untested entry.
 */
typedef struct {
    uint32_t feature_flags;                                          /**< BIST_CPU_REGS_RESULT_CC_FEATURE_* bits. */
    uint32_t status;                                                 /**< bist_esp_err_t return value as a uint32_t. */
    bist_cpu_reg_readback_cc_t gpr[BIST_CPU_REGS_RESULT_CC_GPR_COUNT]; /**< Architectural x0-x31 read-backs. */
    bist_cpu_reg_readback_cc_t fpr[BIST_CPU_REGS_RESULT_CC_FPR_COUNT]; /**< Architectural f0-f31 read-backs. */
} bist_cpu_regs_result_cc_t;

/** @cond */
#if defined(__cplusplus)
#define BIST_CPU_REGS_CC_STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
#define BIST_CPU_REGS_CC_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#endif

BIST_CPU_REGS_CC_STATIC_ASSERT(sizeof(bist_cpu_reg_readback_cc_t) == BIST_CPU_REGS_RESULT_CC_READBACK_SIZE,
                               "CPU register read-back entry layout changed");
BIST_CPU_REGS_CC_STATIC_ASSERT(offsetof(bist_cpu_regs_result_cc_t, feature_flags)
                               == BIST_CPU_REGS_RESULT_CC_OFFSET_FEATURE_FLAGS,
                               "CPU register result feature_flags offset changed");
BIST_CPU_REGS_CC_STATIC_ASSERT(offsetof(bist_cpu_regs_result_cc_t, status) == BIST_CPU_REGS_RESULT_CC_OFFSET_STATUS,
                               "CPU register result status offset changed");
BIST_CPU_REGS_CC_STATIC_ASSERT(offsetof(bist_cpu_regs_result_cc_t, gpr) == BIST_CPU_REGS_RESULT_CC_OFFSET_GPR,
                               "CPU register result GPR offset changed");
BIST_CPU_REGS_CC_STATIC_ASSERT(offsetof(bist_cpu_regs_result_cc_t, fpr) == BIST_CPU_REGS_RESULT_CC_OFFSET_FPR,
                               "CPU register result FPR offset changed");
BIST_CPU_REGS_CC_STATIC_ASSERT(sizeof(bist_cpu_regs_result_cc_t) == BIST_CPU_REGS_RESULT_CC_TOTAL_SIZE,
                               "CPU register result size changed");

#undef BIST_CPU_REGS_CC_STATIC_ASSERT
/** @endcond */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Shared fixed-layout result slots for HP cores 0 and 1
 *
 * The two 520-byte slots reside in HP SRAM accessible to both cores on
 * dual-core targets. Slot 0 and slot 1 hold the evidence produced by HP core
 * 0 and HP core 1, respectively. Callers explicitly choose and own a slot,
 * then pass its address to the reciprocal register walk. This array provides
 * no slot selection, ownership arbitration, completion signaling, or
 * cross-core synchronization. Volatile accesses do not provide
 * synchronization.
 */
extern volatile bist_cpu_regs_result_cc_t bist_cpu_regs_shared_results_cc[BIST_CPU_REGS_RESULT_CC_SLOT_COUNT];

#ifdef __cplusplus
}
#endif

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
 * @file bist_interrupt_reciprocal.h
 * @brief Fixed-layout evidence for reciprocal interrupt tests
 *
 * Both HP cores run the reciprocal software source-map and hardware timer
 * walks and publish pointer-free evidence frames. Each frame is validated on
 * its producing core and cross-validated by the other core.
 */

#pragma once

#include "stddef.h"
#include "stdint.h"

/** Number of software IRQ sources exercised (CPU_INTR_FROM_CPU_0..2).
 *  FROM_CPU_3 is reserved for bist_multicore_call() IPC wake on Core 1. */
#define BIST_INTERRUPT_SW_SOURCE_COUNT 3

/** Triggers per software IRQ source. */
#define BIST_INTERRUPT_SW_LOOP_COUNT 8

/** CPU interrupt line used by the software source-map walk. */
#define BIST_INTERRUPT_SW_CPU_NUMBER 9

/** completion_mask value once every software source has been walked. */
#define BIST_INTERRUPT_SW_RESULT_COMPLETE_MASK \
    ((UINT32_C(1) << BIST_INTERRUPT_SW_SOURCE_COUNT) - 1)

/** Number of shared result slots (one per HP core). */
#define BIST_INTERRUPT_RESULT_SLOT_COUNT 2

/** Expected byte size of bist_interrupt_sw_result_t. */
#define BIST_INTERRUPT_SW_RESULT_TOTAL_SIZE 44

/** Expected byte offset of status in bist_interrupt_sw_result_t. */
#define BIST_INTERRUPT_SW_RESULT_OFFSET_STATUS                0

/** Expected byte offset of completion_mask in bist_interrupt_sw_result_t. */
#define BIST_INTERRUPT_SW_RESULT_OFFSET_COMPLETION_MASK       4

/** Expected byte offset of interrupt_count in bist_interrupt_sw_result_t. */
#define BIST_INTERRUPT_SW_RESULT_OFFSET_INTERRUPT_COUNT       8

/** Expected byte offset of enabled_mask_active in bist_interrupt_sw_result_t. */
#define BIST_INTERRUPT_SW_RESULT_OFFSET_ENABLED_MASK_ACTIVE   20

/** Expected byte offset of enabled_mask_cleared in bist_interrupt_sw_result_t. */
#define BIST_INTERRUPT_SW_RESULT_OFFSET_ENABLED_MASK_CLEARED  32

/** Expected byte size of bist_interrupt_hw_result_t. */
#define BIST_INTERRUPT_HW_RESULT_TOTAL_SIZE 12

/** Expected byte offset of status in bist_interrupt_hw_result_t. */
#define BIST_INTERRUPT_HW_RESULT_OFFSET_STATUS      0

/** Expected byte offset of count_1 in bist_interrupt_hw_result_t. */
#define BIST_INTERRUPT_HW_RESULT_OFFSET_COUNT_1     4

/** Expected byte offset of count_2 in bist_interrupt_hw_result_t. */
#define BIST_INTERRUPT_HW_RESULT_OFFSET_COUNT_2     8

/**
 * Minimum TIMG0 (500 us) ISR count before the hardware walk may exit.
 *
 * TIMG1 uses a 1000 us alarm and is expected near half of count_1. The local
 * walk checks the 2:1 relationship every five TIMG1 interrupts with one-count
 * tolerance; reciprocal evidence validation allows two counts of final
 * sampling skew.
 */
#define BIST_INTERRUPT_HW_MIN_COUNT 1000

/**
 * @brief Fixed-layout software interrupt source-map result
 *
 * For each software source n, interrupt_count[n] is the ISR delivery count,
 * enabled_mask_active[n] is the CPU enable mask while the line is enabled,
 * and enabled_mask_cleared[n] is the mask after disable/unmap. Bit n of
 * completion_mask is set once source n has been walked. A valid completed
 * walk has every bit in BIST_INTERRUPT_SW_RESULT_COMPLETE_MASK set, every
 * count equal to BIST_INTERRUPT_SW_LOOP_COUNT, CPU line 9 set in each active
 * mask, and CPU line 9 clear in each cleared mask. The walk clears the
 * 44-byte frame and marks status failed before any source is exercised, so
 * an interrupted or incomplete walk never leaves a passing frame.
 */
typedef struct {
    uint32_t status;                                                    /**< bist_esp_err_t as uint32_t. */
    uint32_t completion_mask;                                           /**< One bit per software source walked. */
    uint32_t interrupt_count[BIST_INTERRUPT_SW_SOURCE_COUNT];           /**< ISR count per source. */
    uint32_t enabled_mask_active[BIST_INTERRUPT_SW_SOURCE_COUNT];       /**< Enable mask while line enabled. */
    uint32_t enabled_mask_cleared[BIST_INTERRUPT_SW_SOURCE_COUNT];      /**< Enable mask after disable. */
} bist_interrupt_sw_result_t;

/**
 * @brief Fixed-layout hardware interrupt result
 *
 * count_1 / count_2 are the TIMG0 / TIMG1 ISR counts at walk exit.
 * status reflects the mid-run 2:1 ratio check (same as bist_interrupt.c).
 */
typedef struct {
    uint32_t status;        /**< bist_esp_err_t as uint32_t. */
    uint32_t count_1;       /**< TIMG0 (500 us) ISR count. */
    uint32_t count_2;       /**< TIMG1 (1000 us) ISR count. */
} bist_interrupt_hw_result_t;

/** @cond */
#if defined(__cplusplus)
#define BIST_INTERRUPT_STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
#define BIST_INTERRUPT_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#endif

BIST_INTERRUPT_STATIC_ASSERT(offsetof(bist_interrupt_sw_result_t, status) ==
                             BIST_INTERRUPT_SW_RESULT_OFFSET_STATUS,
                             "SW interrupt result status offset changed");
BIST_INTERRUPT_STATIC_ASSERT(offsetof(bist_interrupt_sw_result_t, completion_mask) ==
                             BIST_INTERRUPT_SW_RESULT_OFFSET_COMPLETION_MASK,
                             "SW interrupt result completion_mask offset changed");
BIST_INTERRUPT_STATIC_ASSERT(offsetof(bist_interrupt_sw_result_t, interrupt_count) ==
                             BIST_INTERRUPT_SW_RESULT_OFFSET_INTERRUPT_COUNT,
                             "SW interrupt result interrupt_count offset changed");
BIST_INTERRUPT_STATIC_ASSERT(offsetof(bist_interrupt_sw_result_t, enabled_mask_active) ==
                             BIST_INTERRUPT_SW_RESULT_OFFSET_ENABLED_MASK_ACTIVE,
                             "SW interrupt result enabled_mask_active offset changed");
BIST_INTERRUPT_STATIC_ASSERT(offsetof(bist_interrupt_sw_result_t, enabled_mask_cleared) ==
                             BIST_INTERRUPT_SW_RESULT_OFFSET_ENABLED_MASK_CLEARED,
                             "SW interrupt result enabled_mask_cleared offset changed");
BIST_INTERRUPT_STATIC_ASSERT(sizeof(bist_interrupt_sw_result_t) == BIST_INTERRUPT_SW_RESULT_TOTAL_SIZE,
                             "SW interrupt result size changed");
BIST_INTERRUPT_STATIC_ASSERT(BIST_INTERRUPT_SW_SOURCE_COUNT <= 32,
                             "SW interrupt completion_mask cannot hold every source");

BIST_INTERRUPT_STATIC_ASSERT(offsetof(bist_interrupt_hw_result_t, status) ==
                             BIST_INTERRUPT_HW_RESULT_OFFSET_STATUS,
                             "HW interrupt result status offset changed");
BIST_INTERRUPT_STATIC_ASSERT(offsetof(bist_interrupt_hw_result_t, count_1) ==
                             BIST_INTERRUPT_HW_RESULT_OFFSET_COUNT_1,
                             "HW interrupt result count_1 offset changed");
BIST_INTERRUPT_STATIC_ASSERT(offsetof(bist_interrupt_hw_result_t, count_2) ==
                             BIST_INTERRUPT_HW_RESULT_OFFSET_COUNT_2,
                             "HW interrupt result count_2 offset changed");
BIST_INTERRUPT_STATIC_ASSERT(sizeof(bist_interrupt_hw_result_t) == BIST_INTERRUPT_HW_RESULT_TOTAL_SIZE,
                             "HW interrupt result size changed");

#undef BIST_INTERRUPT_STATIC_ASSERT
/** @endcond */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Shared software interrupt result slots
 *
 * Slot 0 is written by HP core 0 and slot 1 by HP core 1. Both reside in HP
 * SRAM accessible to both cores. Volatile accesses do not provide
 * synchronization; ordering is provided by bist_multicore_call().
 */
extern volatile bist_interrupt_sw_result_t
bist_interrupt_sw_shared_results[BIST_INTERRUPT_RESULT_SLOT_COUNT];

/**
 * @brief Shared hardware interrupt result slots
 *
 * Slot 0 is written by HP core 0 and slot 1 by HP core 1. The cores use the
 * same TIMG0/TIMG1 hardware sequentially. Volatile accesses do not provide
 * synchronization; ordering is provided by bist_multicore_call().
 */
extern volatile bist_interrupt_hw_result_t
bist_interrupt_hw_shared_results[BIST_INTERRUPT_RESULT_SLOT_COUNT];

#ifdef __cplusplus
}
#endif

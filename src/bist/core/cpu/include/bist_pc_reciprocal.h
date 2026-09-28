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
 * @file bist_pc_reciprocal.h
 * @brief Fixed-layout evidence for the reciprocal Program Counter test
 *
 * Both HP cores run the same five-region call walk and publish
 * pointer-free evidence frames. In region order, the walk covers HP SRAM at
 * 0x4FFxxxxx, Flash at 0x400xxxxx (the start of .flash.text), Flash again
 * 0xFFF8 bytes after the first Flash function, LP SRAM at 0x5010xxxx, and TCM
 * at 0x3010xxxx. Each frame records the returned addresses for local and
 * cross-core validation against the canonical linked function addresses.
 */

#pragma once

#include "stddef.h"
#include "stdint.h"

/**
 * Number of PC test regions (HP SRAM, two Flash locations, LP SRAM, TCM).
 * Reciprocal PC testing is only supported on ESP32-P4.
 */
#define BIST_PC_RESULT_REGION_COUNT 5

/** completion_mask value once every region function has returned. */
#define BIST_PC_RESULT_COMPLETE_MASK ((UINT32_C(1) << BIST_PC_RESULT_REGION_COUNT) - 1)

/** Number of shared result slots (one per HP core). */
#define BIST_PC_RESULT_SLOT_COUNT 2

/** Expected byte size of the entire bist_pc_result_t structure. */
#define BIST_PC_RESULT_TOTAL_SIZE 28

/** Expected byte offset of status in bist_pc_result_t. */
#define BIST_PC_RESULT_OFFSET_STATUS           0

/** Expected byte offset of completion_mask in bist_pc_result_t. */
#define BIST_PC_RESULT_OFFSET_COMPLETION_MASK  4

/** Expected byte offset of the returned_address array in bist_pc_result_t. */
#define BIST_PC_RESULT_OFFSET_RETURNED_ADDRESS 8

/**
 * @brief Fixed-layout Program Counter test result
 *
 * returned_address[n] holds the address returned by the region n test
 * function, and bit n of completion_mask is set once that call has returned.
 * A completed walk has every bit in BIST_PC_RESULT_COMPLETE_MASK set. The
 * walk clears the frame and marks status as failed before calling any region
 * function, so an interrupted or incomplete walk never leaves a passing
 * frame.
 */
typedef struct {
    uint32_t status;                                          /**< bist_esp_err_t return value as a uint32_t. */
    uint32_t completion_mask;                                 /**< One bit per region function that returned. */
    uint32_t returned_address[BIST_PC_RESULT_REGION_COUNT];   /**< Address returned by each region function. */
} bist_pc_result_t;

/** @cond */
#if defined(__cplusplus)
#define BIST_PC_STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
#define BIST_PC_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#endif

BIST_PC_STATIC_ASSERT(offsetof(bist_pc_result_t, status) == BIST_PC_RESULT_OFFSET_STATUS,
                      "PC result status offset changed");
BIST_PC_STATIC_ASSERT(offsetof(bist_pc_result_t, completion_mask) == BIST_PC_RESULT_OFFSET_COMPLETION_MASK,
                      "PC result completion_mask offset changed");
BIST_PC_STATIC_ASSERT(offsetof(bist_pc_result_t, returned_address) == BIST_PC_RESULT_OFFSET_RETURNED_ADDRESS,
                      "PC result returned_address offset changed");
BIST_PC_STATIC_ASSERT(sizeof(bist_pc_result_t) == BIST_PC_RESULT_TOTAL_SIZE,
                      "PC result size changed");
BIST_PC_STATIC_ASSERT(BIST_PC_RESULT_REGION_COUNT <= 32,
                      "PC completion_mask cannot hold every region");

#undef BIST_PC_STATIC_ASSERT
/** @endcond */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Shared PC test result slots
 *
 * Slot 0 is written by HP core 0 and slot 1 by HP core 1. Both reside in HP
 * SRAM accessible to both cores. Volatile accesses do not provide
 * synchronization; ordering is provided by bist_multicore_call().
 */
extern volatile bist_pc_result_t bist_pc_shared_results[BIST_PC_RESULT_SLOT_COUNT];

#ifdef __cplusplus
}
#endif

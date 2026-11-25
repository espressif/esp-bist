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
 * @file bist_flash_reciprocal.h
 * @brief Fixed-layout evidence for the reciprocal flash CRC test
 *
 * Both HP cores CRC the same .flash.text and .flash.rodata sections and
 * publish pointer-free evidence frames. Each frame holds only the two CRC32
 * values computed by the producing core, so a frame can be validated by the
 * core that produced it as well as by its peer. The reference values used for
 * validation are the post-build CRC32 references that scripts/calculate_crc32.py
 * injects into .crc_section_text and .crc_section_data.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include "bist_esp_types.h"

/** Number of shared result slots (one per HP core). */
#define BIST_FLASH_RESULT_SLOT_COUNT 2

/** Expected byte size of the entire bist_flash_result_t structure. */
#define BIST_FLASH_RESULT_TOTAL_SIZE 12

/** Expected byte offset of status in bist_flash_result_t. */
#define BIST_FLASH_RESULT_OFFSET_STATUS     0

/** Expected byte offset of crc_rodata in bist_flash_result_t. */
#define BIST_FLASH_RESULT_OFFSET_CRC_RODATA 4

/** Expected byte offset of crc_text in bist_flash_result_t. */
#define BIST_FLASH_RESULT_OFFSET_CRC_TEXT   8

/**
 * @brief Fixed-layout reciprocal flash CRC result
 *
 * The frame is deliberately minimal: the two CRC32 values are the only
 * evidence the producing core can contribute. The stored CRC references and
 * the .flash.rodata / .flash.text lengths are derived from linker symbols, so
 * they are identical on both cores; exchanging them would compare a constant
 * against itself and would only add bytes that could disagree with the local
 * image. The verifier therefore re-reads those inputs locally instead of
 * trusting a peer copy.
 *
 * status is preset to the failure code before the walk starts, so an
 * interrupted or incomplete walk can never be mistaken for a passing frame.
 */
typedef struct {
    uint32_t status;      /**< bist_esp_err_t return value as uint32_t. */
    uint32_t crc_rodata;  /**< CRC32 computed by the producing core over .flash.rodata. */
    uint32_t crc_text;    /**< CRC32 computed by the producing core over .flash.text. */
} bist_flash_result_t;

/** @cond */
#if defined(__cplusplus)
#define BIST_FLASH_STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
#define BIST_FLASH_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#endif

BIST_FLASH_STATIC_ASSERT(offsetof(bist_flash_result_t, status) == BIST_FLASH_RESULT_OFFSET_STATUS,
                         "Flash result status offset changed");
BIST_FLASH_STATIC_ASSERT(offsetof(bist_flash_result_t, crc_rodata) == BIST_FLASH_RESULT_OFFSET_CRC_RODATA,
                         "Flash result crc_rodata offset changed");
BIST_FLASH_STATIC_ASSERT(offsetof(bist_flash_result_t, crc_text) == BIST_FLASH_RESULT_OFFSET_CRC_TEXT,
                         "Flash result crc_text offset changed");
BIST_FLASH_STATIC_ASSERT(sizeof(bist_flash_result_t) == BIST_FLASH_RESULT_TOTAL_SIZE,
                         "Flash result size changed");
BIST_FLASH_STATIC_ASSERT(BIST_FLASH_RESULT_SLOT_COUNT == 2,
                         "Flash result slot count changed");

#undef BIST_FLASH_STATIC_ASSERT
/** @endcond */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Shared reciprocal flash CRC result slots
 *
 * Slot 0 is written by HP core 0 and slot 1 by HP core 1. Both slots live in
 * HP SRAM, which is visible to both cores. The volatile qualifier prevents the
 * compiler from caching or eliding frame accesses; it provides no
 * synchronization, so ordering between the two cores comes from the blocking
 * bist_multicore_call() operations.
 */
extern volatile bist_flash_result_t bist_flash_shared_results[BIST_FLASH_RESULT_SLOT_COUNT];

#ifdef __cplusplus
}
#endif

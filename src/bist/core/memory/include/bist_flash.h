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
 * @file bist_flash.h
 * @brief Flash memory CRC validation test
 *
 * Validates flash integrity by computing CRC32 checksums of .flash.text
 * and .flash.rodata sections at runtime and comparing against stored
 * values injected during build (by scripts/calculate_crc32.py).
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "bist_esp_types.h"
#ifndef __ZEPHYR__
#include "bist_conf.h"
#endif
#include "sdkconfig.h"

/**
 * @brief Calculate CRC32 over a memory region in configured chunks
 *
 * Processes @p data in chunks of up to CONFIG_BIST_FLASH_TEST_CHUNK_SIZE bytes
 * using the IEEE 802.3 CRC-32 lookup table.
 *
 * @param data Pointer to the start of the memory buffer
 * @param length Number of bytes to checksum
 * @return Calculated 32-bit CRC value
 */
uint32_t calculate_crc32(uint8_t *data, size_t length);

/**
 * @brief Test flash memory integrity via CRC validation
 *
 * The STATIC and RECIPROCAL implementations expose the same bist_flash_test()
 * entry point. Both compute CRC32 for .flash.text and .flash.rodata in
 * CONFIG_BIST_FLASH_TEST_CHUNK_SIZE-byte chunks and compare against the
 * post-build references injected by scripts/calculate_crc32.py.
 *
 * In STATIC mode, the calling core computes and compares both CRCs.
 *
 * In RECIPROCAL mode, call from HP core 0. Both HP cores compute
 * the CRCs into fixed-layout slots in shared HP SRAM, then each core validates
 * its own slot and the other core's slot. Only core 0 may post work to core 1.
 *
 * @return BIST_ESP_OK if the selected mode completes and the CRCs match
 * @return BIST_ESP_FLASH_TEST_ERR on CRC mismatch, invalid reference, or
 *         multicore dispatch failure
 */
bist_esp_err_t bist_flash_test(void);

#if CONFIG_ESP_BIST_MEMORY_FLASH_TEST_RECIPROCAL
#include "bist_flash_reciprocal.h"
#endif

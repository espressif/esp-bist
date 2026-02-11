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
#include "sdkconfig.h"

/**
 * @brief Test flash memory integrity via CRC validation
 *
 * Computes CRC32 for:
 * - .flash.text section (executable code)
 * - .flash.rodata section (read-only data)
 *
 * Compares computed CRC against stored values (injected post-build).
 * Processes flash in 1024-byte chunks.
 *
 * @return BIST_ESP_OK if both CRCs match
 * @return BIST_ESP_FLASH_TEST_ERR if any CRC mismatch detected
 */
bist_esp_err_t bist_flash_test(void);

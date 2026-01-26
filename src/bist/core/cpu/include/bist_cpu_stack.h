/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * This file is part of Espressif's BIST (Built-In Self Test) Library.
 *
 * BIST library is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * BIST library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with BIST library. If not, see
 * <https://www.gnu.org/licenses/>.
 */

/**
 * @file bist_cpu_stack.h
 * @brief Stack overflow detection tests
 *
 * Implements runtime stack overflow detection using sentinel pattern
 * monitoring at the stack bottom. Tests verify the detection mechanism
 * works by intentionally causing controlled overflow.
 */

#pragma once

#include "stdint.h"
#include "bist_esp_types.h"

/**
 * @brief Initialize stack overflow detection
 *
 * Writes sentinel pattern (0xDEADBEEF) at the bottom of the stack
 * for subsequent overflow detection.
 *
 * @return BIST_ESP_OK on success
 */
bist_esp_err_t bist_cpu_stack_overflow_init(void);

/**
 * @brief Check for stack overflow
 *
 * Verifies sentinel pattern at stack bottom is intact. If corrupted,
 * indicates stack overflow has occurred.
 *
 * @return BIST_ESP_OK if sentinel intact
 * @return BIST_ESP_STACK_TEST_OVERFLOW if sentinel corrupted (overflow detected)
 */
bist_esp_err_t bist_cpu_stack_overflow_check(void);

/**
 * @brief Test stack overflow detection mechanism
 *
 * Performs bounded recursion (max 20000 iterations) with 128-byte
 * local buffers to intentionally trigger stack overflow. Checks
 * sentinel pattern each iteration to verify detection works.
 *
 * @return BIST_ESP_OK if overflow successfully detected
 * @return BIST_ESP_STACK_TEST_ERR if overflow detection fails
 */
bist_esp_err_t bist_cpu_stack_overflow_test(void);

/**
 * @brief Get the high watermark of the stack
 *
 * Calculates the high watermark of the stack by checking how much of the stack
 * still contains the predefined fill pattern.
 *
 * @return uint32_t High watermark value (in bytes)
 */
uint32_t bist_get_stack_high_watermark(void);

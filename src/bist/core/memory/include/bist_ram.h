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
 * @file bist_ram.h
 * @brief RAM integrity testing using March algorithms
 *
 * Implements March A, March X, and Abraham algorithms for detecting stuck-at,
 * transition, coupling, and address decoder faults in RAM. Tests are
 * non-destructive as original RAM content is backed up and restored.
 *
 * - March A: 3-step algorithm (Write 0, Read 0/Write 1, Read 1)
 * - March X: 6-step algorithm with additional descending address operations
 * - Abraham: 10-element / 30n-operation algorithm per IEC 60730-1 Annex H
 *   H.2.19.1, using time-division over partition pairs for inter-chunk
 *   coupling fault coverage without requiring a full-RAM backup buffer.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "bist_esp_types.h"
#ifndef __ZEPHYR__
#include "bist_conf.h"
#endif

/**
 * @brief Test RAM integrity using March A algorithm
 *
 * Executes 3-step March A algorithm:
 * 1. Write 0 to all cells (ascending)
 * 2. Read 0, write 1 (ascending)
 * 3. Read 1 from all cells (ascending)
 *
 * Backs up original RAM content before test and restores after.
 * Test region defined by linker symbols _bist_ram_test_start/_end.
 *
 * @return BIST_ESP_OK if RAM passes test
 * @return BIST_ESP_RAM_TEST_ERR if mismatch detected
 */
bist_esp_err_t bist_ram_test_march_a(void);

/**
 * @brief Test RAM integrity using March X algorithm
 *
 * Executes 6-step March X algorithm:
 * 1. Write 0 to all cells (ascending)
 * 2. Read 0, write 1 (ascending)
 * 3. Read 1 from all cells (ascending)
 * 4. Read 1, write 0 (descending)
 * 5. Read 0, write 1 (ascending)
 * 6. Read 1, write 0 (descending)
 *
 * More comprehensive than March A. Backs up and restores RAM content.
 * Test region defined by linker symbols _bist_ram_test_start/_end.
 *
 * @return BIST_ESP_OK if RAM passes test
 * @return BIST_ESP_RAM_TEST_ERR if mismatch detected
 */
bist_esp_err_t bist_ram_test_march_x(void);

/**
 * @brief Test RAM integrity using Abraham algorithm (one time-division period)
 *
 * Executes the IEC 60730-1 Annex H H.2.19.1 Abraham algorithm on one pair of
 * RAM partitions, then advances the internal pair index for the next call.
 * The algorithm uses 10 march elements (30 operations per cell) to detect:
 *   - Stuck-At Faults (SAF)
 *   - Transition Faults (TF)
 *   - Coupling Faults (CF) between cells in the tested pair
 *   - Address Decoder Faults (AF)
 *
 * After all C(N,2) pairs have been tested (one per call), the schedule wraps.
 * Backs up and restores RAM content for both partitions under test.
 *
 * @return BIST_ESP_OK if RAM passes test
 * @return BIST_ESP_RAM_TEST_ERR if mismatch detected
 */
bist_esp_err_t bist_ram_test_abraham(void);

/**
 * @brief Reset Abraham time-division pair schedule to the first pair
 *
 * Resets the internal pair index so the next call to bist_ram_test_abraham()
 * starts from pair (0,1). Call once at startup or before beginning a new
 * full coverage cycle.
 */
void bist_ram_test_abraham_reset(void);

/**
 * @brief Run Abraham algorithm over all partition pairs (full coverage)
 *
 * Resets the pair schedule and runs bist_ram_test_abraham() for every
 * C(N,2) partition pair. Suitable for post-boot thorough RAM verification.
 * Returns on the first failure encountered.
 *
 * @return BIST_ESP_OK if all pairs pass
 * @return BIST_ESP_RAM_TEST_ERR if any pair fails
 */
bist_esp_err_t bist_ram_test_abraham_full(void);

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
 * @file bist_ram.h
 * @brief RAM integrity testing using March algorithms
 *
 * Implements March A and March X algorithms for detecting stuck-at
 * and transition faults in RAM. Tests are non-destructive as original
 * RAM content is backed up and restored.
 *
 * - March A: 3-step algorithm (Write 0, Read 0/Write 1, Read 1)
 * - March X: 6-step algorithm with additional descending address operations
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

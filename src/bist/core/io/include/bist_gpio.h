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
 * @file bist_gpio.h
 * @brief GPIO plausibility tests
 *
 * Tests GPIO output and input functionality to detect stuck-at faults
 * and verify proper pin configuration and operation.
 */

#pragma once

#include "stdint.h"
#include "bist_esp_types.h"
#include "gpio.h"

/**
 * @brief Test GPIO output functionality
 *
 * Verifies GPIO pin can be driven to both logic levels and read back correctly:
 * 1. Validates GPIO number
 * 2. Resets pin to default state
 * 3. Configures as input/output mode
 * 4. Sets level to 0, reads back (expects 0)
 * 5. Sets level to 1, reads back (expects 1)
 * 6. Resets pin after test
 *
 * @param gpio_num GPIO pin number to test
 *
 * @return BIST_ESP_OK if output levels read back correctly
 * @return BIST_ESP_IO_TEST_ERR if invalid GPIO, configuration fails, or level mismatch
 */
bist_esp_err_t bist_gpio_output_test(gpio_num_t gpio_num);

/**
 * @brief Test GPIO input functionality
 *
 * Verifies GPIO configured as input reads expected logic level:
 * 1. Validates GPIO number
 * 2. Resets pin to default state
 * 3. Configures as input mode
 * 4. Reads level and compares against expected_level
 * 5. Resets pin after test
 *
 * @param gpio_num GPIO pin number to test
 * @param expected_level Expected logic level (0 or 1)
 *
 * @return BIST_ESP_OK if read level matches expected
 * @return BIST_ESP_IO_TEST_ERR if invalid GPIO, configuration fails, or level mismatch
 *
 * @note Requires external hardware setup to provide known logic level
 */
bist_esp_err_t bist_gpio_input_test(gpio_num_t gpio_num, bool expected_level);

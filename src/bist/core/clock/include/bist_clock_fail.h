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
 * @file bist_clock_fail.h
 * @brief Clock integrity tests
 *
 * Validates external 32.768 kHz crystal and main 40 MHz crystal
 * operation and frequency accuracy.
 */

#pragma once

#include "stdint.h"
#include "bist_esp_types.h"

/**
 * @brief Test external 32.768 kHz crystal operation
 *
 * Monitors external oscillator using XT WDT (external crystal watchdog)
 * with 200-cycle timeout. Waits 2 ms for monitoring. If crystal fails,
 * watchdog callback fires.
 *
 * @return BIST_ESP_OK if crystal operational (no callback triggered)
 * @return BIST_ESP_CLOCK_TEST_ERR if crystal failure detected
 *
 * @note Requires hardware with external 32 kHz crystal installed
 */
bist_esp_err_t bist_ext_crystal_fail_test(void);

/**
 * @brief Test main 40 MHz crystal frequency accuracy
 *
 * Measures actual crystal frequency by calculating ratio between
 * 32.768 kHz reference and 40 MHz crystal over 500 cycles.
 * Computes frequency deviation percentage and compares against
 * configured tolerance (CONFIG_ESP_BIST_CLOCK_PERCENT_FREQUENCY_DRIFT,
 * default ±1%).
 *
 * @return BIST_ESP_OK if frequency within tolerance
 * @return BIST_ESP_CLOCK_TEST_ERR if deviation exceeds tolerance
 *
 * @note Requires functional 32 kHz crystal as reference
 */
bist_esp_err_t bist_main_crystal_test(void);

/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
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
 * @file bist_adc.h
 * @brief ADC plausibility tests
 *
 * Tests ADC input functionality to detect stuck-at faults
 * and verify proper pin configuration and operation.
 */

#pragma once

#include "stdint.h"
#include "bist_esp_types.h"
#include "adc_oneshot.h"

/**
 * @brief Test ADC channel with internal pull-down (low level)
 *
 * @param unit ADC unit to test
 * @param channel ADC channel to test
 *
 * @return BIST_ESP_OK if raw value is within tolerance of the expected low level
 * @return BIST_ESP_ADC_TEST_ERR if parameters, configuration, or reading fails
 */
bist_esp_err_t bist_adc_low_level_test(adc_unit_t unit, adc_channel_t channel);

/**
 * @brief Test ADC channel with internal pull-up (high level)
 *
 * @param unit ADC unit to test
 * @param channel ADC channel to test
 *
 * @return BIST_ESP_OK if raw value is within tolerance of the expected high level
 * @return BIST_ESP_ADC_TEST_ERR if parameters, configuration, or reading fails
 */
bist_esp_err_t bist_adc_high_level_test(adc_unit_t unit, adc_channel_t channel);

#if defined(SOC_TARGET_ESP32C3)
/**
 * @brief Test ADC channel at an intermediate reference level (ESP32-C3 only)
 *
 * Enables both internal pull-up and pull-down to bias the pin near mid-scale,
 * then verifies the reading is neither stuck at the low nor high level.
 *
 * @param unit ADC unit to test
 * @param channel ADC channel to test
 *
 * @return BIST_ESP_OK if raw value is within the expected reference range
 * @return BIST_ESP_ADC_TEST_ERR if parameters, configuration, or reading fails
 */
bist_esp_err_t bist_adc_reference_test(adc_unit_t unit, adc_channel_t channel);
#endif

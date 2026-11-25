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
 * @file main.c
 * @brief Unity app for the reciprocal GPIO input test
 *
 * Configures GPIO pins with the same driver sequence as bist_gpio_output_test()
 * and bist_gpio_input_test(), then cross-compares samples from both HP cores
 * via bist_gpio_input_reciprocal_test(). For the failure case,
 * pytest_device_digital_io_test_reciprocal.py injects a level mismatch into one
 * evidence slot before comparison.
 */

#include <stdio.h>
#include "bist_esp.h"
#include "unity.h"

#if defined(SOC_TARGET_ESP32P4)

#define BIST_TEST_GPIO_EXT_OUT_IO (2)
#define BIST_TEST_GPIO_INPUT_IO   (8)

void setUp(void) {}

void tearDown(void) {}

static void run_gpio_output_loopback_reciprocal_test(gpio_num_t gpio_num)
{
    TEST_ASSERT_EQUAL(ESP_OK, gpio_reset_pin(gpio_num));
    TEST_ASSERT_EQUAL(ESP_OK, gpio_set_direction(gpio_num, GPIO_MODE_INPUT_OUTPUT));

    TEST_ASSERT_EQUAL(ESP_OK, gpio_set_level(gpio_num, 0));
    TEST_ASSERT_EQUAL(BIST_ESP_OK, bist_gpio_input_reciprocal_test(gpio_num));

    TEST_ASSERT_EQUAL(ESP_OK, gpio_set_level(gpio_num, 1));
    TEST_ASSERT_EQUAL(BIST_ESP_OK, bist_gpio_input_reciprocal_test(gpio_num));

    TEST_ASSERT_EQUAL(ESP_OK, gpio_reset_pin(gpio_num));
}

static void run_gpio_input_reciprocal_test(gpio_num_t gpio_num)
{
    TEST_ASSERT_EQUAL(ESP_OK, gpio_reset_pin(gpio_num));
    TEST_ASSERT_EQUAL(ESP_OK, gpio_set_direction(gpio_num, GPIO_MODE_INPUT));

    TEST_ASSERT_EQUAL(BIST_ESP_OK, bist_gpio_input_reciprocal_test(gpio_num));

    TEST_ASSERT_EQUAL(ESP_OK, gpio_reset_pin(gpio_num));
}

void test_BIST_Gpio_Input_Reciprocal(void)
{
    run_gpio_output_loopback_reciprocal_test(BIST_TEST_GPIO_EXT_OUT_IO);
    run_gpio_input_reciprocal_test(BIST_TEST_GPIO_INPUT_IO);
}

void test_BIST_Gpio_Input_Reciprocal_Invalid_Pin(void)
{
    bist_esp_err_t ret = bist_gpio_input_reciprocal_test(SOC_GPIO_PIN_COUNT);
    TEST_ASSERT_EQUAL(BIST_ESP_IO_TEST_ERR, ret);

    ret = bist_gpio_input_reciprocal_test(-1);
    TEST_ASSERT_EQUAL(BIST_ESP_IO_TEST_ERR, ret);
}

#endif

int main(void)
{
    UNITY_BEGIN();
#if defined(SOC_TARGET_ESP32P4)
    RUN_TEST(test_BIST_Gpio_Input_Reciprocal);
    RUN_TEST(test_BIST_Gpio_Input_Reciprocal_Invalid_Pin);
#endif
    return UNITY_END();
}

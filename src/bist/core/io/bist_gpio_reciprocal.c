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
 * @file bist_gpio_reciprocal.c
 * @brief Reciprocal GPIO input test for ESP32-P4
 *
 * Implements IEC 60730 H.2.18.15 reciprocal comparison for one digital input.
 * Both HP cores read the same pin into their own fixed-layout slot, and each
 * core then compares the two samples. The walk only calls gpio_get_level(): it
 * never resets a pin, changes its direction, or drives a level, so the pin
 * keeps the configuration the application gave it.
 */

#include "bist_gpio_reciprocal.h"
#include "bist_conf.h"
#include "bist_log.h"
#include "bist_multicore.h"
#include "gpio.h"

#ifndef SOC_TARGET_ESP32P4
# error "Reciprocal GPIO test is only supported on ESP32-P4"
#endif

static const char *TAG = "bist.gpio.reciprocal";

volatile bist_gpio_input_result_t bist_gpio_input_shared_results[BIST_GPIO_INPUT_RESULT_SLOT_COUNT]
__attribute__((aligned(8)));

/* Pin sampled by both cores. bist_multicore_call() blocks, so only one sample
 * runs at a time and core 1 reads this through the same image. */
static gpio_num_t s_gpio_input_pin;

/*
 * Collect-only sample: one pin read per call, no comparison and no logging.
 * Core 1 receives the slot pointer as its only argument. The status is preset
 * to the failure code so a truncated sample can never look like a pass.
 */
static void __attribute__((noinline, noclone)) bist_gpio_input_sample_job(void *arg)
{
    volatile bist_gpio_input_result_t *result = (volatile bist_gpio_input_result_t *)arg;

    if (result == NULL) {
        return;
    }

    result->level = 0;
    result->status = (uint32_t)BIST_ESP_IO_TEST_ERR;

    BIST_ADD_LABEL("bist_gpio_input_sample");

    result->level = (uint32_t)gpio_get_level(s_gpio_input_pin);
    result->status = (uint32_t)BIST_ESP_OK;
}

/*
 * Pure evidence check: no hardware access and no logging, so either core can
 * run it. A NULL slot or a producer status still at its fail-closed value
 * rejects the pair.
 */
static bist_esp_err_t verify_gpio_input(volatile bist_gpio_input_result_t *slot_a,
                                        volatile bist_gpio_input_result_t *slot_b)
{
    if (slot_a == NULL || slot_b == NULL) {
        return BIST_ESP_IO_TEST_ERR;
    }

    if (slot_a->status != (uint32_t)BIST_ESP_OK || slot_b->status != (uint32_t)BIST_ESP_OK) {
        return BIST_ESP_IO_TEST_ERR;
    }

    if (slot_a->level > 1U || slot_b->level > 1U) {
        return BIST_ESP_IO_TEST_ERR;
    }

    if (slot_a->level != slot_b->level) {
        return BIST_ESP_IO_TEST_ERR;
    }

    return BIST_ESP_OK;
}

static void verify_gpio_input_job(void *arg)
{
    volatile bist_gpio_input_result_t *result = (volatile bist_gpio_input_result_t *)arg;

    if (result == NULL) {
        return;
    }

    result->status = (uint32_t)verify_gpio_input(&bist_gpio_input_shared_results[0],
                                                 &bist_gpio_input_shared_results[1]);
}

bist_esp_err_t bist_gpio_input_reciprocal_test(gpio_num_t gpio_num)
{
    volatile bist_gpio_input_result_t *test_result_core_0 = &bist_gpio_input_shared_results[0];
    volatile bist_gpio_input_result_t *test_result_core_1 = &bist_gpio_input_shared_results[1];
    bist_esp_err_t status;
    bist_esp_err_t ret;

    if (!GPIO_IS_VALID_GPIO(gpio_num)) {
        ESP_LOGE(TAG, "GPIO number error: %d", (int)gpio_num);
        return BIST_ESP_IO_TEST_ERR;
    }

    s_gpio_input_pin = gpio_num;

    /* Sample on core 0 */
    bist_gpio_input_sample_job((void *)test_result_core_0);
    if (test_result_core_0->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "Core 0 could not sample GPIO %d", (int)gpio_num);
        return BIST_ESP_IO_TEST_ERR;
    }

    /* Sample on core 1 */
    ret = bist_multicore_call(1, bist_gpio_input_sample_job, (void *)test_result_core_1);
    if (ret != BIST_ESP_OK) {
        ESP_LOGE(TAG, "Sampling dispatch to core 1 failed (%d)", ret);
        return BIST_ESP_IO_TEST_ERR;
    }
    if (test_result_core_1->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "Core 1 could not sample GPIO %d", (int)gpio_num);
        return BIST_ESP_IO_TEST_ERR;
    }

    /* Cross-validate both samples on core 1 */
    BIST_ADD_LABEL("bist_gpio_input_verify");
    ret = bist_multicore_call(1, verify_gpio_input_job, (void *)test_result_core_1);
    if (ret != BIST_ESP_OK) {
        ESP_LOGE(TAG, "Cross-check dispatch to core 1 failed (%d)", ret);
        return BIST_ESP_IO_TEST_ERR;
    }
    if (test_result_core_1->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "GPIO %d rejected on core 1: level %u on core 0, %u on core 1", (int)gpio_num,
                 (unsigned)test_result_core_0->level, (unsigned)test_result_core_1->level);
        return BIST_ESP_IO_TEST_ERR;
    }

    /* Cross-validate both samples on core 0 */
    status = verify_gpio_input(test_result_core_0, test_result_core_1);
    if (status != BIST_ESP_OK) {
        ESP_LOGE(TAG, "GPIO %d rejected on core 0: level %u on core 0, %u on core 1", (int)gpio_num,
                 (unsigned)test_result_core_0->level, (unsigned)test_result_core_1->level);
        return BIST_ESP_IO_TEST_ERR;
    }

    return BIST_ESP_OK;
}

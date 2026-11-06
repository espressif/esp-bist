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
 * @file bist_gpio_reciprocal.h
 * @brief Fixed-layout evidence for the reciprocal GPIO input test
 *
 * Defines the pointer-free result layout and shared storage used to exchange
 * one input-pin sample per HP core. Each core reads the same pin into its own
 * slot; the two levels are then cross-validated on both cores.
 */

#pragma once

#include "stddef.h"
#include "stdint.h"
#include "bist_esp_types.h"
#include "gpio.h"

/** Number of shared result slots (one per HP core). */
#define BIST_GPIO_INPUT_RESULT_SLOT_COUNT 2

/** Expected byte size of bist_gpio_input_result_t. */
#define BIST_GPIO_INPUT_RESULT_TOTAL_SIZE 8

/** Expected byte offset of status in bist_gpio_input_result_t. */
#define BIST_GPIO_INPUT_RESULT_OFFSET_STATUS 0

/** Expected byte offset of level in bist_gpio_input_result_t. */
#define BIST_GPIO_INPUT_RESULT_OFFSET_LEVEL 4

/**
 * @brief Fixed-layout reciprocal GPIO input sample
 *
 * The frame carries the sample of one pin taken on one HP core: Core 0 fills
 * slot 0 and Core 1 fills slot 1. status is preset to the failure code before
 * the read, so an interrupted or incomplete sample can never be mistaken for a
 * passing frame.
 */
typedef struct {
    uint32_t status; /**< bist_esp_err_t return value as uint32_t. */
    uint32_t level;  /**< Sampled pin level, 0 or 1. */
} bist_gpio_input_result_t;

/** @cond */
#if defined(__cplusplus)
#define BIST_GPIO_INPUT_STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
#define BIST_GPIO_INPUT_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#endif

BIST_GPIO_INPUT_STATIC_ASSERT(offsetof(bist_gpio_input_result_t, status) == BIST_GPIO_INPUT_RESULT_OFFSET_STATUS,
                              "GPIO input result status offset changed");
BIST_GPIO_INPUT_STATIC_ASSERT(offsetof(bist_gpio_input_result_t, level) == BIST_GPIO_INPUT_RESULT_OFFSET_LEVEL,
                              "GPIO input result level offset changed");
BIST_GPIO_INPUT_STATIC_ASSERT(sizeof(bist_gpio_input_result_t) == BIST_GPIO_INPUT_RESULT_TOTAL_SIZE,
                              "GPIO input result size changed");
BIST_GPIO_INPUT_STATIC_ASSERT(BIST_GPIO_INPUT_RESULT_SLOT_COUNT == 2, "GPIO input result slot count changed");

#undef BIST_GPIO_INPUT_STATIC_ASSERT
/** @endcond */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Shared reciprocal GPIO input result slots
 *
 * Slot 0 is written by HP core 0 and slot 1 by HP core 1. The volatile
 * qualifier prevents the compiler from caching or eliding frame accesses; it
 * provides no synchronization, so ordering between the two cores comes from
 * the blocking bist_multicore_call() operations.
 */
extern volatile bist_gpio_input_result_t bist_gpio_input_shared_results[BIST_GPIO_INPUT_RESULT_SLOT_COUNT];

/**
 * @brief Run the reciprocal GPIO input test for one pin
 *
 * Core 0 and Core 1 each read the level of @p gpio_num into their own evidence
 * slot, then both cores compare the two samples. The walk only reads the pin:
 * it never resets it, changes its direction, or drives a level, so the pin
 * keeps the configuration the application gave it.
 *
 * @param gpio_num GPIO number sampled by both HP cores
 *
 * @return BIST_ESP_OK if both samples completed and reported the same level
 * @return BIST_ESP_IO_TEST_ERR if the pin is invalid, a dispatch failed, a
 *         sample did not complete, a level was neither 0 nor 1, or the two
 *         levels differ
 *
 * @note The pin must already have its input buffer enabled. The application
 *       must configure a functional output as GPIO_MODE_INPUT_OUTPUT, because
 *       gpio_get_level() returns 0 when a pad is not an input.
 * @note Call from HP core 0. Only Core 0 may post work to Core 1.
 */
bist_esp_err_t bist_gpio_input_reciprocal_test(gpio_num_t gpio_num);

#ifdef __cplusplus
}
#endif

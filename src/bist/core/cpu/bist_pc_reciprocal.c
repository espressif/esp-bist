/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * This file is part of Espressif's BIST (Built-In Self Test) Library.
 *
 * BIST library is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * BIST library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with BIST library. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "bist_pc.h"
#include "bist_pc_reciprocal.h"
#include "bist_multicore.h"
#include "esp_attr.h"
#include "esp_log.h"

#ifndef SOC_TARGET_ESP32P4
# error "Reciprocal Program Counter test is only supported on ESP32-P4"
#endif

#ifndef CONFIG_ESP_BIST_MULTICORE
# error "Reciprocal Program Counter test requires CONFIG_ESP_BIST_MULTICORE"
#endif

/*
 * Reciprocal Program Counter testing
 *
 * Uses the same region functions and linker placement as the static test
 * (see bist_pc.c). HP core 0 and HP core 1 share one physical HP SPM (TCM) at
 * 0x30100000, so pcTestFunction4 executes at the same address on both cores.
 */

typedef void *(*bist_pc_test_fn_t)(void);

__attribute__((section(".pc_test_0"))) static void *pcTestFunction0(void);
__attribute__((section(".pc_test_1"))) static void *pcTestFunction1(void);
__attribute__((section(".pc_test_2"))) static void *pcTestFunction2(void);
__attribute__((section(".pc_test_3"))) static void *pcTestFunction3(void);
__attribute__((section(".pc_test_4"))) static void *pcTestFunction4(void);

static const char *TAG = "BIST_PC_TEST";

volatile bist_pc_result_t bist_pc_shared_results[BIST_PC_RESULT_SLOT_COUNT] __attribute__((aligned(8)));

/* Canonical linked addresses, independent of the walk's call table. */
static const bist_pc_test_fn_t s_pc_expected_functions[BIST_PC_RESULT_REGION_COUNT] = {
    pcTestFunction0,
    pcTestFunction1,
    pcTestFunction2,
    pcTestFunction3,
    pcTestFunction4,
};

/*
 * noclone keeps the fault-injection labels unique. The call table is volatile
 * so every call and comparison reads the stack copy that GDB corrupts.
 */
static void IRAM_ATTR __attribute__((noinline, noclone)) bist_pc_test_job(void *arg)
{
    volatile bist_pc_result_t *out = (volatile bist_pc_result_t *)arg;
    bist_pc_test_fn_t volatile pcTestFunctions[BIST_PC_RESULT_REGION_COUNT] = {
        pcTestFunction0,
        pcTestFunction1,
        pcTestFunction2,
        pcTestFunction3,
        pcTestFunction4,
    };
    volatile uint32_t *words;
    uintptr_t returnFunctionAddress;

    if (out == NULL) {
        return;
    }

    words = (volatile uint32_t *)out;
    for (uint32_t i = 0; i < (sizeof(*out) / sizeof(*words)); ++i) {
        words[i] = 0;
    }
    out->status = (uint32_t)BIST_ESP_PC_TEST_ERR;

    ASM("" ::: "memory");
    BIST_ADD_LABEL("bist_pc_walk");
    for (uint32_t i = 0; i < BIST_PC_RESULT_REGION_COUNT; ++i) {
        returnFunctionAddress = (uintptr_t)(*pcTestFunctions[i])();
        out->returned_address[i] = (uint32_t)returnFunctionAddress;
        out->completion_mask |= UINT32_C(1) << i;
        if ((uintptr_t)pcTestFunctions[i] != returnFunctionAddress) {
            return;
        }
    }

    out->status = (uint32_t)BIST_ESP_OK;
}

static bist_esp_err_t IRAM_ATTR verify_bist_pc_test_result(volatile bist_pc_result_t *result)
{
    if (result == NULL || result->status != (uint32_t)BIST_ESP_OK ||
            result->completion_mask != BIST_PC_RESULT_COMPLETE_MASK) {
        return BIST_ESP_PC_TEST_ERR;
    }

    for (uint32_t i = 0; i < BIST_PC_RESULT_REGION_COUNT; ++i) {
        if (result->returned_address[i] != (uint32_t)(uintptr_t)s_pc_expected_functions[i]) {
            return BIST_ESP_PC_TEST_ERR;
        }
    }

    return BIST_ESP_OK;
}

static void IRAM_ATTR verify_bist_pc_test_result_job(void *arg)
{
    volatile bist_pc_result_t *result = (volatile bist_pc_result_t *)arg;

    if (result == NULL) {
        return;
    }

    result->status = (uint32_t)verify_bist_pc_test_result(result);
}

bist_esp_err_t IRAM_ATTR bist_pc_test(void)
{
    volatile bist_pc_result_t *test_result_core_0 = &bist_pc_shared_results[0];
    volatile bist_pc_result_t *test_result_core_1 = &bist_pc_shared_results[1];
    bist_esp_err_t status;
    int ret;

    /* Execute and validate test on core 0 */

    bist_pc_test_job((void *)test_result_core_0);
    if (test_result_core_0->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to execute PC walk on core 0");
        return BIST_ESP_PC_TEST_ERR;
    }

    status = verify_bist_pc_test_result(test_result_core_0);
    if (status != BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to validate core 0 PC results on core 0");
        return BIST_ESP_PC_TEST_ERR;
    }

    /* Execute and validate test on core 1 */

    ret = bist_multicore_call(1, bist_pc_test_job, (void *)test_result_core_1);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to dispatch PC walk to core 1");
        return BIST_ESP_PC_TEST_ERR;
    }
    if (test_result_core_1->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to execute PC walk on core 1");
        return BIST_ESP_PC_TEST_ERR;
    }

    ret = bist_multicore_call(1, verify_bist_pc_test_result_job, (void *)test_result_core_1);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to dispatch validation of core 1 PC results to core 1");
        return BIST_ESP_PC_TEST_ERR;
    }
    if (test_result_core_1->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to validate core 1 PC results on core 1");
        return BIST_ESP_PC_TEST_ERR;
    }

    /* Validate core 0 results on core 1 */

    BIST_ADD_LABEL("bist_pc_verify_core0_on_core1");
    ret = bist_multicore_call(1, verify_bist_pc_test_result_job, (void *)test_result_core_0);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to dispatch validation of core 0 PC results to core 1");
        return BIST_ESP_PC_TEST_ERR;
    }
    if (test_result_core_0->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to validate core 0 PC results on core 1");
        return BIST_ESP_PC_TEST_ERR;
    }

    /* Validate core 1 results on core 0 */

    BIST_ADD_LABEL("bist_pc_verify_core1_on_core0");
    status = verify_bist_pc_test_result(test_result_core_1);
    if (status != BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to validate core 1 PC results on core 0");
        return BIST_ESP_PC_TEST_ERR;
    }

    return BIST_ESP_OK;
}

void *pcTestFunction0(void)
{
    return (pcTestFunction0);
}

void *pcTestFunction1(void)
{
    return (pcTestFunction1);
}

void *pcTestFunction2(void)
{
    return (pcTestFunction2);
}

void *pcTestFunction3(void)
{
    return (pcTestFunction3);
}

void *pcTestFunction4(void)
{
    return (pcTestFunction4);
}

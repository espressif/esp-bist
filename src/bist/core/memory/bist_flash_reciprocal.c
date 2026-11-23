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

/**
 * @file bist_flash_reciprocal.c
 * @brief Reciprocal flash CRC test for ESP32-P4
 *
 * Same flash integrity test as bist_flash.c, restructured as an IEC 60730
 * H.2.18.15 reciprocal test. Each HP core CRC32s .flash.rodata and
 * .flash.text and publishes the two values in its own shared frame. The walk
 * only collects evidence; every comparison happens afterwards in
 * verify_bist_flash_result(), which is executed by the producing core and by
 * its peer, so the CRC logic is cross-checked between the two cores.
 *
 * The frames hold no pointers and no linker-derived constants: the stored CRC
 * references and the section lengths are identical on both cores, and a
 * verifier that read them from a peer frame would only compare a constant with
 * itself. The verifier instead re-reads them locally on the core that runs the
 * check.
 */

#include "bist_flash.h"
#include "bist_flash_reciprocal.h"
#include "bist_conf.h"
#include "bist_log.h"
#include "bist_multicore.h"

#ifndef SOC_TARGET_ESP32P4
# error "Reciprocal flash CRC test is only supported on ESP32-P4"
#endif
#ifndef CONFIG_ESP_BIST_MULTICORE
# error "Reciprocal flash CRC test requires CONFIG_ESP_BIST_MULTICORE"
#endif

// Pointer to CRC32 value
extern uint32_t _crc_section_text_start, _crc_section_data_start;

// Flash sections
extern uint32_t _flash_rodata_start, _flash_rodata_end;
extern uint32_t _flash_text_start, _flash_text_end;

static const char *TAG = "bist.flash.reciprocal";

volatile bist_flash_result_t bist_flash_shared_results[BIST_FLASH_RESULT_SLOT_COUNT] __attribute__((aligned(8)));

static void clear_flash_result(volatile bist_flash_result_t *result)
{
    volatile uint32_t *words = (volatile uint32_t *)result;

    for (uint32_t i = 0; i < (sizeof(*result) / sizeof(*words)); ++i) {
        words[i] = 0;
    }
}

/*
 * Collect-only walk: no comparisons and no early error returns, because the
 * evidence must reach the peer verifier even when it is wrong. noclone keeps
 * the fault-injection labels unique. The section length stays a volatile
 * local so the CRC call always loads it from the stack; the test suite
 * overwrites it through GDB at the labels to prove a wrong length is caught.
 */
static void __attribute__((noinline, noclone)) bist_flash_walk_job(void *arg)
{
    volatile bist_flash_result_t *out = (volatile bist_flash_result_t *)arg;
    volatile size_t crc_section_len = 0;

    if (out == NULL) {
        return;
    }

    clear_flash_result(out);
    out->status = (uint32_t)BIST_ESP_FLASH_TEST_ERR;

    ASM("" ::: "memory");
    BIST_ADD_LABEL("bist_flash_walk");

    crc_section_len = (size_t)(&_flash_rodata_end - &_flash_rodata_start) * 4;
    BIST_ADD_LABEL("bist_flash_walk_rodata");
    out->crc_rodata = calculate_crc32((uint8_t *)&_flash_rodata_start, crc_section_len);

    crc_section_len = (size_t)(&_flash_text_end - &_flash_text_start) * 4;
    BIST_ADD_LABEL("bist_flash_walk_text");
    out->crc_text = calculate_crc32((uint8_t *)&_flash_text_start, crc_section_len);

    out->status = (uint32_t)BIST_ESP_OK;
}

/*
 * Pure evidence check. The CRC references are re-read from the local image
 * instead of being exchanged through the frame: they come from the linker and
 * are therefore the same on both cores, so a peer copy would only confirm the
 * peer agrees with its own constant.
 */
static bist_esp_err_t verify_bist_flash_result(volatile bist_flash_result_t *result)
{
    uint32_t ref_rodata;
    uint32_t ref_text;

    if (result == NULL || result->status != (uint32_t)BIST_ESP_OK) {
        return BIST_ESP_FLASH_TEST_ERR;
    }

    /* .crc_section_data holds the .flash.rodata reference, .crc_section_text the .flash.text one. */
    ref_rodata = _crc_section_data_start;
    ref_text = _crc_section_text_start;

    /*
     * A zero reference means the post-build step never injected the CRC32
     * values, so a zero CRC would otherwise be accepted on an empty image.
     */
    if (ref_rodata == 0U || ref_text == 0U) {
        return BIST_ESP_FLASH_TEST_ERR;
    }

    if (result->crc_rodata != ref_rodata) {
        return BIST_ESP_FLASH_TEST_ERR;
    }

    if (result->crc_text != ref_text) {
        return BIST_ESP_FLASH_TEST_ERR;
    }

    return BIST_ESP_OK;
}

static void verify_bist_flash_result_job(void *arg)
{
    volatile bist_flash_result_t *result = (volatile bist_flash_result_t *)arg;

    if (result == NULL) {
        return;
    }

    result->status = (uint32_t)verify_bist_flash_result(result);
}

bist_esp_err_t bist_flash_test(void)
{
    volatile bist_flash_result_t *test_result_core_0 = &bist_flash_shared_results[0];
    volatile bist_flash_result_t *test_result_core_1 = &bist_flash_shared_results[1];
    bist_esp_err_t status;
    bist_esp_err_t ret;

    /* Execute and validate test on core 0 */

    bist_flash_walk_job((void *)test_result_core_0);
    if (test_result_core_0->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to execute flash walk on core 0");
        return BIST_ESP_FLASH_TEST_ERR;
    }

    status = verify_bist_flash_result(test_result_core_0);
    if (status != BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to validate core 0 flash results on core 0");
        return BIST_ESP_FLASH_TEST_ERR;
    }

    /* Execute and validate test on core 1 */

    ret = bist_multicore_call(1, bist_flash_walk_job, (void *)test_result_core_1);
    if (ret != BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to dispatch flash walk to core 1");
        return BIST_ESP_FLASH_TEST_ERR;
    }
    if (test_result_core_1->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to execute flash walk on core 1");
        return BIST_ESP_FLASH_TEST_ERR;
    }

    ret = bist_multicore_call(1, verify_bist_flash_result_job, (void *)test_result_core_1);
    if (ret != BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to dispatch validation of core 1 flash results to core 1");
        return BIST_ESP_FLASH_TEST_ERR;
    }
    if (test_result_core_1->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to validate core 1 flash results on core 1");
        return BIST_ESP_FLASH_TEST_ERR;
    }

    /* Validate core 0 results on core 1 */

    BIST_ADD_LABEL("bist_flash_verify_core0_on_core1");
    ret = bist_multicore_call(1, verify_bist_flash_result_job, (void *)test_result_core_0);
    if (ret != BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to dispatch validation of core 0 flash results to core 1");
        return BIST_ESP_FLASH_TEST_ERR;
    }
    if (test_result_core_0->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to validate core 0 flash results on core 1");
        return BIST_ESP_FLASH_TEST_ERR;
    }

    /* Validate core 1 results on core 0 */

    BIST_ADD_LABEL("bist_flash_verify_core1_on_core0");
    status = verify_bist_flash_result(test_result_core_1);
    if (status != BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to validate core 1 flash results on core 0");
        return BIST_ESP_FLASH_TEST_ERR;
    }

    ESP_LOGD(TAG, "Core 0 CRC: 0x%08x", test_result_core_0->crc_rodata);
    ESP_LOGD(TAG, "Core 1 CRC: 0x%08x", test_result_core_1->crc_rodata);
    ESP_LOGD(TAG, "Core 0 text CRC: 0x%08x", test_result_core_0->crc_text);
    ESP_LOGD(TAG, "Core 1 text CRC: 0x%08x", test_result_core_1->crc_text);

    return BIST_ESP_OK;
}

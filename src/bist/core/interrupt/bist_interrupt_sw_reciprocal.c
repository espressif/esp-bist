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

#include "bist_interrupt.h"
#include "bist_multicore.h"
#include "esp_cpu.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "soc/system_intr.h"
#include "soc/system_reg.h"

#ifndef SOC_TARGET_ESP32P4
# error "Reciprocal software interrupt source-map test is only supported on ESP32-P4"
#endif

#ifndef CONFIG_ESP_BIST_MULTICORE
# error "Reciprocal software interrupt source-map test requires CONFIG_ESP_BIST_MULTICORE"
#endif

/*
 * Reciprocal software interrupt source-map test (ESP32-P4 only).
 *
 * Collects pointer-free evidence per source into a shared slot, then
 * cross-validates each slot on both HP cores via bist_multicore_call().
 *
 * CPU_INTR_FROM_CPU_3 is intentionally not tested here. On ESP32-P4 standalone
 * multicore builds, bist_multicore_call() wakes Core 1 by asserting
 * SYSTEM_CPU_INTR_FROM_CPU_3 and routing SYS_CPU_INTR_FROM_CPU_3_SOURCE to
 * ETS_IPC_ISR_INUM (see cpu_multicore.c). The static Class-B walk remaps that
 * source to CPU line 9 and later unmaps it to ETS_INVALID_INUM. Doing the same
 * on Core 1 destroys the IPC matrix entry, so Core 0's next bist_multicore_call
 * (for example the post-walk verify job) asserts FROM_CPU_3 while Core 1 sits
 * in wait_for_intr() forever. Keep FROM_CPU_0..2 for reciprocal coverage; leave
 * FROM_CPU_3 exclusively for the multicore dispatcher. Static single-core still
 * exercises all four sources.
 */

typedef struct {
    uint32_t reg;
    uint32_t source;
} bist_interrupt_sw_source_t;

static const char *TAG = "bist_irq_sw";

static const bist_interrupt_sw_source_t s_test_interrupt_sources[BIST_INTERRUPT_SW_SOURCE_COUNT] = {
    {HP_SYSTEM_CPU_INT_FROM_CPU_0_REG, SYS_CPU_INTR_FROM_CPU_0_SOURCE},
    {HP_SYSTEM_CPU_INT_FROM_CPU_1_REG, SYS_CPU_INTR_FROM_CPU_1_SOURCE},
    {HP_SYSTEM_CPU_INT_FROM_CPU_2_REG, SYS_CPU_INTR_FROM_CPU_2_SOURCE},
};

volatile bist_interrupt_sw_result_t bist_interrupt_sw_shared_results[BIST_INTERRUPT_RESULT_SLOT_COUNT]
__attribute__((aligned(8)));

/* Walk-local ISR counter (walks are sequential; not part of exchanged evidence). */
static volatile uint32_t s_interrupt_count;

static void bist_interrupt_sw_callback(void *arg)
{
    REG_WRITE((uint32_t)arg, 0U);
    s_interrupt_count++;
}

static void clear_interrupt_sw_result(volatile bist_interrupt_sw_result_t *out)
{
    volatile uint32_t *words = (volatile uint32_t *)out;
    for (uint32_t i = 0; i < (sizeof(*out) / sizeof(*words)); ++i) {
        words[i] = 0;
    }
}

static void __attribute__((noinline, noclone)) bist_interrupt_sw_walk_job(void *arg)
{
    volatile bist_interrupt_sw_result_t *out = (volatile bist_interrupt_sw_result_t *)arg;
    const uint32_t core_id = esp_cpu_get_core_id();
    const uint32_t cpu_intr_mask = 1U << BIST_INTERRUPT_SW_CPU_NUMBER;

    if (out == NULL) {
        return;
    }

    clear_interrupt_sw_result(out);
    out->status = (uint32_t)BIST_ESP_INTERRUPT_TEST_ERR;

    BIST_ADD_LABEL("bist_interrupt_sw_walk");

    esp_cpu_intr_disable(cpu_intr_mask);
    esp_cpu_intr_set_type(BIST_INTERRUPT_SW_CPU_NUMBER, ESP_CPU_INTR_TYPE_LEVEL);
    esp_cpu_intr_set_priority(BIST_INTERRUPT_SW_CPU_NUMBER, SOC_INTERRUPT_LEVEL_MEDIUM);

    for (uint32_t src = 0; src < BIST_INTERRUPT_SW_SOURCE_COUNT; src++) {
        const uint32_t source = s_test_interrupt_sources[src].source;
        const uint32_t reg = s_test_interrupt_sources[src].reg;

        s_interrupt_count = 0;
        esp_rom_route_intr_matrix(core_id, source, BIST_INTERRUPT_SW_CPU_NUMBER);
        esp_cpu_intr_set_handler(BIST_INTERRUPT_SW_CPU_NUMBER, bist_interrupt_sw_callback, (void *)reg);
        esp_cpu_intr_enable(cpu_intr_mask);

        for (uint32_t i = 0; i < BIST_INTERRUPT_SW_LOOP_COUNT; ++i) {
            REG_WRITE(reg, BIT(0));
            esp_rom_delay_us(100);
        }
        BIST_ADD_LABEL("bist_interrupt_source_map_count");

        out->enabled_mask_active[src] = esp_cpu_intr_get_enabled_mask();
        out->interrupt_count[src] = s_interrupt_count;

        esp_cpu_intr_disable(cpu_intr_mask);
        esp_rom_route_intr_matrix(core_id, source, ETS_INVALID_INUM);

        out->enabled_mask_cleared[src] = esp_cpu_intr_get_enabled_mask();
        out->completion_mask |= UINT32_C(1) << src;
    }

    out->status = (uint32_t)BIST_ESP_OK;
}

static bist_esp_err_t verify_bist_interrupt_sw_result(volatile bist_interrupt_sw_result_t *result)
{
    const uint32_t cpu_intr_mask = 1U << BIST_INTERRUPT_SW_CPU_NUMBER;

    if (result == NULL || result->status != (uint32_t)BIST_ESP_OK ||
            result->completion_mask != BIST_INTERRUPT_SW_RESULT_COMPLETE_MASK) {
        return BIST_ESP_INTERRUPT_TEST_ERR;
    }

    for (uint32_t src = 0; src < BIST_INTERRUPT_SW_SOURCE_COUNT; src++) {
        if (result->interrupt_count[src] != BIST_INTERRUPT_SW_LOOP_COUNT) {
            return BIST_ESP_INTERRUPT_TEST_ERR;
        }
        if ((result->enabled_mask_active[src] & cpu_intr_mask) == 0U) {
            return BIST_ESP_INTERRUPT_TEST_ERR;
        }
        if ((result->enabled_mask_cleared[src] & cpu_intr_mask) != 0U) {
            return BIST_ESP_INTERRUPT_TEST_ERR;
        }
    }

    return BIST_ESP_OK;
}

static void verify_bist_interrupt_sw_result_job(void *arg)
{
    volatile bist_interrupt_sw_result_t *result = (volatile bist_interrupt_sw_result_t *)arg;

    if (result == NULL) {
        return;
    }

    result->status = (uint32_t)verify_bist_interrupt_sw_result(result);
}

bist_esp_err_t bist_interrupt_source_map_test(void)
{
    volatile bist_interrupt_sw_result_t *test_result_core_0 = &bist_interrupt_sw_shared_results[0];
    volatile bist_interrupt_sw_result_t *test_result_core_1 = &bist_interrupt_sw_shared_results[1];
    bist_esp_err_t status;
    bist_esp_err_t ret;

    bist_interrupt_sw_walk_job((void *)test_result_core_0);
    if (test_result_core_0->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to execute SW interrupt walk on core 0");
        return BIST_ESP_INTERRUPT_TEST_ERR;
    }

    status = verify_bist_interrupt_sw_result(test_result_core_0);
    if (status != BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to validate core 0 SW interrupt results on core 0");
        return BIST_ESP_INTERRUPT_TEST_ERR;
    }

    ret = bist_multicore_call(1, bist_interrupt_sw_walk_job, (void *)test_result_core_1);
    if (ret != BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to dispatch SW interrupt walk to core 1");
        return BIST_ESP_INTERRUPT_TEST_ERR;
    }
    if (test_result_core_1->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to execute SW interrupt walk on core 1");
        return BIST_ESP_INTERRUPT_TEST_ERR;
    }

    ret = bist_multicore_call(1, verify_bist_interrupt_sw_result_job, (void *)test_result_core_1);
    if (ret != BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to dispatch validation of core 1 SW interrupt results to core 1");
        return BIST_ESP_INTERRUPT_TEST_ERR;
    }
    if (test_result_core_1->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to validate core 1 SW interrupt results on core 1");
        return BIST_ESP_INTERRUPT_TEST_ERR;
    }

    BIST_ADD_LABEL("bist_interrupt_sw_verify_core0_on_core1");
    ret = bist_multicore_call(1, verify_bist_interrupt_sw_result_job, (void *)test_result_core_0);
    if (ret != BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to dispatch validation of core 0 SW interrupt results to core 1");
        return BIST_ESP_INTERRUPT_TEST_ERR;
    }
    if (test_result_core_0->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to validate core 0 SW interrupt results on core 1");
        return BIST_ESP_INTERRUPT_TEST_ERR;
    }

    BIST_ADD_LABEL("bist_interrupt_sw_verify_core1_on_core0");
    status = verify_bist_interrupt_sw_result(test_result_core_1);
    if (status != BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to validate core 1 SW interrupt results on core 0");
        return BIST_ESP_INTERRUPT_TEST_ERR;
    }

    return BIST_ESP_OK;
}

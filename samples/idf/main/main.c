/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * HP-core side of the ESP-BIST IDF sample.
 *
 * Loads the LP core firmware and signals it to start BIST tests.
 * After the LP core completes each test phase, results are read from
 * ULP shared variables and printed on the main UART so that
 * pytest-embedded can verify them.
 *
 * Two rounds are collected:
 *   1. Post-boot  - runs once right after LP core boots.
 *   2. Runtime    - first periodic runtime round.
 */

#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include "esp_sleep.h"
#include "ulp_lp_core.h"
#include "lp_core_uart.h"
#include "ulp_idf_bist_sample.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bist_protocol.h"

#define POLL_INTERVAL_MS  100
#define POLL_TIMEOUT_MS   10000
#define RUNTIME_LOOPS     10

extern const uint8_t ulp_idf_bist_sample_bin_start[] asm("_binary_ulp_idf_bist_sample_bin_start");
extern const uint8_t ulp_idf_bist_sample_bin_end[]   asm("_binary_ulp_idf_bist_sample_bin_end");

static void lp_uart_init(void)
{
    lp_core_uart_cfg_t cfg = LP_CORE_UART_DEFAULT_CONFIG();

    ESP_ERROR_CHECK(lp_core_uart_init(&cfg));

    printf("LP UART initialized successfully\n");
}

static void lp_core_init(void)
{
    ulp_lp_core_cfg_t cfg = {
        .wakeup_source = ULP_LP_CORE_WAKEUP_SOURCE_HP_CPU,
    };

    ESP_ERROR_CHECK(ulp_lp_core_load_binary(ulp_idf_bist_sample_bin_start,
                    (ulp_idf_bist_sample_bin_end - ulp_idf_bist_sample_bin_start)));

    ESP_ERROR_CHECK(ulp_lp_core_run(&cfg));

    printf("LP core loaded with firmware and running successfully\n");
}

static int wait_for_result(volatile uint32_t *result_var)
{
    int remaining_ms = POLL_TIMEOUT_MS;

    while (!(*result_var & BIST_BIT_POSTBOOT) && remaining_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
        remaining_ms -= POLL_INTERVAL_MS;
    }

    return (*result_var & BIST_BIT_POSTBOOT) ? 0 : -1;
}

static int wait_for_runtime_count(uint32_t target)
{
    int remaining_ms = POLL_TIMEOUT_MS;

    while (ulp_runtime_count < target && remaining_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
        remaining_ms -= POLL_INTERVAL_MS;
    }

    return (ulp_runtime_count >= target) ? 0 : -1;
}

static void print_postboot_results(uint32_t result)
{
    printf("=== Post-boot BIST results ===\n");
    printf("test_BIST_cpu_reg:%s\n",     (result & BIST_BIT_CPU_REG) ? "PASS" : "FAIL");
    printf("test_BIST_cpu_csr:%s\n",     (result & BIST_BIT_CPU_CSR) ? "PASS" : "FAIL");
    printf("test_BIST_ram_march_x:%s\n", (result & BIST_BIT_RAM_X)  ? "PASS" : "FAIL");
    printf("test_BIST_flash_crc:%s\n",   (result & BIST_BIT_FLASH)  ? "PASS" : "FAIL");
}

static void print_runtime_results(uint32_t result)
{
    printf("=== Runtime BIST results ===\n");
    printf("test_BIST_runtime_cpu_reg:%s\n",      (result & BIST_BIT_CPU_REG) ? "PASS" : "FAIL");
    printf("test_BIST_runtime_cpu_csr:%s\n",      (result & BIST_BIT_CPU_CSR) ? "PASS" : "FAIL");
    printf("test_BIST_runtime_ram_march_a:%s\n",  (result & BIST_BIT_RAM_A)   ? "PASS" : "FAIL");
    printf("test_BIST_runtime_stack_check:%s\n",  (result & BIST_BIT_STACK)   ? "PASS" : "FAIL");
}

void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(1000));

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_causes();
    if (cause != ESP_SLEEP_WAKEUP_ULP) {
        printf("Not an LP core wakeup. Cause = %d\n", cause);
        printf("Initializing...\n");

        lp_uart_init();
        lp_core_init();
    }

    /* Signal LP core to start BIST tests */
    ulp_hp_ready = BIST_MSG_READY;

    /* Wait for post-boot results */
    if (wait_for_result(&ulp_postboot_result) == 0) {
        print_postboot_results(ulp_postboot_result);
    } else {
        printf("test_BIST_postboot:TIMEOUT\n");
    }

    /* Wait for runtime results over multiple LP core iterations */
    bool all_pass = true;

    for (int i = 1; i <= RUNTIME_LOOPS; i++) {
        if (wait_for_runtime_count(i) != 0) {
            printf("test_BIST_runtime:TIMEOUT (loop %d)\n", i);
            all_pass = false;
            break;
        }
        uint32_t result = ulp_runtime_result;
        printf("--- Runtime loop %d/%d ---\n", i, RUNTIME_LOOPS);
        print_runtime_results(result);
        if ((result & BIST_RUNTIME_ALL_PASS) != BIST_RUNTIME_ALL_PASS) {
            all_pass = false;
        }
    }

    printf("BIST_RESULT:%s\n", all_pass ? "PASS" : "FAIL");

    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

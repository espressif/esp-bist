/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * HP-core side of the ESP-BIST IDF sample.
 *
 * Loads the LP companion firmware and starts the Host Diagnostic Agent.
 * The companion runs LP BIST and reports status bitmasks; the agent
 * queues them for this app to print. With CONFIG_ESP_BIST_HD_AUDIT_QA,
 * the companion also issues Q&A challenges that the agent answers on
 * the worker path.
 *
 * This is the happy path only, so it stays readable as a starting point.
 * Fail-closed behaviour is validated in tests/integration/hd_idf, which makes
 * the host misbehave and observes the companion resetting it.
 */

#include <stdio.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
#include "ulp_lp_core.h"
#include "lp_core_uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bist_hd_agent.h"
#include "bist_hd_protocol.h"

static const char *TAG = "bist_sample";

#define MAILBOX_TIMEOUT_MS  10000
#define RUNTIME_LOOPS       10

extern const uint8_t ulp_idf_bist_sample_bin_start[] asm("_binary_ulp_idf_bist_sample_bin_start");
extern const uint8_t ulp_idf_bist_sample_bin_end[]   asm("_binary_ulp_idf_bist_sample_bin_end");

static void lp_uart_init(void)
{
    lp_core_uart_cfg_t cfg = LP_CORE_UART_DEFAULT_CONFIG();

    ESP_ERROR_CHECK(lp_core_uart_init(&cfg));

    ESP_LOGI(TAG, "LP UART initialized successfully");
}

static void lp_core_init(void)
{
    ulp_lp_core_cfg_t cfg = {
        .wakeup_source = ULP_LP_CORE_WAKEUP_SOURCE_HP_CPU,
    };

    ESP_ERROR_CHECK(ulp_lp_core_load_binary(ulp_idf_bist_sample_bin_start,
                                            (ulp_idf_bist_sample_bin_end - ulp_idf_bist_sample_bin_start)));

    ESP_ERROR_CHECK(ulp_lp_core_run(&cfg));

    ESP_LOGI(TAG, "LP core loaded with firmware and running successfully");
}

static void print_postboot_results(uint32_t result)
{
    ESP_LOGI(TAG, "=== Post-boot BIST results ===");
    ESP_LOGI(TAG, "test_BIST_cpu_reg:%s\n", (result & BIST_HD_BIT_CPU_REG) ? "PASS" : "FAIL");
    ESP_LOGI(TAG, "test_BIST_cpu_csr:%s", (result & BIST_HD_BIT_CPU_CSR) ? "PASS" : "FAIL");
    ESP_LOGI(TAG, "test_BIST_ram_march_x:%s", (result & BIST_HD_BIT_RAM_X)  ? "PASS" : "FAIL");
    ESP_LOGI(TAG, "test_BIST_ram_abraham:%s", (result & BIST_HD_BIT_ABRAHAM) ? "PASS" : "FAIL");
    ESP_LOGI(TAG, "test_BIST_flash_crc:%s", (result & BIST_HD_BIT_FLASH)  ? "PASS" : "FAIL");
}

static void print_runtime_results(uint32_t result)
{
    ESP_LOGI(TAG, "=== Runtime BIST results ===");
    ESP_LOGI(TAG, "test_BIST_runtime_cpu_reg:%s", (result & BIST_HD_BIT_CPU_REG) ? "PASS" : "FAIL");
    ESP_LOGI(TAG, "test_BIST_runtime_cpu_csr:%s", (result & BIST_HD_BIT_CPU_CSR) ? "PASS" : "FAIL");
    ESP_LOGI(TAG, "test_BIST_runtime_ram_march_a:%s", (result & BIST_HD_BIT_RAM_A)   ? "PASS" : "FAIL");
    ESP_LOGI(TAG, "test_BIST_runtime_ram_abraham:%s", (result & BIST_HD_BIT_ABRAHAM) ? "PASS" : "FAIL");
    ESP_LOGI(TAG, "test_BIST_runtime_stack_check:%s", (result & BIST_HD_BIT_STACK)   ? "PASS" : "FAIL");
}

void app_main(void)
{
    uint32_t status;
    bool all_pass = true;
    bool runtime_ok = true;
    int err;

    vTaskDelay(pdMS_TO_TICKS(1000));

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_causes();
    if (cause != ESP_SLEEP_WAKEUP_ULP) {
        ESP_LOGI(TAG, "Not an LP core wakeup. Cause = %d", cause);
        ESP_LOGI(TAG, "Initializing...");

        lp_uart_init();
        lp_core_init();
    }

    /* Give the LP core time to initialize the software mailbox first. */
    vTaskDelay(pdMS_TO_TICKS(100));

    err = bist_hd_agent_start();
    if (err != 0) {
        ESP_LOGE(TAG, "Failed to start Host Diagnostic Agent: %d", err);
        ESP_LOGI(TAG, "test_HD_agent_ready:FAIL\n");
        return;
    }
    ESP_LOGI(TAG, "Host Diagnostic Agent started");
    ESP_LOGI(TAG, "test_HD_agent_ready:PASS\n");

    err = bist_hd_agent_wait_lp_status(&status, BIST_HD_BIT_POSTBOOT, MAILBOX_TIMEOUT_MS);
    if (err == 0) {
        print_postboot_results(status);
    } else {
        ESP_LOGI(TAG, "test_BIST_postboot:TIMEOUT\n");
    }

    for (int i = 1; i <= RUNTIME_LOOPS; i++) {
        err = bist_hd_agent_wait_lp_status(&status, BIST_HD_BIT_RUNTIME, MAILBOX_TIMEOUT_MS);
        if (err != 0) {
            ESP_LOGI(TAG, "test_BIST_runtime:TIMEOUT (loop %d)\n", i);
            all_pass = false;
            runtime_ok = false;
            break;
        }
        ESP_LOGI(TAG, "--- Runtime loop %d/%d ---\n", i, RUNTIME_LOOPS);
        print_runtime_results(status);
        if ((status & BIST_HD_RUNTIME_ALL_PASS) != BIST_HD_RUNTIME_ALL_PASS) {
            all_pass = false;
        }
    }

#ifdef CONFIG_ESP_BIST_HD_AUDIT_QA
    /*
     * Companion issues a challenge after each LP_STATUS. Completing all
     * runtime loops proves that prior Q&A rounds passed (else safe state
     * would have stopped further status). A runtime test failure does not
     * invalidate Q&A, so only the loop completion matters here.
     */
    if (runtime_ok) {
        ESP_LOGI(TAG, "test_HD_challenge:PASS\n");
    } else {
        ESP_LOGI(TAG, "test_HD_challenge:FAIL\n");
    }
#endif

    ESP_LOGI(TAG, "BIST_RESULT:%s\n", all_pass ? "PASS" : "FAIL");

    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());

    /*
     * The host is QM and must not self-declare safe state or halt: the
     * companion owns safe-state decisions (stops feeding the LP WDT on
     * failure). Park app_main so the agent task keeps running.
     */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

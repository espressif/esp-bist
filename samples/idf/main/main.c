/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * HP-core side of the ESP-BIST IDF sample.
 *
 * Loads the LP core firmware and signals it to start BIST tests over
 * the LP mailbox. After the LP core completes each test phase, results
 * are received as bitmask messages and printed on the main UART so that
 * pytest-embedded can verify them.
 *
 * Two rounds are collected:
 *   1. Post-boot  - runs once right after LP core boots.
 *   2. Runtime    - first periodic runtime round.
 */

#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include "esp_err.h"
#include "esp_sleep.h"
#include "ulp_lp_core.h"
#include "lp_core_uart.h"
#include "lp_core_mailbox.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bist_protocol.h"

#define MAILBOX_TIMEOUT_MS  10000
#define RUNTIME_LOOPS       10

extern const uint8_t ulp_idf_bist_sample_bin_start[] asm("_binary_ulp_idf_bist_sample_bin_start");
extern const uint8_t ulp_idf_bist_sample_bin_end[]   asm("_binary_ulp_idf_bist_sample_bin_end");

static lp_mailbox_t s_mailbox;

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

static void print_postboot_results(uint32_t result)
{
    printf("=== Post-boot BIST results ===\n");
    printf("test_BIST_cpu_reg:%s\n",     (result & BIST_BIT_CPU_REG) ? "PASS" : "FAIL");
    printf("test_BIST_cpu_csr:%s\n",     (result & BIST_BIT_CPU_CSR) ? "PASS" : "FAIL");
    printf("test_BIST_ram_march_x:%s\n", (result & BIST_BIT_RAM_X)  ? "PASS" : "FAIL");
    printf("test_BIST_ram_abraham:%s\n", (result & BIST_BIT_ABRAHAM) ? "PASS" : "FAIL");
    printf("test_BIST_flash_crc:%s\n",   (result & BIST_BIT_FLASH)  ? "PASS" : "FAIL");
}

static void print_runtime_results(uint32_t result)
{
    printf("=== Runtime BIST results ===\n");
    printf("test_BIST_runtime_cpu_reg:%s\n",      (result & BIST_BIT_CPU_REG) ? "PASS" : "FAIL");
    printf("test_BIST_runtime_cpu_csr:%s\n",      (result & BIST_BIT_CPU_CSR) ? "PASS" : "FAIL");
    printf("test_BIST_runtime_ram_march_a:%s\n",  (result & BIST_BIT_RAM_A)   ? "PASS" : "FAIL");
    printf("test_BIST_runtime_ram_abraham:%s\n",  (result & BIST_BIT_ABRAHAM) ? "PASS" : "FAIL");
    printf("test_BIST_runtime_stack_check:%s\n",  (result & BIST_BIT_STACK)   ? "PASS" : "FAIL");
}

void app_main(void)
{
    lp_message_t msg;
    esp_err_t err;
    TickType_t timeout = pdMS_TO_TICKS(MAILBOX_TIMEOUT_MS);

    vTaskDelay(pdMS_TO_TICKS(1000));

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_causes();
    if (cause != ESP_SLEEP_WAKEUP_ULP) {
        printf("Not an LP core wakeup. Cause = %d\n", cause);
        printf("Initializing...\n");

        lp_uart_init();
        lp_core_init();
    }

    /* Give the LP core time to initialize the software mailbox first. */
    vTaskDelay(pdMS_TO_TICKS(100));

    err = lp_core_mailbox_init(&s_mailbox, NULL);
    if (err != ESP_OK) {
        printf("Could not initialize mailbox: %x\n", err);
        return;
    }
    printf("LP Mailbox initialized successfully\n");

    /* Signal LP core to start BIST tests */
    err = lp_core_mailbox_send(s_mailbox, (lp_message_t)BIST_MSG_READY, timeout);
    if (err != ESP_OK) {
        printf("Failed to send ready signal: %x\n", err);
        return;
    }

    /* Wait for post-boot results */
    err = lp_core_mailbox_receive(s_mailbox, &msg, timeout);
    if (err == ESP_OK && ((uint32_t)msg & BIST_BIT_POSTBOOT)) {
        print_postboot_results((uint32_t)msg);
    } else {
        printf("test_BIST_postboot:TIMEOUT\n");
    }

    /* Wait for runtime results over multiple LP core iterations */
    bool all_pass = true;

    for (int i = 1; i <= RUNTIME_LOOPS; i++) {
        err = lp_core_mailbox_receive(s_mailbox, &msg, timeout);
        if (err != ESP_OK || !((uint32_t)msg & BIST_BIT_RUNTIME)) {
            printf("test_BIST_runtime:TIMEOUT (loop %d)\n", i);
            all_pass = false;
            break;
        }
        uint32_t result = (uint32_t)msg;
        printf("--- Runtime loop %d/%d ---\n", i, RUNTIME_LOOPS);
        print_runtime_results(result);
        if ((result & BIST_RUNTIME_ALL_PASS) != BIST_RUNTIME_ALL_PASS) {
            all_pass = false;
        }
    }

    printf("BIST_RESULT:%s\n", all_pass ? "PASS" : "FAIL");

    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());

    /*
     * Keep acknowledging LP runtime messages. Synchronous mailbox send on
     * the LP core blocks until HP receives; without this drain the LP WDT
     * fires and resets the chip (LP_WDT_HPSYS).
     */
    while (1) {
        (void)lp_core_mailbox_receive(s_mailbox, &msg, portMAX_DELAY);
    }
}

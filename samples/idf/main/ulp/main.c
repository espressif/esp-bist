/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * LP-core side of the ESP-BIST IDF sample.
 *
 * Waits for a BIST_MSG_READY handshake from the HP core, then runs
 * BIST post-boot tests once, followed by periodic runtime tests.
 * Results are communicated to the HP core via shared variables.
 */

#include <stdint.h>
#include "ulp_lp_core_utils.h"
#include "ulp_lp_core_print.h"
#include "bist_esp.h"
#include "bist_log.h"
#include "lp_wdt.h"
#include "bist_protocol.h"

#define RUNTIME_INTERVAL_US 10000

static const char *TAG = "ulp_idf_bist_sample";

/* Shared variables -- accessible from HP core as ulp_<name> */
volatile uint32_t hp_ready = 0;
volatile uint32_t postboot_result = 0;
volatile uint32_t runtime_result = 0;
volatile uint32_t runtime_count = 0;

void handle_stack_overflow(void)
{
    ESP_LOGE(TAG, "Stack overflow detected\r\n");
}

static uint32_t run_postboot_tests(void)
{
    uint32_t mask = 0;
    bist_esp_err_t err;

#ifdef CONFIG_ESP_BIST_CPU_REG_TEST
    ESP_LOGI(TAG, "CPU reg test... ");
    err = bist_cpu_regs_test();
    ESP_LOGI(TAG, "%s (%d)\r\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
    if (err == BIST_ESP_OK) {
        mask |= BIST_BIT_CPU_REG;
    }
#endif

#ifdef CONFIG_ESP_BIST_CPU_CSR_REG_TEST
    ESP_LOGI(TAG, "CPU CSR test... ");
    err = bist_cpu_csr_regs_test();
    ESP_LOGI(TAG, "%s (%d)\r\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
    if (err == BIST_ESP_OK) {
        mask |= BIST_BIT_CPU_CSR;
    }
#endif

#ifdef CONFIG_ESP_BIST_MEMORY_RAM_TEST
    ESP_LOGI(TAG, "RAM March-X test... ");
    err = bist_ram_test_march_x();
    ESP_LOGI(TAG, "%s (%d)\r\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
    if (err == BIST_ESP_OK) {
        mask |= BIST_BIT_RAM_X;
    }
#endif

#ifdef CONFIG_ESP_BIST_MEMORY_FLASH_TEST
    ESP_LOGI(TAG, "Flash CRC test... ");
    err = bist_flash_test();
    ESP_LOGI(TAG, "%s (%d)\r\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
    if (err == BIST_ESP_OK) {
        mask |= BIST_BIT_FLASH;
    }
#endif

    return mask;
}

static uint32_t run_runtime_tests(void)
{
    uint32_t mask = 0;
    bist_esp_err_t err;

#ifdef CONFIG_ESP_BIST_CPU_REG_TEST
    ESP_LOGI(TAG, "CPU reg test... ");
    err = bist_cpu_regs_test();
    ESP_LOGI(TAG, "%s (%d)\r\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
    if (err == BIST_ESP_OK) {
        mask |= BIST_BIT_CPU_REG;
    }
#endif

#ifdef CONFIG_ESP_BIST_CPU_CSR_REG_TEST
    ESP_LOGI(TAG, "CPU CSR test... ");
    err = bist_cpu_csr_regs_test();
    ESP_LOGI(TAG, "%s (%d)\r\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
    if (err == BIST_ESP_OK) {
        mask |= BIST_BIT_CPU_CSR;
    }
#endif

#ifdef CONFIG_ESP_BIST_MEMORY_RAM_TEST
    ESP_LOGI(TAG, "RAM March-A test... ");
    err = bist_ram_test_march_a();
    ESP_LOGI(TAG, "%s (%d)\r\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
    if (err == BIST_ESP_OK) {
        mask |= BIST_BIT_RAM_A;
    }
#endif

#ifdef CONFIG_ESP_BIST_STACK_TEST
    ESP_LOGI(TAG, "Stack overflow check... ");
    err = bist_cpu_stack_overflow_check();
    ESP_LOGI(TAG, "%s (%d)\r\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
    if (err == BIST_ESP_OK) {
        mask |= BIST_BIT_STACK;
    }
#endif

    return mask;
}

int main(void)
{
    uint32_t result;

    ESP_LOGI(TAG, "Starting ULP BIST sample\r\n");

    bist_cpu_stack_overflow_init();

    ESP_LOGI(TAG, "Waiting for HP core ready signal...\r\n");
    while (hp_ready != BIST_MSG_READY) {
        ulp_lp_core_delay_us(1000);
    }
    ESP_LOGI(TAG, "HP core ready, starting tests\r\n");

    ESP_LOGI(TAG, "=== Post-boot tests ===\r\n");
    result = run_postboot_tests() | BIST_BIT_POSTBOOT;
    ESP_LOGI(TAG, "Post-boot result: 0x%08x\r\n", result);
    postboot_result = result;

    lp_wdt_init(CONFIG_ESP_BIST_WDT_TIMEOUT_US);

    ESP_LOGI(TAG, "=== Runtime tests (periodic) ===\r\n");
    while (1) {
        lp_wdt_feed();
        ulp_lp_core_delay_us(RUNTIME_INTERVAL_US);
        result = run_runtime_tests() | BIST_BIT_RUNTIME;
        runtime_result = result;
        runtime_count++;
    }

    return 0;
}

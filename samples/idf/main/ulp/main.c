/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * LP-core side of the ESP-BIST IDF sample.
 *
 * Waits for a BIST_MSG_READY handshake from the HP core over the LP
 * mailbox, then runs BIST post-boot tests once, followed by periodic
 * runtime tests. Results are sent to the HP core as bitmask messages.
 */

#include <stdint.h>
#include "ulp_lp_core_utils.h"
#include "ulp_lp_core_print.h"
#include "ulp_lp_core_mailbox.h"
#include "bist_esp.h"
#include "bist_log.h"
#include "lp_wdt.h"
#include "bist_protocol.h"

#define RUNTIME_INTERVAL_US 10000
/* Mailbox timeout is in CPU cycles. (Assuming 40MHz CPU frequency for ~1s)*/
#define READY_TIMEOUT_CYCLES  (10 * 40000000)

static const char *TAG = "ulp_idf_bist_sample";

static lp_mailbox_t mailbox;

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

    ESP_LOGI(TAG, "RAM Abraham full test... ");
    err = bist_ram_test_abraham_full();
    ESP_LOGI(TAG, "%s (%d)\r\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
    if (err == BIST_ESP_OK) {
        mask |= BIST_BIT_ABRAHAM;
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
    lp_wdt_feed();
#endif

#ifdef CONFIG_ESP_BIST_CPU_CSR_REG_TEST
    ESP_LOGI(TAG, "CPU CSR test... ");
    err = bist_cpu_csr_regs_test();
    ESP_LOGI(TAG, "%s (%d)\r\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
    if (err == BIST_ESP_OK) {
        mask |= BIST_BIT_CPU_CSR;
    }
    lp_wdt_feed();
#endif

#ifdef CONFIG_ESP_BIST_MEMORY_RAM_TEST
    ESP_LOGI(TAG, "RAM March-A test... ");
    err = bist_ram_test_march_a();
    ESP_LOGI(TAG, "%s (%d)\r\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
    if (err == BIST_ESP_OK) {
        mask |= BIST_BIT_RAM_A;
    }
    lp_wdt_feed();

    ESP_LOGI(TAG, "RAM Abraham test... ");
    err = bist_ram_test_abraham();
    ESP_LOGI(TAG, "%s (%d)\r\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
    if (err == BIST_ESP_OK) {
        mask |= BIST_BIT_ABRAHAM;
    }
    lp_wdt_feed();
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
    lp_message_t msg;
    uint32_t result;
    esp_err_t err;

    ESP_LOGI(TAG, "Starting ULP BIST sample\r\n");

    /* Software mailbox requires LP init before HP; do this first. */
    err = lp_core_mailbox_init(&mailbox, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Mailbox init failed: %d\r\n", err);
        return 0;
    }

    bist_cpu_stack_overflow_init();

    ESP_LOGI(TAG, "Waiting for HP core ready signal...\r\n");
    err = lp_core_mailbox_receive(mailbox, &msg, READY_TIMEOUT_CYCLES);
    if (err != ESP_OK || (uint32_t)msg != BIST_MSG_READY) {
        ESP_LOGE(TAG, "Ready handshake failed: err=%d msg=0x%08x\r\n",
                 err, (unsigned)msg);
        return 0;
    }
    ESP_LOGI(TAG, "HP core ready, starting tests\r\n");

    ESP_LOGI(TAG, "=== Post-boot tests ===\r\n");
    result = run_postboot_tests() | BIST_BIT_POSTBOOT;
    ESP_LOGI(TAG, "Post-boot result: 0x%08x\r\n", result);
    err = lp_core_mailbox_send(mailbox, (lp_message_t)result, -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send post-boot result: %d\r\n", err);
        return 0;
    }

    lp_wdt_init(CONFIG_ESP_BIST_WDT_TIMEOUT_US);

    ESP_LOGI(TAG, "=== Runtime tests (periodic) ===\r\n");
    while (1) {
        ulp_lp_core_delay_us(RUNTIME_INTERVAL_US);
        lp_wdt_feed();
        result = run_runtime_tests() | BIST_BIT_RUNTIME;
        err = lp_core_mailbox_send(mailbox, (lp_message_t)result, -1);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send runtime result: %d\r\n", err);
        }
    }

    return 0;
}

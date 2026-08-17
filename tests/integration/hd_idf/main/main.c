/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * HP-core side of the Host Diagnostics fail-closed validation app.
 *
 * This is not an integration example; samples/idf is. Here the host
 * deliberately misbehaves so the LP companion must enter safe state, stop
 * feeding the LP watchdog and reset the chip.
 *
 * The ESP-BIST library is built exactly as a product would build it. The fault
 * comes from this app: either the image carries a challenge key the companion
 * does not share (see main/CMakeLists.txt) or app_main withholds the agent past
 * the challenge window (starve_agent below).
 */

#include <stdio.h>
#include "esp_err.h"
#include "esp_rom_sys.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
#include "ulp_lp_core.h"
#include "lp_core_uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#include "bist_hd_agent.h"
#include "bist_hd_protocol.h"

#if !defined(CONFIG_BIST_HD_TEST_KEY_MISMATCH) && !defined(CONFIG_BIST_HD_TEST_STARVE_AGENT)
#error "Select a fault: CONFIG_BIST_HD_TEST_KEY_MISMATCH or CONFIG_BIST_HD_TEST_STARVE_AGENT"
#endif

#define MAILBOX_TIMEOUT_MS 10000

extern const uint8_t ulp_hd_idf_test_bin_start[] asm("_binary_ulp_hd_idf_test_bin_start");
extern const uint8_t ulp_hd_idf_test_bin_end[]   asm("_binary_ulp_hd_idf_test_bin_end");

#ifdef CONFIG_BIST_HD_TEST_STARVE_AGENT
/* Outlasts the challenge window plus the companion LP watchdog timeout. */
#define STARVE_US         500000
#define STARVE_TASK_STACK 2048

static void starve_core(void *arg)
{
    esp_rom_delay_us(STARVE_US);
    vTaskDelete(NULL);
}

/*
 * Model a host too busy to answer: run above the agent task priority and never
 * yield, so the agent is not scheduled and no ANSWER reaches the companion
 * before the window closes. Nothing in the library participates.
 */
static void starve_agent(void)
{
    const BaseType_t self = xPortGetCoreID();

    fflush(stdout);
    vTaskPrioritySet(NULL, configMAX_PRIORITIES - 1);

    for (BaseType_t core = 0; core < configNUMBER_OF_CORES; core++) {
        if (core != self) {
            xTaskCreatePinnedToCore(starve_core, "hd_starve", STARVE_TASK_STACK, NULL,
                                    configMAX_PRIORITIES - 1, NULL, core);
        }
    }

    esp_rom_delay_us(STARVE_US);
}
#endif

static void lp_uart_init(void)
{
    lp_core_uart_cfg_t cfg = LP_CORE_UART_DEFAULT_CONFIG();

    ESP_ERROR_CHECK(lp_core_uart_init(&cfg));
}

static void lp_core_init(void)
{
    ulp_lp_core_cfg_t cfg = {
        .wakeup_source = ULP_LP_CORE_WAKEUP_SOURCE_HP_CPU,
    };

    ESP_ERROR_CHECK(ulp_lp_core_load_binary(ulp_hd_idf_test_bin_start,
                    (ulp_hd_idf_test_bin_end - ulp_hd_idf_test_bin_start)));

    ESP_ERROR_CHECK(ulp_lp_core_run(&cfg));
}

void app_main(void)
{
    uint32_t status;
    int err;

    vTaskDelay(pdMS_TO_TICKS(1000));

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_causes();
    if (cause != ESP_SLEEP_WAKEUP_ULP) {
        lp_uart_init();
        lp_core_init();
    }

    /* Give the LP core time to initialize the software mailbox first. */
    vTaskDelay(pdMS_TO_TICKS(100));

    err = bist_hd_agent_start();
    if (err != 0) {
        printf("test_HD_agent_ready:FAIL\n");
        return;
    }
    printf("test_HD_agent_ready:PASS\n");

    err = bist_hd_agent_wait_lp_status(&status, BIST_HD_BIT_POSTBOOT, MAILBOX_TIMEOUT_MS);
    printf("test_BIST_postboot:%s\n", (err == 0) ? "PASS" : "TIMEOUT");

    /*
     * One runtime LP_STATUS is reported before the first challenge, so this
     * marker confirms the supervision loop is live before the fault lands.
     */
    err = bist_hd_agent_wait_lp_status(&status, BIST_HD_BIT_RUNTIME, MAILBOX_TIMEOUT_MS);
    printf("test_HD_fail_closed_armed:%s\n", (err == 0) ? "PASS" : "FAIL");

#ifdef CONFIG_BIST_HD_TEST_STARVE_AGENT
    starve_agent();
#endif

    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());

    /*
     * The key-mismatch build has no busy-wait: every answer is wrong, so the
     * companion reaches its safe-state threshold while this task idles.
     */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

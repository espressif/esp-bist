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
#include <stdbool.h>
#include <stdint.h>
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_rom_sys.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "sdkconfig.h"
#include "ulp_lp_core.h"
#include "lp_core_uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#include "bist_hd_agent.h"
#include "bist_hd_protocol.h"

#if !defined(CONFIG_BIST_HD_TEST_KEY_MISMATCH) && \
    !defined(CONFIG_BIST_HD_TEST_STARVE_AGENT) && \
    !defined(CONFIG_BIST_HD_TEST_SKIP_CHECKPOINT) && \
    !defined(CONFIG_BIST_HD_TEST_HOST_SW_RESET)
#error "Select a fault: CONFIG_BIST_HD_TEST_KEY_MISMATCH, CONFIG_BIST_HD_TEST_STARVE_AGENT, CONFIG_BIST_HD_TEST_SKIP_CHECKPOINT or CONFIG_BIST_HD_TEST_HOST_SW_RESET"
#endif

#define MAILBOX_TIMEOUT_MS   10000
#define RAM_CANARY_WORDS     16U
#define RAM_DATA_CANARY_SEED 0xA5C35A3CU
#define RAM_BSS_CANARY_SEED  0x5A3CA5C3U
#define RAM_DMA_CANARY_SEED  0xC3A53C5AU

extern uint32_t _safe_ram_start;
extern uint32_t _safe_ram_end;
extern uint32_t _bist_hp_ram_start;
extern uint32_t _bist_hp_ram_end;

/*
 * Static .dram0.data and .dram0.bss canaries verify that BIST_HD_AUDIT_RAM
 * marches and restores static DRAM inside [_bist_hp_ram_start, _bist_hp_ram_end),
 * while MALLOC_CAP_DMA allocations come from the dedicated .dram0.safe_ram pool
 * below _bist_hp_ram_start.
 */
static DRAM_ATTR volatile uint32_t s_data_canary[RAM_CANARY_WORDS] = {
    RAM_DATA_CANARY_SEED ^ 0U,  RAM_DATA_CANARY_SEED ^ 1U,
    RAM_DATA_CANARY_SEED ^ 2U,  RAM_DATA_CANARY_SEED ^ 3U,
    RAM_DATA_CANARY_SEED ^ 4U,  RAM_DATA_CANARY_SEED ^ 5U,
    RAM_DATA_CANARY_SEED ^ 6U,  RAM_DATA_CANARY_SEED ^ 7U,
    RAM_DATA_CANARY_SEED ^ 8U,  RAM_DATA_CANARY_SEED ^ 9U,
    RAM_DATA_CANARY_SEED ^ 10U, RAM_DATA_CANARY_SEED ^ 11U,
    RAM_DATA_CANARY_SEED ^ 12U, RAM_DATA_CANARY_SEED ^ 13U,
    RAM_DATA_CANARY_SEED ^ 14U, RAM_DATA_CANARY_SEED ^ 15U,
};
static volatile uint32_t s_bss_canary[RAM_CANARY_WORDS];

static bool verify_ram_canaries(const volatile uint32_t *dma_canary)
{
    uintptr_t safe_lo = (uintptr_t)&_safe_ram_start;
    uintptr_t safe_hi = (uintptr_t)&_safe_ram_end;
    uintptr_t span_lo = (uintptr_t)&_bist_hp_ram_start;
    uintptr_t span_hi = (uintptr_t)&_bist_hp_ram_end;
    uintptr_t dma_addr = (uintptr_t)dma_canary;
    uintptr_t data_addr = (uintptr_t)s_data_canary;
    uintptr_t bss_addr = (uintptr_t)s_bss_canary;

    if (dma_canary == NULL) {
        return false;
    }
    if (dma_addr < safe_lo || dma_addr >= safe_hi || dma_addr >= span_lo) {
        return false;
    }
    if (data_addr < span_lo || data_addr >= span_hi ||
            bss_addr < span_lo || bss_addr >= span_hi) {
        return false;
    }
    for (uint32_t i = 0; i < RAM_CANARY_WORDS; i++) {
        if (s_data_canary[i] != (RAM_DATA_CANARY_SEED ^ i)) {
            return false;
        }
        if (s_bss_canary[i] != (RAM_BSS_CANARY_SEED ^ i)) {
            return false;
        }
        if (dma_canary[i] != (RAM_DMA_CANARY_SEED ^ i)) {
            return false;
        }
    }
    return true;
}

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
    uint32_t *dma_canary = NULL;
    int err;

    for (uint32_t i = 0; i < RAM_CANARY_WORDS; i++) {
        s_bss_canary[i] = RAM_BSS_CANARY_SEED ^ i;
    }
    dma_canary = (uint32_t *)heap_caps_malloc(RAM_CANARY_WORDS * sizeof(uint32_t),
                                              MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (dma_canary != NULL) {
        for (uint32_t i = 0; i < RAM_CANARY_WORDS; i++) {
            dma_canary[i] = RAM_DMA_CANARY_SEED ^ i;
        }
    }

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
    printf("test_HD_safe_dma_pool:%s\n", verify_ram_canaries(dma_canary) ? "PASS" : "FAIL");
    printf("test_HD_fail_closed_armed:%s\n", (err == 0) ? "PASS" : "FAIL");
    heap_caps_free(dma_canary);
#ifndef CONFIG_BIST_HD_TEST_SKIP_CHECKPOINT
    bist_hd_checkpoint_reached();
#endif

#ifdef CONFIG_BIST_HD_TEST_STARVE_AGENT
    starve_agent();
#endif

#ifdef CONFIG_BIST_HD_TEST_HOST_SW_RESET
    fflush(stdout);
    esp_rom_delay_us(2000);
    esp_restart();
#endif

    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());

    /*
     * The key-mismatch build has no busy-wait: every answer is wrong, so the
     * companion reaches its safe-state threshold while this task idles.
     */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
#ifndef CONFIG_BIST_HD_TEST_SKIP_CHECKPOINT
        bist_hd_checkpoint_reached();
#endif
    }
}

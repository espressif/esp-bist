/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * ESP-IDF / FreeRTOS platform adapter for the Host Diagnostic Agent.
 */

#include "bist_hd_platform.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "esp_log.h"

#if CONFIG_ESP_SYSTEM_HW_STACK_GUARD
#include "esp_private/hw_stack_guard.h"
#endif

#if !CONFIG_FREERTOS_UNICORE
#include "esp_cpu.h"
#endif

#if defined(CONFIG_ESP_BIST_HD_AUDIT_RAM) && \
    defined(CONFIG_ESP_BIST_HD_SAFE_DMA_POOL_SIZE) && \
    (CONFIG_ESP_BIST_HD_SAFE_DMA_POOL_SIZE > 0)
#ifndef MULTI_HEAP_FREERTOS
#define MULTI_HEAP_FREERTOS
#endif
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_heap_caps_init.h"
#include "esp_private/esp_sys_event_app_init.h"
#include "heap_private.h"
#endif

#ifndef CONFIG_ESP_BIST_HD_AGENT_TASK_STACK
#define CONFIG_ESP_BIST_HD_AGENT_TASK_STACK 3072
#endif

#ifndef CONFIG_ESP_BIST_HD_AGENT_TASK_PRIO
#define CONFIG_ESP_BIST_HD_AGENT_TASK_PRIO (configMAX_PRIORITIES - 1)
#endif

#define LP_STATUS_QUEUE_LEN 4

static const char *TAG = "bist_hd_platform_idf";
static QueueHandle_t s_lp_status_q;

#if defined(CONFIG_ESP_BIST_HD_AUDIT_RAM) && \
    defined(CONFIG_ESP_BIST_HD_SAFE_DMA_POOL_SIZE) && \
    (CONFIG_ESP_BIST_HD_SAFE_DMA_POOL_SIZE > 0)
#define SAFE_DMA_POOL_WORDS ((CONFIG_ESP_BIST_HD_SAFE_DMA_POOL_SIZE + 3) / 4)

static uint32_t __attribute__((section(".dram0.safe_ram"), aligned(16)))
s_safe_dma_pool[SAFE_DMA_POOL_WORDS];

static bool safe_dma_pool_postboot_test(void)
{
    volatile uint32_t *pool = (volatile uint32_t *)s_safe_dma_pool;

    for (size_t i = 0; i < SAFE_DMA_POOL_WORDS; i++) {
        pool[i] = 0U;
    }
    for (size_t i = 0; i < SAFE_DMA_POOL_WORDS; i++) {
        if (pool[i] != 0U) {
            return false;
        }
        pool[i] = 0xFFFFFFFFU;
    }
    for (size_t i = SAFE_DMA_POOL_WORDS; i != 0U; i--) {
        if (pool[i - 1U] != 0xFFFFFFFFU) {
            return false;
        }
        pool[i - 1U] = 0U;
    }
    for (size_t i = 0; i < SAFE_DMA_POOL_WORDS; i++) {
        if (pool[i] != 0U) {
            return false;
        }
    }
    return true;
}

ESP_PRE_APP_MAIN_HANDLER_REGISTER(bist_hd_init_safe_dma_heap, 115)
{
    (void)user_arg;
    (void)ctx;
    heap_t *heap;
    /*
     * Priority 0 advertises only MALLOC_CAP_DMA while get_all_caps()
     * (prio 0 | prio 1) includes MALLOC_CAP_INTERNAL | 8BIT | 32BIT.
     * Consequently, MALLOC_CAP_DMA allocations match this pool at prio 0,
     * whereas non-DMA allocations (xTaskCreate, malloc) skip this pool at
     * prio 0 and allocate from the marched main heap first.
     */
    const uint32_t dma_caps[SOC_MEMORY_TYPE_NO_PRIOS] = {
        MALLOC_CAP_DMA,
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT | MALLOC_CAP_32BIT,
        0U,
    };

    if (!safe_dma_pool_postboot_test()) {
        ESP_LOGE(TAG, "Safe DMA pool postboot test failed");
        return ESP_FAIL;
    }

    SLIST_FOREACH(heap, &registered_heaps, next) {
        if ((get_ored_caps(heap->caps) & MALLOC_CAP_INTERNAL) != 0U) {
            for (int prio = 0; prio < SOC_MEMORY_TYPE_NO_PRIOS; prio++) {
                heap->caps[prio] &= ~MALLOC_CAP_DMA;
            }
        }
    }

    return heap_caps_add_region_with_caps(dma_caps,
                                          (intptr_t)s_safe_dma_pool,
                                          (intptr_t)(s_safe_dma_pool + SAFE_DMA_POOL_WORDS));
}
#endif

int bist_hd_platform_init(void)
{
    if (s_lp_status_q != NULL) {
        return 0;
    }

    s_lp_status_q = xQueueCreate(LP_STATUS_QUEUE_LEN, sizeof(uint32_t));
    if (s_lp_status_q == NULL) {
        return -1;
    }
    return 0;
}

int bist_hd_platform_start_worker(bist_hd_worker_fn_t fn, void *arg)
{
    BaseType_t ok;

    if (fn == NULL) {
        return -1;
    }

    ok = xTaskCreate(fn, "bist_hd_agent", CONFIG_ESP_BIST_HD_AGENT_TASK_STACK, arg,
                     CONFIG_ESP_BIST_HD_AGENT_TASK_PRIO, NULL);
    return (ok == pdPASS) ? 0 : -1;
}

int bist_hd_platform_lp_status_push(uint32_t status)
{
    if (s_lp_status_q == NULL) {
        return -1;
    }
    return (xQueueSend(s_lp_status_q, &status, 0) == pdTRUE) ? 0 : -1;
}

int bist_hd_platform_lp_status_pop(uint32_t *status_out, int32_t timeout_ms)
{
    TickType_t ticks;

    if (status_out == NULL || s_lp_status_q == NULL) {
        return -1;
    }

    if (timeout_ms < 0) {
        ticks = portMAX_DELAY;
    } else {
        ticks = pdMS_TO_TICKS((uint32_t)timeout_ms);
    }

    if (xQueueReceive(s_lp_status_q, status_out, ticks) != pdTRUE) {
        return -1;
    }
    return 0;
}

uint32_t bist_hd_platform_time_ms(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

void bist_hd_platform_sleep_ms(uint32_t ms)
{
    TickType_t ticks = pdMS_TO_TICKS(ms);

    /* pdMS_TO_TICKS truncates to 0 below one tick period; always yield. */
    vTaskDelay((ticks == 0) ? 1 : ticks);
}

uint32_t bist_hd_platform_irq_lock(void)
{
    uint32_t key = (uint32_t)portSET_INTERRUPT_MASK_FROM_ISR();

#if CONFIG_ESP_SYSTEM_HW_STACK_GUARD
    esp_hw_stack_guard_monitor_stop();
#endif
#if !CONFIG_FREERTOS_UNICORE
    esp_cpu_stall((esp_cpu_get_core_id() == 0) ? 1 : 0);
#endif
    return key;
}

void bist_hd_platform_irq_unlock(uint32_t key)
{
#if !CONFIG_FREERTOS_UNICORE
    esp_cpu_unstall((esp_cpu_get_core_id() == 0) ? 1 : 0);
#endif
#if CONFIG_ESP_SYSTEM_HW_STACK_GUARD
    esp_hw_stack_guard_monitor_start();
#endif
    portCLEAR_INTERRUPT_MASK_FROM_ISR(key);
}

void bist_hd_platform_preempt_lock(void)
{
    vTaskSuspendAll();
}

void bist_hd_platform_preempt_unlock(void)
{
    (void)xTaskResumeAll();
}

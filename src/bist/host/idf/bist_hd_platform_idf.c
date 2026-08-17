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

#ifndef CONFIG_ESP_BIST_HD_AGENT_TASK_STACK
#define CONFIG_ESP_BIST_HD_AGENT_TASK_STACK 3072
#endif

#ifndef CONFIG_ESP_BIST_HD_AGENT_TASK_PRIO
#define CONFIG_ESP_BIST_HD_AGENT_TASK_PRIO (configMAX_PRIORITIES - 1)
#endif

#define LP_STATUS_QUEUE_LEN 4

static QueueHandle_t s_lp_status_q;

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

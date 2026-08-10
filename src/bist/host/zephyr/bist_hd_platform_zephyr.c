/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Zephyr platform adapter for the Host Diagnostic Agent.
 */

#include "bist_hd_platform.h"

#include <zephyr/kernel.h>

#ifndef CONFIG_ESP_BIST_HD_AGENT_TASK_STACK
#define CONFIG_ESP_BIST_HD_AGENT_TASK_STACK 3072
#endif

#ifndef CONFIG_ESP_BIST_HD_AGENT_THREAD_COOP_PRIO
#define CONFIG_ESP_BIST_HD_AGENT_THREAD_COOP_PRIO 0
#endif

/*
 * Cooperative priority: the supervision path must answer inside the
 * companion challenge window even when application threads are busy.
 */
#define HD_AGENT_THREAD_PRIO K_PRIO_COOP(CONFIG_ESP_BIST_HD_AGENT_THREAD_COOP_PRIO)

#define LP_STATUS_QUEUE_LEN 4

K_THREAD_STACK_DEFINE(s_agent_stack, CONFIG_ESP_BIST_HD_AGENT_TASK_STACK);
K_MSGQ_DEFINE(s_lp_status_q, sizeof(uint32_t), LP_STATUS_QUEUE_LEN, 4);

static struct k_thread s_agent_thread;
static bool s_worker_started;

static void agent_thread_entry(void *p1, void *p2, void *p3)
{
	bist_hd_worker_fn_t fn = (bist_hd_worker_fn_t)p1;

	ARG_UNUSED(p3);

	fn(p2);
}

int bist_hd_platform_init(void)
{
	return 0;
}

int bist_hd_platform_start_worker(bist_hd_worker_fn_t fn, void *arg)
{
	k_tid_t tid;

	if (fn == NULL || s_worker_started) {
		return -EINVAL;
	}

	tid = k_thread_create(&s_agent_thread, s_agent_stack,
			      K_THREAD_STACK_SIZEOF(s_agent_stack), agent_thread_entry, fn, arg,
			      NULL, HD_AGENT_THREAD_PRIO, 0, K_NO_WAIT);
	if (tid == NULL) {
		return -EAGAIN;
	}
	k_thread_name_set(tid, "bist_hd_agent");

	s_worker_started = true;
	return 0;
}

int bist_hd_platform_lp_status_push(uint32_t status)
{
	return k_msgq_put(&s_lp_status_q, &status, K_NO_WAIT);
}

int bist_hd_platform_lp_status_pop(uint32_t *status_out, int32_t timeout_ms)
{
	k_timeout_t timeout = (timeout_ms < 0) ? K_FOREVER : K_MSEC(timeout_ms);

	if (status_out == NULL) {
		return -EINVAL;
	}

	return k_msgq_get(&s_lp_status_q, status_out, timeout);
}

uint32_t bist_hd_platform_time_ms(void)
{
	return k_uptime_get_32();
}

void bist_hd_platform_sleep_ms(uint32_t ms)
{
	(void)k_msleep((int32_t)ms);
}

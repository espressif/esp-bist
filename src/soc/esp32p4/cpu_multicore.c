/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * This file is part of Espressif's BIST (Built-In Self Test) Library.
 *
 * BIST library is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * BIST library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with BIST library. If not, see
 * <https://www.gnu.org/licenses/lgpl-3.0.html>.
 */

#include "bist_conf.h"

#include <stdatomic.h>
#include <stddef.h>
#include "bist_multicore.h"
#include "bist_soc_multicore.h"
#include "esp_cpu.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "hal/cpu_utility_ll.h"
#include "rom/ets_sys.h"
#include "riscv/interrupt.h"
#include "riscv/rv_utils.h"
#include "soc/hp_sys_clkrst_reg.h"
#include "soc/soc.h"
#include "soc/soc_caps.h"
#include "soc/system_intr.h"
#include "soc/system_reg.h"

extern int _vector_table;
extern int _mtvt_table;
extern void __start_cpu1(void);
extern void core_intr_matrix_clear(void);

typedef struct {
    atomic_uint pending;
    bist_cpu_fn_t fn;
    void *arg;
} bist_cpu_job_t;

static bist_cpu_job_t s_job;
static volatile uint32_t s_cpu1_ready;
static const char *TAG = "multicore";

static void bist_cpu_ipc_isr(void *arg)
{
    (void)arg;
    REG_WRITE(SYSTEM_CPU_INTR_FROM_CPU_3_REG, 0);
}

static void bist_cpu_worker_init(void)
{
    const uint32_t intr_mask = 1U << ETS_IPC_ISR_INUM;
    const uint32_t core_id = esp_cpu_get_core_id();

    esp_cpu_intr_disable(intr_mask);
    REG_WRITE(SYSTEM_CPU_INTR_FROM_CPU_3_REG, 0);
    esp_rom_route_intr_matrix(core_id, SYS_CPU_INTR_FROM_CPU_3_SOURCE, ETS_IPC_ISR_INUM);
    esp_cpu_intr_set_type(ETS_IPC_ISR_INUM, ESP_CPU_INTR_TYPE_LEVEL);
    esp_cpu_intr_set_priority(ETS_IPC_ISR_INUM, SOC_INTERRUPT_LEVEL_MEDIUM);
    esp_cpu_intr_set_handler(ETS_IPC_ISR_INUM, bist_cpu_ipc_isr, NULL);
    esp_cpu_intr_enable(intr_mask);
    rv_utils_intr_global_enable();
}

bist_esp_err_t bist_multicore_call(uint32_t core_id, bist_cpu_fn_t fn, void *arg)
{
    uint32_t self = esp_cpu_get_core_id();

    if (core_id >= SOC_CPU_CORES_NUM || fn == NULL) {
        return BIST_ESP_MULTICORE_ERR;
    }

    if (core_id == self) {
        fn(arg);
        return BIST_ESP_OK;
    }

    if (self != 0 || core_id != 1) {
        return BIST_ESP_MULTICORE_ERR;
    }

    while (atomic_load_explicit(&s_job.pending, memory_order_acquire)) {
    }

    s_job.fn = fn;
    s_job.arg = arg;
    atomic_store_explicit(&s_job.pending, 1, memory_order_release);
    REG_WRITE(SYSTEM_CPU_INTR_FROM_CPU_3_REG, SYSTEM_CPU_INTR_FROM_CPU_3);

    while (atomic_load_explicit(&s_job.pending, memory_order_acquire)) {
    }

    return BIST_ESP_OK;
}

void __attribute__((noreturn)) bist_cpu_worker(void)
{
    bist_cpu_worker_init();

    for (;;) {
        bist_cpu_fn_t fn;
        void *arg;

        rv_utils_intr_global_disable();
        if (!atomic_load_explicit(&s_job.pending, memory_order_acquire)) {
            rv_utils_wait_for_intr();
        }
        rv_utils_intr_global_enable();
        if (!atomic_load_explicit(&s_job.pending, memory_order_acquire)) {
            continue;
        }

        fn = s_job.fn;
        arg = s_job.arg;
        if (fn != NULL) {
            fn(arg);
        }

        atomic_store_explicit(&s_job.pending, 0, memory_order_release);
    }
}

void bist_cpu_start_core1(void)
{
    if (s_cpu1_ready) {
        return;
    }

    REG_SET_BIT(HP_SYS_CLKRST_CPU_WAITI_CTRL0_REG, HP_SYS_CLKRST_REG_CORE1_WAITI_ICG_EN);

    cpu_utility_ll_unstall_cpu(1);
    cpu_utility_ll_enable_clock_and_reset_app_cpu();
    ets_set_appcpu_boot_addr((uint32_t)__start_cpu1);

    while (!s_cpu1_ready) {
        esp_rom_delay_us(100);
    }

    ESP_EARLY_LOGI(TAG, "HP core 1 up");
}

void __start_cpu1(void)
{
    __asm__ __volatile__(".option push\n"
                         ".option norelax\n"
                         "la gp, __global_pointer$\n"
                         ".option pop");
    __asm__ __volatile__("la sp, _stack_top_cpu1");

    rv_utils_enable_fpu();
    esp_cpu_branch_prediction_enable();

    esp_cpu_intr_set_ivt_addr(&_vector_table);
    esp_cpu_intr_set_xtvt_addr(&_mtvt_table);

    ets_set_appcpu_boot_addr(0);

    core_intr_matrix_clear();
    esprv_int_set_threshold(0);

    __asm__ __volatile__("fence" ::: "memory");
    s_cpu1_ready = 1;

    bist_cpu_worker();
}

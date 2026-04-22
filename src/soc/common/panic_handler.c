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

#include <stdint.h>
#include <stddef.h>
#include "riscv/rvruntime-frames.h"
#include "bist_log.h"

static const char *TAG = "panic";

static const char *bist_exc_cause_table[] = {
    [0]  = "Instruction address misaligned",
    [1]  = "Instruction access fault",
    [2]  = "Illegal instruction",
    [3]  = "Breakpoint",
    [4]  = "Load address misaligned",
    [5]  = "Load access fault",
    [6]  = "Store address misaligned",
    [7]  = "Store access fault",
    [8]  = "Environment call from U-mode",
    [9]  = "Environment call from S-mode",
    [11] = "Environment call from M-mode",
    [12] = "Instruction page fault",
    [13] = "Load page fault",
    [15] = "Store page fault",
};

#define NUM_EXC_CAUSE (sizeof(bist_exc_cause_table) / sizeof(bist_exc_cause_table[0]))

void __attribute__((noreturn)) panic_handler_c(RvExcFrame *frame)
{
    uint32_t mcause = frame->mcause;
    uint32_t exc_code = mcause & 0x1F;
    const char *reason = "Unknown";

    if (mcause & 0x80000000) {
        ESP_LOGE(TAG, "Unexpected interrupt during exception handling (mcause=0x%08x)",
                 (unsigned)mcause);
    } else {
        if (exc_code < NUM_EXC_CAUSE && bist_exc_cause_table[exc_code] != NULL) {
            reason = bist_exc_cause_table[exc_code];
        }
        ESP_LOGE(TAG, "Guru Meditation Error: %s (mcause=0x%08x, mtval=0x%08x)",
                 reason, (unsigned)mcause, (unsigned)frame->mtval);
    }

    ESP_LOGE(TAG, "Core %d register dump:", (int)frame->mhartid);
    ESP_LOGE(TAG, "MEPC    : 0x%08x  RA      : 0x%08x  SP      : 0x%08x  GP      : 0x%08x",
             (unsigned)frame->mepc, (unsigned)frame->ra, (unsigned)frame->sp, (unsigned)frame->gp);
    ESP_LOGE(TAG, "TP      : 0x%08x  T0      : 0x%08x  T1      : 0x%08x  T2      : 0x%08x",
             (unsigned)frame->tp, (unsigned)frame->t0, (unsigned)frame->t1, (unsigned)frame->t2);
    ESP_LOGE(TAG, "S0/FP   : 0x%08x  S1      : 0x%08x  A0      : 0x%08x  A1      : 0x%08x",
             (unsigned)frame->s0, (unsigned)frame->s1, (unsigned)frame->a0, (unsigned)frame->a1);
    ESP_LOGE(TAG, "A2      : 0x%08x  A3      : 0x%08x  A4      : 0x%08x  A5      : 0x%08x",
             (unsigned)frame->a2, (unsigned)frame->a3, (unsigned)frame->a4, (unsigned)frame->a5);
    ESP_LOGE(TAG, "A6      : 0x%08x  A7      : 0x%08x  S2      : 0x%08x  S3      : 0x%08x",
             (unsigned)frame->a6, (unsigned)frame->a7, (unsigned)frame->s2, (unsigned)frame->s3);
    ESP_LOGE(TAG, "S4      : 0x%08x  S5      : 0x%08x  S6      : 0x%08x  S7      : 0x%08x",
             (unsigned)frame->s4, (unsigned)frame->s5, (unsigned)frame->s6, (unsigned)frame->s7);
    ESP_LOGE(TAG, "S8      : 0x%08x  S9      : 0x%08x  S10     : 0x%08x  S11     : 0x%08x",
             (unsigned)frame->s8, (unsigned)frame->s9, (unsigned)frame->s10, (unsigned)frame->s11);
    ESP_LOGE(TAG, "T3      : 0x%08x  T4      : 0x%08x  T5      : 0x%08x  T6      : 0x%08x",
             (unsigned)frame->t3, (unsigned)frame->t4, (unsigned)frame->t5, (unsigned)frame->t6);
    ESP_LOGE(TAG, "MSTATUS : 0x%08x  MTVEC   : 0x%08x  MCAUSE  : 0x%08x  MTVAL   : 0x%08x",
             (unsigned)frame->mstatus, (unsigned)frame->mtvec, (unsigned)frame->mcause,
             (unsigned)frame->mtval);
    ESP_LOGE(TAG, "MHARTID : 0x%08x", (unsigned)frame->mhartid);

    while (1) {
        __asm__ volatile("wfi");
    }
}

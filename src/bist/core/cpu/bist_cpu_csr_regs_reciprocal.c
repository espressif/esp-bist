/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * This file is part of Espressif's BIST (Built-In Self Test) Library.
 *
 * BIST library is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * BIST library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with BIST library. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "bist_cpu_csr_regs_reciprocal.h"
#include "bist_multicore.h"
#include "esp_log.h"

#ifndef SOC_TARGET_ESP32P4
# error "Reciprocal CPU CSR test is only supported on ESP32-P4"
#endif

#define CSR_MASK_32BIT      0xFFFFFFFF
#define CSR_MTVEC_MASK      0xFFFFFF00
#define CSR_MEPC_MASK       0xFFFFFFFE
#define CSR_MCAUSE_MASK     0x8000001F
#define CSR_PMPADDR_MASK    0x3FFFFFE0
#define CSR_PMPCFG_MASK     0x0D0D0D0D
#define CSR_PMAADDR_MASK    0x3FFFFFE0
#define CSR_FFLAGS_MASK     0x0000001F
#define CSR_FRM_MASK        0x00000007
#define CSR_FCSR_MASK       0x000000FF
#define CSR_PATTERN_A       0xAAAAAAAA
#define CSR_PATTERN_5       0x55555555

#define RESULT_OFFSET_CSR_A(index) BIST_CPU_CSR_REGS_RESULT_CC_OFFSET_CSR_PATTERN_A(index)
#define RESULT_OFFSET_CSR_5(index) BIST_CPU_CSR_REGS_RESULT_CC_OFFSET_CSR_PATTERN_5(index)

#define ASM_TO_STR_EXPAND(value) #value
#define ASM_TO_STR(value)        ASM_TO_STR_EXPAND(value)

volatile bist_cpu_csr_regs_result_cc_t
bist_cpu_csr_regs_shared_results_cc[BIST_CPU_CSR_REGS_RESULT_CC_SLOT_COUNT] __attribute__((aligned(8)));

/*
 * a4 remains the result base for the complete walk. t0 is the masked
 * expected value, t1 is the read-back, t2 is the original CSR value, and
 * t3 is the CSR mask. The original value is also held at 0(sp) so each
 * failure path can restore the CSR before returning.
 */
#define BIST_CC_TEST_CSR(csr, token, index, mask)                                                                       \
    do {                                                                                                               \
        ASM(" csrr t2, " #csr);                                                                                        \
        ASM(" sw t2, 0(sp)");                                                                                          \
        ASM(" li t3, " ASM_TO_STR(mask));                                                                              \
        ASM(" li t0, " ASM_TO_STR(CSR_PATTERN_A));                                                                      \
        ASM(" and t0, t0, t3");                                                                                        \
        ASM(" csrw " #csr ", t0");                                                                                     \
        ASM(" csrr t1, " #csr);                                                                                        \
        ASM(" sw t1, " ASM_TO_STR(RESULT_OFFSET_CSR_A(index)) "(a4)");                                                  \
        ASM(" and t1, t1, t3");                                                                                        \
        ASM("bist_cc_test_csr_a_" #token ": bne t1, t0, bist_cc_restore_csr_" #token);                                  \
        ASM(" li t0, " ASM_TO_STR(CSR_PATTERN_5));                                                                      \
        ASM(" and t0, t0, t3");                                                                                        \
        ASM(" csrw " #csr ", t0");                                                                                     \
        ASM(" csrr t1, " #csr);                                                                                        \
        ASM(" sw t1, " ASM_TO_STR(RESULT_OFFSET_CSR_5(index)) "(a4)");                                                  \
        ASM(" and t1, t1, t3");                                                                                        \
        ASM("bist_cc_test_csr_5_" #token ": bne t1, t0, bist_cc_restore_csr_" #token);                                  \
        ASM(" lw t2, 0(sp)");                                                                                          \
        ASM(" csrw " #csr ", t2");                                                                                     \
        ASM(" j bist_cc_done_csr_" #token);                                                                            \
        ASM("bist_cc_restore_csr_" #token ": lw t2, 0(sp)");                                                           \
        ASM(" csrw " #csr ", t2");                                                                                     \
        ASM(" j bist_cc_csr_error");                                                                                   \
        ASM("bist_cc_done_csr_" #token ":");                                                                           \
    }                                                                                                                  \
    while (0)

static const char *TAG = "BIST_CPU_CSR_TEST";

static __attribute__((naked, noinline)) bist_esp_err_t bist_cc_cpu_csr_regs_walk(
    volatile bist_cpu_csr_regs_result_cc_t *out __attribute__((unused)))
{
    ASM(" mv a4, a0");
    ASM(" addi sp, sp, -16");

    BIST_CC_TEST_CSR(mtvec, mtvec, 0, CSR_MTVEC_MASK);
    BIST_CC_TEST_CSR(mscratch, mscratch, 1, CSR_MASK_32BIT);
    BIST_CC_TEST_CSR(mepc, mepc, 2, CSR_MEPC_MASK);
    BIST_CC_TEST_CSR(mcause, mcause, 3, CSR_MCAUSE_MASK);
    BIST_CC_TEST_CSR(mtval, mtval, 4, CSR_MASK_32BIT);

    BIST_CC_TEST_CSR(pmpaddr0, pmpaddr0, 5, CSR_PMPADDR_MASK);
    BIST_CC_TEST_CSR(pmpaddr1, pmpaddr1, 6, CSR_PMPADDR_MASK);
    BIST_CC_TEST_CSR(pmpaddr2, pmpaddr2, 7, CSR_PMPADDR_MASK);
    BIST_CC_TEST_CSR(pmpaddr3, pmpaddr3, 8, CSR_PMPADDR_MASK);
    BIST_CC_TEST_CSR(pmpaddr4, pmpaddr4, 9, CSR_PMPADDR_MASK);
    BIST_CC_TEST_CSR(pmpaddr5, pmpaddr5, 10, CSR_PMPADDR_MASK);
    BIST_CC_TEST_CSR(pmpaddr6, pmpaddr6, 11, CSR_PMPADDR_MASK);
    BIST_CC_TEST_CSR(pmpaddr7, pmpaddr7, 12, CSR_PMPADDR_MASK);
    BIST_CC_TEST_CSR(pmpaddr8, pmpaddr8, 13, CSR_PMPADDR_MASK);
    BIST_CC_TEST_CSR(pmpaddr9, pmpaddr9, 14, CSR_PMPADDR_MASK);
    BIST_CC_TEST_CSR(pmpaddr10, pmpaddr10, 15, CSR_PMPADDR_MASK);
    BIST_CC_TEST_CSR(pmpaddr11, pmpaddr11, 16, CSR_PMPADDR_MASK);
    BIST_CC_TEST_CSR(pmpaddr12, pmpaddr12, 17, CSR_PMPADDR_MASK);
    BIST_CC_TEST_CSR(pmpaddr13, pmpaddr13, 18, CSR_PMPADDR_MASK);
    BIST_CC_TEST_CSR(pmpaddr14, pmpaddr14, 19, CSR_PMPADDR_MASK);
    BIST_CC_TEST_CSR(pmpaddr15, pmpaddr15, 20, CSR_PMPADDR_MASK);

    BIST_CC_TEST_CSR(pmpcfg0, pmpcfg0, 21, CSR_PMPCFG_MASK);
    BIST_CC_TEST_CSR(pmpcfg1, pmpcfg1, 22, CSR_PMPCFG_MASK);
    BIST_CC_TEST_CSR(pmpcfg2, pmpcfg2, 23, CSR_PMPCFG_MASK);
    BIST_CC_TEST_CSR(pmpcfg3, pmpcfg3, 24, CSR_PMPCFG_MASK);

    BIST_CC_TEST_CSR(0xBD0, pmaaddr0, 25, CSR_PMAADDR_MASK);
    BIST_CC_TEST_CSR(0xBD1, pmaaddr1, 26, CSR_PMAADDR_MASK);
    BIST_CC_TEST_CSR(0xBD2, pmaaddr2, 27, CSR_PMAADDR_MASK);
    BIST_CC_TEST_CSR(0xBD3, pmaaddr3, 28, CSR_PMAADDR_MASK);
    BIST_CC_TEST_CSR(0xBD4, pmaaddr4, 29, CSR_PMAADDR_MASK);
    BIST_CC_TEST_CSR(0xBD5, pmaaddr5, 30, CSR_PMAADDR_MASK);
    BIST_CC_TEST_CSR(0xBD6, pmaaddr6, 31, CSR_PMAADDR_MASK);
    BIST_CC_TEST_CSR(0xBD7, pmaaddr7, 32, CSR_PMAADDR_MASK);
    BIST_CC_TEST_CSR(0xBD8, pmaaddr8, 33, CSR_PMAADDR_MASK);
    BIST_CC_TEST_CSR(0xBD9, pmaaddr9, 34, CSR_PMAADDR_MASK);
    BIST_CC_TEST_CSR(0xBDA, pmaaddr10, 35, CSR_PMAADDR_MASK);
    BIST_CC_TEST_CSR(0xBDB, pmaaddr11, 36, CSR_PMAADDR_MASK);

    BIST_CC_TEST_CSR(fflags, fflags, 37, CSR_FFLAGS_MASK);
    BIST_CC_TEST_CSR(frm, frm, 38, CSR_FRM_MASK);
    BIST_CC_TEST_CSR(fcsr, fcsr, 39, CSR_FCSR_MASK);

    ASM(" li a0, 0");
    ASM(" addi sp, sp, 16");
    ASM(" ret");

    ASM("bist_cc_csr_error: li a0, 2");
    ASM(" addi sp, sp, 16");
    ASM(" ret");
}

static const uint32_t s_csr_masks[BIST_CPU_CSR_REGS_RESULT_CC_CSR_COUNT] = {
    [BIST_CPU_CSR_REGS_CC_INDEX_MTVEC] = CSR_MTVEC_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_MSCRATCH] = CSR_MASK_32BIT,
    [BIST_CPU_CSR_REGS_CC_INDEX_MEPC] = CSR_MEPC_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_MCAUSE] = CSR_MCAUSE_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_MTVAL] = CSR_MASK_32BIT,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR0] = CSR_PMPADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR1] = CSR_PMPADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR2] = CSR_PMPADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR3] = CSR_PMPADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR4] = CSR_PMPADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR5] = CSR_PMPADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR6] = CSR_PMPADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR7] = CSR_PMPADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR8] = CSR_PMPADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR9] = CSR_PMPADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR10] = CSR_PMPADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR11] = CSR_PMPADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR12] = CSR_PMPADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR13] = CSR_PMPADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR14] = CSR_PMPADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMPADDR15] = CSR_PMPADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMPCFG0] = CSR_PMPCFG_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMPCFG1] = CSR_PMPCFG_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMPCFG2] = CSR_PMPCFG_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMPCFG3] = CSR_PMPCFG_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR0] = CSR_PMAADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR1] = CSR_PMAADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR2] = CSR_PMAADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR3] = CSR_PMAADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR4] = CSR_PMAADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR5] = CSR_PMAADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR6] = CSR_PMAADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR7] = CSR_PMAADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR8] = CSR_PMAADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR9] = CSR_PMAADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR10] = CSR_PMAADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_PMAADDR11] = CSR_PMAADDR_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_FFLAGS] = CSR_FFLAGS_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_FRM] = CSR_FRM_MASK,
    [BIST_CPU_CSR_REGS_CC_INDEX_FCSR] = CSR_FCSR_MASK,
};

static void bist_cpu_csr_regs_test_cc_job(void *arg)
{
    volatile bist_cpu_csr_regs_result_cc_t *out = (volatile bist_cpu_csr_regs_result_cc_t *)arg;

    if (out == NULL) {
        return;
    }

    volatile uint32_t *words = (volatile uint32_t *)out;
    for (uint32_t i = 0; i < (sizeof(*out) / sizeof(*words)); ++i) {
        words[i] = 0;
    }

    out->status = (uint32_t)bist_cc_cpu_csr_regs_walk(out);
}

static bist_esp_err_t verify_bist_cpu_csr_regs_test_result(volatile bist_cpu_csr_regs_result_cc_t *result)
{
    if (result == NULL || result->status != (uint32_t)BIST_ESP_OK) {
        return BIST_ESP_CPU_CSR_TEST_ERR;
    }

    for (uint32_t i = 0; i < BIST_CPU_CSR_REGS_RESULT_CC_CSR_COUNT; ++i) {
        const uint32_t mask = s_csr_masks[i];
        const uint32_t expected_a = BIST_CPU_CSR_REGS_RESULT_CC_PATTERN_A & mask;
        const uint32_t expected_5 = BIST_CPU_CSR_REGS_RESULT_CC_PATTERN_5 & mask;

        if ((result->csr[i].pattern_a & mask) != expected_a ||
                (result->csr[i].pattern_5 & mask) != expected_5) {
            return BIST_ESP_CPU_CSR_TEST_ERR;
        }
    }

    return BIST_ESP_OK;
}

static void verify_bist_cpu_csr_regs_test_result_job(void *arg)
{
    volatile bist_cpu_csr_regs_result_cc_t *result = (volatile bist_cpu_csr_regs_result_cc_t *)arg;

    if (result == NULL) {
        return;
    }

    result->status = (uint32_t)verify_bist_cpu_csr_regs_test_result(result);
}

bist_esp_err_t bist_cpu_csr_regs_test(void)
{
    volatile bist_cpu_csr_regs_result_cc_t *test_result_core_0 = &bist_cpu_csr_regs_shared_results_cc[0];
    volatile bist_cpu_csr_regs_result_cc_t *test_result_core_1 = &bist_cpu_csr_regs_shared_results_cc[1];
    bist_esp_err_t status;
    int ret;

    bist_cpu_csr_regs_test_cc_job((void *)test_result_core_0);
    if (test_result_core_0->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to execute CSR walk on core 0");
        return BIST_ESP_CPU_CSR_TEST_ERR;
    }

    status = verify_bist_cpu_csr_regs_test_result(test_result_core_0);
    if (status != BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to validate core 0 CSR results on core 0");
        return BIST_ESP_CPU_CSR_TEST_ERR;
    }

    ret = bist_multicore_call(1, bist_cpu_csr_regs_test_cc_job, (void *)test_result_core_1);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to dispatch CSR walk to core 1");
        return BIST_ESP_CPU_CSR_TEST_ERR;
    }
    if (test_result_core_1->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to execute CSR walk on core 1");
        return BIST_ESP_CPU_CSR_TEST_ERR;
    }

    ret = bist_multicore_call(1, verify_bist_cpu_csr_regs_test_result_job, (void *)test_result_core_1);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to dispatch validation of core 1 CSR results to core 1");
        return BIST_ESP_CPU_CSR_TEST_ERR;
    }
    if (test_result_core_1->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to validate core 1 CSR results on core 1");
        return BIST_ESP_CPU_CSR_TEST_ERR;
    }

    ret = bist_multicore_call(1, verify_bist_cpu_csr_regs_test_result_job, (void *)test_result_core_0);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to dispatch validation of core 0 CSR results to core 1");
        return BIST_ESP_CPU_CSR_TEST_ERR;
    }
    if (test_result_core_0->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to validate core 0 CSR results on core 1");
        return BIST_ESP_CPU_CSR_TEST_ERR;
    }

    status = verify_bist_cpu_csr_regs_test_result(test_result_core_1);
    if (status != BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to validate core 1 CSR results on core 0");
        return BIST_ESP_CPU_CSR_TEST_ERR;
    }

    return BIST_ESP_OK;
}

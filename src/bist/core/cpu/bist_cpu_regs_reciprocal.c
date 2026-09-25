/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
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

#include "bist_cpu_regs_reciprocal.h"
#include "bist_conf.h"
#include "bist_multicore.h"
#include "esp_log.h"

#ifndef SOC_TARGET_ESP32P4
# error "Reciprocal CPU register test is only supported on ESP32-P4"
#endif

/* Byte offsets within bist_cpu_regs_result_cc_t */
#define RESULT_OFFSET_FEATURE_FLAGS BIST_CPU_REGS_RESULT_CC_OFFSET_FEATURE_FLAGS
#define RESULT_OFFSET_STATUS        BIST_CPU_REGS_RESULT_CC_OFFSET_STATUS

/* Byte offsets for register read-back array entries (each entry is 8 bytes: pattern_a at +0, pattern_5 at +4) */
#define RESULT_OFFSET_GPR_A(index) BIST_CPU_REGS_RESULT_CC_OFFSET_GPR_ENTRY(index)
#define RESULT_OFFSET_GPR_5(index) (RESULT_OFFSET_GPR_A(index) + 4)
#define RESULT_OFFSET_FPR_A(index) BIST_CPU_REGS_RESULT_CC_OFFSET_FPR_ENTRY(index)
#define RESULT_OFFSET_FPR_5(index) (RESULT_OFFSET_FPR_A(index) + 4)

/* Helper to format numerical offsets and constants into inline assembly string literals */
#define ASM_TO_STR_EXPAND(val) #val
#define ASM_TO_STR(val)        ASM_TO_STR_EXPAND(val)

static const char *TAG = "BIST_CPU_REG_TEST";

volatile bist_cpu_regs_result_cc_t bist_cpu_regs_shared_results_cc[BIST_CPU_REGS_RESULT_CC_SLOT_COUNT]
__attribute__((aligned(8)));

/*
 * Tests an unstacked GPR with 0xAAAAAAAA and 0x55555555 checkerboard patterns.
 * Captures the raw register value before independently loading and comparing
 * the expected value. expected_reg must differ from reg and base_reg.
 * reg: target test register
 * index: register index (0-31)
 * base_reg: result base register (t6)
 * expected_reg: expected value register
 * error_label: error label
 */
#define BIST_CC_TEST_GPR_NOT_STACKED(reg, index, base_reg, expected_reg, error_label)                                    \
    do {                                                                                                               \
        ASM(" li " #reg ", 0xAAAAAAAA");                                                                               \
        ASM(" sw " #reg ", " ASM_TO_STR(RESULT_OFFSET_GPR_A(index)) "(" #base_reg ")");                                \
        ASM(" li " #expected_reg ", 0xAAAAAAAA");                                                                      \
        ASM("bist_cc_test_gpr_a_" #reg ": bne " #reg ", " #expected_reg ", " #error_label);                              \
        ASM(" li " #reg ", 0x55555555");                                                                               \
        ASM(" sw " #reg ", " ASM_TO_STR(RESULT_OFFSET_GPR_5(index)) "(" #base_reg ")");                                \
        ASM(" li " #expected_reg ", 0x55555555");                                                                      \
        ASM("bist_cc_test_gpr_5_" #reg ": bne " #reg ", " #expected_reg ", " #error_label);                              \
    }                                                                                                                  \
    while (0)

/*
 * Tests a callee-saved or ABI-critical GPR.
 * Saves the register on stack at 4(sp), runs the test using t6 as result base,
 * and restores the original value from 4(sp) on success.
 */
#define BIST_CC_TEST_GPR_STACKED(reg, index, expected_reg)                                                               \
    do {                                                                                                               \
        ASM(" sw " #reg ", 4(sp)");                                                                                    \
        BIST_CC_TEST_GPR_NOT_STACKED(reg, index, t6, expected_reg, bist_cc_restore_gpr_##reg);                             \
        ASM(" lw " #reg ", 4(sp)");                                                                                    \
    }                                                                                                                  \
    while (0)

/*
 * Failure restore handler for stacked GPRs: restores the saved register from
 * 4(sp) before jumping to the common error path.
 */
#define BIST_CC_GPR_RESTORE_HANDLER(reg)                                                                                 \
    do {                                                                                                               \
        ASM("bist_cc_restore_gpr_" #reg ": lw " #reg ", 4(sp)");                                                         \
        ASM(" j bist_cc_error");                                                                                         \
    }                                                                                                                  \
    while (0)

#if defined(ESP_BIST_USE_FPU)
/*
 * Tests an unstacked single-precision floating-point register (FPR).
 * Stores raw binary32 bits with fsw before independently loading and comparing
 * the expected value using feq.s.
 */
#define BIST_CC_TEST_FPR_NOT_STACKED(reg, index, f_expected_reg, expected_reg, error_label)                              \
    do {                                                                                                               \
        ASM(" li " #expected_reg ", 0xAAAAAAAA");                                                                      \
        ASM(" fmv.w.x " #reg ", " #expected_reg);                                                                      \
        ASM(" fsw " #reg ", " ASM_TO_STR(RESULT_OFFSET_FPR_A(index)) "(t6)");                                          \
        ASM(" li " #expected_reg ", 0xAAAAAAAA");                                                                      \
        ASM(" fmv.w.x " #f_expected_reg ", " #expected_reg);                                                           \
        ASM("bist_cc_test_fpr_a_" #reg ": feq.s " #expected_reg ", " #reg ", " #f_expected_reg);                         \
        ASM(" beqz " #expected_reg ", " #error_label);                                                                 \
        ASM(" li " #expected_reg ", 0x55555555");                                                                      \
        ASM(" fmv.w.x " #reg ", " #expected_reg);                                                                      \
        ASM(" fsw " #reg ", " ASM_TO_STR(RESULT_OFFSET_FPR_5(index)) "(t6)");                                          \
        ASM(" li " #expected_reg ", 0x55555555");                                                                      \
        ASM(" fmv.w.x " #f_expected_reg ", " #expected_reg);                                                           \
        ASM("bist_cc_test_fpr_5_" #reg ": feq.s " #expected_reg ", " #reg ", " #f_expected_reg);                         \
        ASM(" beqz " #expected_reg ", " #error_label);                                                                 \
    }                                                                                                                  \
    while (0)

#define BIST_CC_TEST_FPR_STACKED(reg, index, f_expected_reg, expected_reg)                                               \
    do {                                                                                                               \
        ASM(" fsw " #reg ", 4(sp)");                                                                                   \
        BIST_CC_TEST_FPR_NOT_STACKED(reg, index, f_expected_reg, expected_reg, bist_cc_restore_fpr_##reg);                 \
        ASM(" flw " #reg ", 4(sp)");                                                                                   \
    }                                                                                                                  \
    while (0)

#define BIST_CC_FPR_RESTORE_HANDLER(reg)                                                                                 \
    do {                                                                                                               \
        ASM("bist_cc_restore_fpr_" #reg ": flw " #reg ", 4(sp)");                                                        \
        ASM(" j bist_cc_error");                                                                                         \
    }                                                                                                                  \
    while (0)
#endif

#if defined(CONFIG_ESP_BIST_CPU_REG_TEST)

/*
 * a0 is the result base while t0-t6 are tested. Once t6 has passed, t6 owns
 * the base for the remainder of the walk, allowing a0, sp, and gp to be tested.
 */
static __attribute__((naked, noinline)) bist_esp_err_t bist_cc_cpu_regs_walk(
    volatile bist_cpu_regs_result_cc_t *out __attribute__((unused)))
{
    ASM(" addi sp, sp, -16");

    BIST_CC_TEST_GPR_NOT_STACKED(t0, 5, a0, t1, bist_cc_early_error);
    BIST_CC_TEST_GPR_NOT_STACKED(t1, 6, a0, t0, bist_cc_early_error);
    BIST_CC_TEST_GPR_NOT_STACKED(t2, 7, a0, t0, bist_cc_early_error);
    BIST_CC_TEST_GPR_NOT_STACKED(t3, 28, a0, t0, bist_cc_early_error);
    BIST_CC_TEST_GPR_NOT_STACKED(t4, 29, a0, t0, bist_cc_early_error);
    BIST_CC_TEST_GPR_NOT_STACKED(t5, 30, a0, t0, bist_cc_early_error);
    BIST_CC_TEST_GPR_NOT_STACKED(t6, 31, a0, t0, bist_cc_early_error);

    ASM(" mv t6, a0");
    ASM(" sw ra, 0(sp)");

    BIST_CC_TEST_GPR_STACKED(ra, 1, t0);

    ASM(" mv t3, sp");
    BIST_CC_TEST_GPR_NOT_STACKED(sp, 2, t6, t0, bist_cc_restore_sp);
    ASM(" mv sp, t3");

    BIST_CC_TEST_GPR_STACKED(gp, 3, t0);
    BIST_CC_TEST_GPR_STACKED(tp, 4, t0);
    BIST_CC_TEST_GPR_STACKED(s0, 8, t0);
    BIST_CC_TEST_GPR_STACKED(s1, 9, t0);
    BIST_CC_TEST_GPR_STACKED(s2, 18, t0);
    BIST_CC_TEST_GPR_STACKED(s3, 19, t0);
    BIST_CC_TEST_GPR_STACKED(s4, 20, t0);
    BIST_CC_TEST_GPR_STACKED(s5, 21, t0);
    BIST_CC_TEST_GPR_STACKED(s6, 22, t0);
    BIST_CC_TEST_GPR_STACKED(s7, 23, t0);
    BIST_CC_TEST_GPR_STACKED(s8, 24, t0);
    BIST_CC_TEST_GPR_STACKED(s9, 25, t0);
    BIST_CC_TEST_GPR_STACKED(s10, 26, t0);
    BIST_CC_TEST_GPR_STACKED(s11, 27, t0);
    BIST_CC_TEST_GPR_NOT_STACKED(a0, 10, t6, t0, bist_cc_error);
    BIST_CC_TEST_GPR_NOT_STACKED(a1, 11, t6, t0, bist_cc_error);
    BIST_CC_TEST_GPR_NOT_STACKED(a2, 12, t6, t0, bist_cc_error);
    BIST_CC_TEST_GPR_NOT_STACKED(a3, 13, t6, t0, bist_cc_error);
    BIST_CC_TEST_GPR_NOT_STACKED(a4, 14, t6, t0, bist_cc_error);
    BIST_CC_TEST_GPR_NOT_STACKED(a5, 15, t6, t0, bist_cc_error);
    BIST_CC_TEST_GPR_NOT_STACKED(a6, 16, t6, t0, bist_cc_error);
    BIST_CC_TEST_GPR_NOT_STACKED(a7, 17, t6, t0, bist_cc_error);

#if defined(ESP_BIST_USE_FPU)
    ASM(" li t0, " ASM_TO_STR(BIST_CPU_REGS_RESULT_CC_FEATURE_FPR_TESTED));
    ASM(" sw t0, " ASM_TO_STR(RESULT_OFFSET_FEATURE_FLAGS) "(t6)");

    BIST_CC_TEST_FPR_NOT_STACKED(ft0, 0, ft1, t0, bist_cc_error);
    BIST_CC_TEST_FPR_NOT_STACKED(ft1, 1, ft0, t0, bist_cc_error);
    BIST_CC_TEST_FPR_NOT_STACKED(ft2, 2, ft0, t0, bist_cc_error);
    BIST_CC_TEST_FPR_NOT_STACKED(ft3, 3, ft0, t0, bist_cc_error);
    BIST_CC_TEST_FPR_NOT_STACKED(ft4, 4, ft0, t0, bist_cc_error);
    BIST_CC_TEST_FPR_NOT_STACKED(ft5, 5, ft0, t0, bist_cc_error);
    BIST_CC_TEST_FPR_NOT_STACKED(ft6, 6, ft0, t0, bist_cc_error);
    BIST_CC_TEST_FPR_NOT_STACKED(ft7, 7, ft0, t0, bist_cc_error);
    BIST_CC_TEST_FPR_NOT_STACKED(ft8, 28, ft0, t0, bist_cc_error);
    BIST_CC_TEST_FPR_NOT_STACKED(ft9, 29, ft0, t0, bist_cc_error);
    BIST_CC_TEST_FPR_NOT_STACKED(ft10, 30, ft0, t0, bist_cc_error);
    BIST_CC_TEST_FPR_NOT_STACKED(ft11, 31, ft0, t0, bist_cc_error);
    BIST_CC_TEST_FPR_STACKED(fs0, 8, ft0, t0);
    BIST_CC_TEST_FPR_STACKED(fs1, 9, ft0, t0);
    BIST_CC_TEST_FPR_STACKED(fs2, 18, ft0, t0);
    BIST_CC_TEST_FPR_STACKED(fs3, 19, ft0, t0);
    BIST_CC_TEST_FPR_STACKED(fs4, 20, ft0, t0);
    BIST_CC_TEST_FPR_STACKED(fs5, 21, ft0, t0);
    BIST_CC_TEST_FPR_STACKED(fs6, 22, ft0, t0);
    BIST_CC_TEST_FPR_STACKED(fs7, 23, ft0, t0);
    BIST_CC_TEST_FPR_STACKED(fs8, 24, ft0, t0);
    BIST_CC_TEST_FPR_STACKED(fs9, 25, ft0, t0);
    BIST_CC_TEST_FPR_STACKED(fs10, 26, ft0, t0);
    BIST_CC_TEST_FPR_STACKED(fs11, 27, ft0, t0);
    BIST_CC_TEST_FPR_NOT_STACKED(fa0, 10, ft0, t0, bist_cc_error);
    BIST_CC_TEST_FPR_NOT_STACKED(fa1, 11, ft0, t0, bist_cc_error);
    BIST_CC_TEST_FPR_NOT_STACKED(fa2, 12, ft0, t0, bist_cc_error);
    BIST_CC_TEST_FPR_NOT_STACKED(fa3, 13, ft0, t0, bist_cc_error);
    BIST_CC_TEST_FPR_NOT_STACKED(fa4, 14, ft0, t0, bist_cc_error);
    BIST_CC_TEST_FPR_NOT_STACKED(fa5, 15, ft0, t0, bist_cc_error);
    BIST_CC_TEST_FPR_NOT_STACKED(fa6, 16, ft0, t0, bist_cc_error);
    BIST_CC_TEST_FPR_NOT_STACKED(fa7, 17, ft0, t0, bist_cc_error);
#endif

    ASM(" li a0, 0");
    ASM(" lw ra, 0(sp)");
    ASM(" addi sp, sp, 16");
    ASM(" ret");

    ASM("bist_cc_restore_sp: mv sp, t3");
    ASM(" j bist_cc_error");

    BIST_CC_GPR_RESTORE_HANDLER(ra);
    BIST_CC_GPR_RESTORE_HANDLER(gp);
    BIST_CC_GPR_RESTORE_HANDLER(tp);
    BIST_CC_GPR_RESTORE_HANDLER(s0);
    BIST_CC_GPR_RESTORE_HANDLER(s1);
    BIST_CC_GPR_RESTORE_HANDLER(s2);
    BIST_CC_GPR_RESTORE_HANDLER(s3);
    BIST_CC_GPR_RESTORE_HANDLER(s4);
    BIST_CC_GPR_RESTORE_HANDLER(s5);
    BIST_CC_GPR_RESTORE_HANDLER(s6);
    BIST_CC_GPR_RESTORE_HANDLER(s7);
    BIST_CC_GPR_RESTORE_HANDLER(s8);
    BIST_CC_GPR_RESTORE_HANDLER(s9);
    BIST_CC_GPR_RESTORE_HANDLER(s10);
    BIST_CC_GPR_RESTORE_HANDLER(s11);

#if defined(ESP_BIST_USE_FPU)
    BIST_CC_FPR_RESTORE_HANDLER(fs0);
    BIST_CC_FPR_RESTORE_HANDLER(fs1);
    BIST_CC_FPR_RESTORE_HANDLER(fs2);
    BIST_CC_FPR_RESTORE_HANDLER(fs3);
    BIST_CC_FPR_RESTORE_HANDLER(fs4);
    BIST_CC_FPR_RESTORE_HANDLER(fs5);
    BIST_CC_FPR_RESTORE_HANDLER(fs6);
    BIST_CC_FPR_RESTORE_HANDLER(fs7);
    BIST_CC_FPR_RESTORE_HANDLER(fs8);
    BIST_CC_FPR_RESTORE_HANDLER(fs9);
    BIST_CC_FPR_RESTORE_HANDLER(fs10);
    BIST_CC_FPR_RESTORE_HANDLER(fs11);
#endif

    ASM("bist_cc_error: li a0, 1");
    ASM(" lw ra, 0(sp)");
    ASM(" addi sp, sp, 16");
    ASM(" ret");

    ASM("bist_cc_early_error: li a0, 1");
    ASM(" addi sp, sp, 16");
    ASM(" ret");
}

static void bist_cpu_regs_test_cc(volatile bist_cpu_regs_result_cc_t *out)
{
    if (out == NULL) {
        out->status = BIST_ESP_CPU_TEST_ERR;
        return;
    }

    volatile uint32_t *words = (volatile uint32_t *)out;
    for (uint32_t i = 0; i < (sizeof(*out) / sizeof(*words)); ++i) {
        words[i] = 0;
    }

    bist_esp_err_t status = bist_cc_cpu_regs_walk(out);
    out->status = (uint32_t)status;
}

static bist_esp_err_t verify_bist_cpu_regs_test_result(volatile bist_cpu_regs_result_cc_t *result)
{
    /* x0 is hardwired zero and is not walked. */
    for (uint32_t i = 1; i < BIST_CPU_REGS_RESULT_CC_GPR_COUNT; ++i) {
        if (result->gpr[i].pattern_a != BIST_CPU_REGS_RESULT_CC_PATTERN_A ||
                result->gpr[i].pattern_5 != BIST_CPU_REGS_RESULT_CC_PATTERN_5) {
            ESP_LOGE(TAG, "GPR %d failed check", i);
            return BIST_ESP_CPU_TEST_ERR;
        }
    }

    /* Check if floating-point registers were tested */

    if ((result->feature_flags & BIST_CPU_REGS_RESULT_CC_FEATURE_FPR_TESTED) == 0) {
        return BIST_ESP_OK;
    }

    for (uint32_t i = 0; i < BIST_CPU_REGS_RESULT_CC_FPR_COUNT; ++i) {
        if (result->fpr[i].pattern_a != BIST_CPU_REGS_RESULT_CC_PATTERN_A ||
                result->fpr[i].pattern_5 != BIST_CPU_REGS_RESULT_CC_PATTERN_5) {
            ESP_LOGE(TAG, "FPR %d failed check", i);
            return BIST_ESP_CPU_TEST_ERR;
        }
    }

    return BIST_ESP_OK;
}

static void verify_bist_cpu_regs_test_result_job(void *arg)
{
    volatile bist_cpu_regs_result_cc_t *result = (volatile bist_cpu_regs_result_cc_t *)arg;

    result->status = (uint32_t)verify_bist_cpu_regs_test_result(result);
}

bist_esp_err_t bist_cpu_regs_test(void)
{
    volatile bist_cpu_regs_result_cc_t *test_result_core_0 = &bist_cpu_regs_shared_results_cc[0];
    volatile bist_cpu_regs_result_cc_t *test_result_core_1 = &bist_cpu_regs_shared_results_cc[1];
    bist_esp_err_t status;
    int ret;

    /* Execute and validate test on core 0 */

    bist_cpu_regs_test_cc(test_result_core_0);
    if (test_result_core_0->status != BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to execute register walk on core 0");
        return BIST_ESP_CPU_TEST_ERR;
    }

    status = verify_bist_cpu_regs_test_result(test_result_core_0);
    if (status != BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to validate core 0 results on core 0");
        return BIST_ESP_CPU_TEST_ERR;
    }

    /* Execute and validate test on core 1 */

    ret = bist_multicore_call(1, (void *)bist_cpu_regs_test_cc, (void *)test_result_core_1);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to dispatch register walk to core 1");
        return BIST_ESP_CPU_TEST_ERR;
    }
    if (test_result_core_1->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to execute register walk on core 1");
        return BIST_ESP_CPU_TEST_ERR;
    }

    ret = bist_multicore_call(1, verify_bist_cpu_regs_test_result_job, (void *)test_result_core_1);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to dispatch validation of core 1 results to core 1");
        return BIST_ESP_CPU_TEST_ERR;
    }
    if (test_result_core_1->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to validate core 1 results on core 1");
        return BIST_ESP_CPU_TEST_ERR;
    }

    /* Validate core 0 results on core 1 */

    ret = bist_multicore_call(1, verify_bist_cpu_regs_test_result_job, (void *)test_result_core_0);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to dispatch validation of core 0 results to core 1");
        return BIST_ESP_CPU_TEST_ERR;
    }
    if (test_result_core_0->status != (uint32_t)BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to validate core 0 results on core 1");
        return BIST_ESP_CPU_TEST_ERR;
    }

    /* Validate core 1 results on core 0 */

    status = verify_bist_cpu_regs_test_result(test_result_core_1);
    if (status != BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to validate core 1 results on core 0");
        return BIST_ESP_CPU_TEST_ERR;
    }

    return BIST_ESP_OK;
}

#endif // CONFIG_ESP_BIST_CPU_REG_TEST

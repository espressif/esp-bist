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

#include "bist_cpu_instructions.h"
#include "bist_log.h"

static const char *TAG = "bist_cpu_insn";

/** Written by group asm on failure; cleared on pass. */
uint32_t bist_cpu_instruction_failure_id;

/** Set by C when a group returns error; NONE on start/success. */
static bist_insn_group_t bist_cpu_instruction_failure_group;

#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_INTEGER)
bist_esp_err_t bist_cpu_instruction_integer(void);
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_SHIFT)
bist_esp_err_t bist_cpu_instruction_shift(void);
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_UPPERIMM)
bist_esp_err_t bist_cpu_instruction_upperimm(void);
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_CTRL)
bist_esp_err_t bist_cpu_instruction_ctrl(void);
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_LDST)
bist_esp_err_t bist_cpu_instruction_ldst(void);
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_MUL)
bist_esp_err_t bist_cpu_instruction_mul(void);
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_CSR)
bist_esp_err_t bist_cpu_instruction_csr(void);
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_AMO) && !defined(SOC_TARGET_ESP32C3)
bist_esp_err_t bist_cpu_instruction_amo(void);
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_FP) && defined(ESP_BIST_USE_FPU)
bist_esp_err_t bist_cpu_instruction_fp(void);
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_COMP)
bist_esp_err_t bist_cpu_instruction_comp(void);
#endif

bist_esp_err_t bist_cpu_instruction_test(void)
{
    bist_esp_err_t err = BIST_ESP_OK;

    bist_cpu_instruction_failure_id = 0;
    bist_cpu_instruction_failure_group = BIST_INSN_GROUP_NONE;

#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_INTEGER)
    err = bist_cpu_instruction_integer();
    if (err != BIST_ESP_OK) {
        bist_cpu_instruction_failure_group = BIST_INSN_GROUP_INTEGER;
        ESP_LOGE(TAG, "integer ALU failed, vector %lu",
                 (unsigned long)bist_cpu_instruction_failure_id);
        return err;
    }
    ESP_LOGD(TAG, "integer ALU tests passed");
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_SHIFT)
    err = bist_cpu_instruction_shift();
    if (err != BIST_ESP_OK) {
        bist_cpu_instruction_failure_group = BIST_INSN_GROUP_SHIFT;
        ESP_LOGE(TAG, "shifts failed, vector %lu",
                 (unsigned long)bist_cpu_instruction_failure_id);
        return err;
    }
    ESP_LOGD(TAG, "shifts tests passed");
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_UPPERIMM)
    err = bist_cpu_instruction_upperimm();
    if (err != BIST_ESP_OK) {
        bist_cpu_instruction_failure_group = BIST_INSN_GROUP_UPPERIMM;
        ESP_LOGE(TAG, "LUI/AUIPC failed, vector %lu",
                 (unsigned long)bist_cpu_instruction_failure_id);
        return err;
    }
    ESP_LOGD(TAG, "LUI/AUIPC tests passed");
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_CTRL)
    err = bist_cpu_instruction_ctrl();
    if (err != BIST_ESP_OK) {
        bist_cpu_instruction_failure_group = BIST_INSN_GROUP_CTRL;
        ESP_LOGE(TAG, "control flow failed, vector %lu",
                 (unsigned long)bist_cpu_instruction_failure_id);
        return err;
    }
    ESP_LOGD(TAG, "control flow passed");
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_LDST)
    err = bist_cpu_instruction_ldst();
    if (err != BIST_ESP_OK) {
        bist_cpu_instruction_failure_group = BIST_INSN_GROUP_LDST;
        ESP_LOGE(TAG, "load/store failed, vector %lu",
                 (unsigned long)bist_cpu_instruction_failure_id);
        return err;
    }
    ESP_LOGD(TAG, "load/store tests passed");
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_MUL)
    err = bist_cpu_instruction_mul();
    if (err != BIST_ESP_OK) {
        bist_cpu_instruction_failure_group = BIST_INSN_GROUP_MUL;
        ESP_LOGE(TAG, "multiply/divide failed, vector %lu",
                 (unsigned long)bist_cpu_instruction_failure_id);
        return err;
    }
    ESP_LOGD(TAG, "multiply/divide tests passed");
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_CSR)
    err = bist_cpu_instruction_csr();
    if (err != BIST_ESP_OK) {
        bist_cpu_instruction_failure_group = BIST_INSN_GROUP_CSR;
        ESP_LOGE(TAG, "CSR/fence failed, vector %lu",
                 (unsigned long)bist_cpu_instruction_failure_id);
        return err;
    }
    ESP_LOGD(TAG, "CSR/fence tests passed");
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_AMO) && !defined(SOC_TARGET_ESP32C3)
    err = bist_cpu_instruction_amo();
    if (err != BIST_ESP_OK) {
        bist_cpu_instruction_failure_group = BIST_INSN_GROUP_AMO;
        ESP_LOGE(TAG, "atomics failed, vector %lu",
                 (unsigned long)bist_cpu_instruction_failure_id);
        return err;
    }
    ESP_LOGD(TAG, "atomics tests passed");
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_FP) && defined(ESP_BIST_USE_FPU)
    err = bist_cpu_instruction_fp();
    if (err != BIST_ESP_OK) {
        bist_cpu_instruction_failure_group = BIST_INSN_GROUP_FP;
        ESP_LOGE(TAG, "floating point failed, vector %lu",
                 (unsigned long)bist_cpu_instruction_failure_id);
        return err;
    }
    ESP_LOGD(TAG, "floating point tests passed");
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_COMP)
    err = bist_cpu_instruction_comp();
    if (err != BIST_ESP_OK) {
        bist_cpu_instruction_failure_group = BIST_INSN_GROUP_COMP;
        ESP_LOGE(TAG, "compressed instructions failed, vector %lu",
                 (unsigned long)bist_cpu_instruction_failure_id);
        return err;
    }
    ESP_LOGD(TAG, "compressed instruction tests passed");
#endif

    bist_cpu_instruction_failure_id = 0;
    bist_cpu_instruction_failure_group = BIST_INSN_GROUP_NONE;
    return BIST_ESP_OK;
}

uint32_t bist_cpu_instruction_last_failure_id(void)
{
    return bist_cpu_instruction_failure_id;
}

bist_insn_group_t bist_cpu_instruction_last_failure_group(void)
{
    return bist_cpu_instruction_failure_group;
}

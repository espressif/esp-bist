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

bist_esp_err_t bist_cpu_instruction_test(void)
{
    bist_esp_err_t err = BIST_ESP_OK;

    bist_cpu_instruction_failure_id = 0;
    bist_cpu_instruction_failure_group = BIST_INSN_GROUP_NONE;

#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_INTEGER)
    err = bist_cpu_instruction_integer();
    if (err != BIST_ESP_OK) {
        bist_cpu_instruction_failure_group = BIST_INSN_GROUP_INTEGER;
        return err;
    }
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_SHIFT)
    err = bist_cpu_instruction_shift();
    if (err != BIST_ESP_OK) {
        bist_cpu_instruction_failure_group = BIST_INSN_GROUP_SHIFT;
        return err;
    }
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_UPPERIMM)
    err = bist_cpu_instruction_upperimm();
    if (err != BIST_ESP_OK) {
        bist_cpu_instruction_failure_group = BIST_INSN_GROUP_UPPERIMM;
        return err;
    }
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_CTRL)
    err = bist_cpu_instruction_ctrl();
    if (err != BIST_ESP_OK) {
        bist_cpu_instruction_failure_group = BIST_INSN_GROUP_CTRL;
        return err;
    }
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_LDST)
    err = bist_cpu_instruction_ldst();
    if (err != BIST_ESP_OK) {
        bist_cpu_instruction_failure_group = BIST_INSN_GROUP_LDST;
        return err;
    }
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_MUL)
    err = bist_cpu_instruction_mul();
    if (err != BIST_ESP_OK) {
        bist_cpu_instruction_failure_group = BIST_INSN_GROUP_MUL;
        return err;
    }
#endif
#if defined(CONFIG_ESP_BIST_CPU_INSTRUCTION_CSR)
    err = bist_cpu_instruction_csr();
    if (err != BIST_ESP_OK) {
        bist_cpu_instruction_failure_group = BIST_INSN_GROUP_CSR;
        return err;
    }
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

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

/**
 * @file bist_cpu_instructions.h
 * @brief CPU instruction equivalence-class self-test (IEC 60730 H.2.18.5)
 */

#pragma once

#include "stdint.h"
#include "bist_esp_types.h"
#ifndef __ZEPHYR__
#include "bist_conf.h"
#endif

#include "bist_cpu_instructions_integer.h"
#include "bist_cpu_instructions_shift.h"
#include "bist_cpu_instructions_upperimm.h"
#include "bist_cpu_instructions_ctrl.h"
#include "bist_cpu_instructions_ldst.h"
#include "bist_cpu_instructions_mul.h"
#include "bist_cpu_instructions_csr.h"

/**
 * Instruction group for last-failure reporting.
 * Pair with bist_cpu_instruction_last_failure_id(): id 0 and
 * BIST_INSN_GROUP_NONE mean pass/not failed; on fail, the group
 * identifies which per-group enum the id belongs to.
 */
typedef enum {
    BIST_INSN_GROUP_NONE = 0,
    BIST_INSN_GROUP_INTEGER,
    BIST_INSN_GROUP_SHIFT,
    BIST_INSN_GROUP_UPPERIMM,
    BIST_INSN_GROUP_CTRL,
    BIST_INSN_GROUP_LDST,
    BIST_INSN_GROUP_MUL,
    BIST_INSN_GROUP_CSR,
} bist_insn_group_t;

/**
 * @brief Run all Kconfig-enabled CPU instruction equivalence groups
 *
 * @return BIST_ESP_OK on success
 * @return BIST_ESP_CPU_INSTRUCTION_TEST_ERR if any vector fails
 */
bist_esp_err_t bist_cpu_instruction_test(void);

/**
 * @brief Vector ID of the last failure from bist_cpu_instruction_test()
 *
 * @return 0 if last run passed or has not failed; otherwise a
 *         per-group enum value (see bist_cpu_instruction_last_failure_group())
 */
uint32_t bist_cpu_instruction_last_failure_id(void);

/**
 * @brief Group of the last failure from bist_cpu_instruction_test()
 *
 * @return BIST_INSN_GROUP_NONE if last run passed or has not failed
 */
bist_insn_group_t bist_cpu_instruction_last_failure_group(void);

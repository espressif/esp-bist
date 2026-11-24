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
 * @file bist_cpu_instructions_integer.h
 * @brief Integer ALU instruction test vector IDs
 */

#pragma once

typedef enum {
    BIST_INSN_INTEGER_NONE = 0,
    BIST_INSN_ADD_ZEROS = 1,
    BIST_INSN_ADD_ONE_NEG1,
    BIST_INSN_ADD_UNEQUAL,
    BIST_INSN_ADD_MAX_ONE,
    BIST_INSN_ADD_MIN_MIN,
    BIST_INSN_ADD_CHECKER,
    BIST_INSN_ADDI_IMM_ZERO,
    BIST_INSN_ADDI_IMM_NEG1,
    BIST_INSN_ADDI_IMM_MIN,
    BIST_INSN_ADDI_IMM_MAX,
    BIST_INSN_ADDI_MAX_ONE,
    BIST_INSN_ADDI_MIN_NEG1,
    BIST_INSN_SUB_ZEROS,
    BIST_INSN_SUB_ONE_ONE,
    BIST_INSN_SUB_ZERO_ONE,
    BIST_INSN_SUB_MIN_ONE,
    BIST_INSN_SUB_MAX_NEG1,
    BIST_INSN_SUB_CHECKER,
    BIST_INSN_AND_ZERO,
    BIST_INSN_AND_ONES,
    BIST_INSN_AND_UNEQUAL,
    BIST_INSN_AND_CHECKER,
    BIST_INSN_AND_AA_ONES,
    BIST_INSN_AND_MAX_MIN,
    BIST_INSN_ANDI_IMM_ZERO,
    BIST_INSN_ANDI_IMM_NEG1,
    BIST_INSN_ANDI_IMM_MIN,
    BIST_INSN_ANDI_IMM_MAX,
    BIST_INSN_ANDI_AA_NEG1,
    BIST_INSN_ANDI_AA_ONE,
    BIST_INSN_OR_ZEROS,
    BIST_INSN_OR_ZERO_X,
    BIST_INSN_OR_UNEQUAL,
    BIST_INSN_OR_CHECKER,
    BIST_INSN_OR_AA_ZERO,
    BIST_INSN_OR_MAX_MIN,
    BIST_INSN_ORI_IMM_ZERO,
    BIST_INSN_ORI_IMM_NEG1,
    BIST_INSN_ORI_IMM_MIN,
    BIST_INSN_ORI_IMM_MAX,
    BIST_INSN_ORI_AA_ONE,
    BIST_INSN_ORI_55_NEG1,
    BIST_INSN_XOR_ZEROS,
    BIST_INSN_XOR_SAME,
    BIST_INSN_XOR_UNEQUAL,
    BIST_INSN_XOR_CHECKER,
    BIST_INSN_XOR_AA_ZERO,
    BIST_INSN_XOR_NEG1_MAX,
    BIST_INSN_XORI_IMM_ZERO,
    BIST_INSN_XORI_IMM_NEG1,
    BIST_INSN_XORI_NEG1_NEG1,
    BIST_INSN_XORI_IMM_MIN,
    BIST_INSN_XORI_IMM_MAX,
    BIST_INSN_XORI_AA_NEG1,
    BIST_INSN_SLT_LT,
    BIST_INSN_SLT_GT,
    BIST_INSN_SLT_EQ,
    BIST_INSN_SLT_NEG_POS,
    BIST_INSN_SLT_MIN_MAX,
    BIST_INSN_SLT_MAX_MIN,
    BIST_INSN_SLTI_LT,
    BIST_INSN_SLTI_GT,
    BIST_INSN_SLTI_NEG_ZERO,
    BIST_INSN_SLTI_IMM_MIN,
    BIST_INSN_SLTI_IMM_MAX,
    BIST_INSN_SLTI_IMM_NEG1,
    BIST_INSN_SLTI_MIN_NEG1,
    BIST_INSN_SLTU_LT,
    BIST_INSN_SLTU_GT,
    BIST_INSN_SLTU_EQ,
    BIST_INSN_SLTU_NEG_POS,
    BIST_INSN_SLTU_ZERO_NEG1,
    BIST_INSN_SLTU_MIN_MAX,
    BIST_INSN_SLTIU_LT,
    BIST_INSN_SLTIU_GT,
    BIST_INSN_SLTIU_NEG_ZERO,
    BIST_INSN_SLTIU_ZERO_NEG1,
    BIST_INSN_SLTIU_IMM_MIN,
    BIST_INSN_SLTIU_IMM_MAX,
    BIST_INSN_SLTIU_NEG1_MAX,
} bist_insn_integer_id_t;

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
 * @file bist_cpu_instructions_fp.h
 * @brief RV32F single-precision instruction test vector IDs
 *
 * Executed only on FPU targets (ESP32-H4, ESP32-P4).
 */

#pragma once

typedef enum {
    BIST_INSN_FP_NONE = 0,
    BIST_INSN_FP_FSW_FLW_ROUNDTRIP = 1,
    BIST_INSN_FP_FLW_NAN_PAYLOAD,
    BIST_INSN_FP_FADD_NORMAL,
    BIST_INSN_FP_FADD_INF_SUB_INF,
    BIST_INSN_FP_FADD_SUBNORMAL,
    BIST_INSN_FP_FSUB_NORMAL,
    BIST_INSN_FP_FSUB_NEG_ZEROS,
    BIST_INSN_FP_FMUL_NORMAL,
    BIST_INSN_FP_FMUL_SIGN_ZERO,
    BIST_INSN_FP_FDIV_NORMAL,
    BIST_INSN_FP_FDIV_NEG_BY_ZERO,
    BIST_INSN_FP_FSQRT_EXACT,
    BIST_INSN_FP_FSQRT_NEGATIVE,
    BIST_INSN_FP_FMADD_NORMAL,
    BIST_INSN_FP_FMADD_SINGLE_ROUNDING,
    BIST_INSN_FP_FMADD_INF_ZERO,
    BIST_INSN_FP_FMSUB_NORMAL,
    BIST_INSN_FP_FNMSUB_NORMAL,
    BIST_INSN_FP_FNMADD_NORMAL,
    BIST_INSN_FP_FSGNJ_TAKE_NEG,
    BIST_INSN_FP_FSGNJN_NEGATE,
    BIST_INSN_FP_FSGNJX_OPPOSITE_SIGNS,
    BIST_INSN_FP_FSGNJ_SAME_REG_NAN,
    BIST_INSN_FP_FSGNJX_SAME_REG_ABS,
    BIST_INSN_FP_FMIN_NORMAL,
    BIST_INSN_FP_FMIN_SIGNED_ZEROS,
    BIST_INSN_FP_FMIN_NAN_OPERAND,
    BIST_INSN_FP_FMAX_NORMAL,
    BIST_INSN_FP_FMAX_SIGNED_ZEROS,
    BIST_INSN_FP_FMAX_NAN_OPERAND,
    BIST_INSN_FP_FEQ_EQUAL,
    BIST_INSN_FP_FEQ_SIGNED_ZEROS,
    BIST_INSN_FP_FEQ_NAN,
    BIST_INSN_FP_FLT_INFINITIES,
    BIST_INSN_FP_FLT_SIGNED_ZEROS,
    BIST_INSN_FP_FLT_NAN,
    BIST_INSN_FP_FLE_SIGNED_ZEROS,
    BIST_INSN_FP_FLE_NAN,
    BIST_INSN_FP_FCVT_W_S_RTZ,
    BIST_INSN_FP_FCVT_W_S_RNE,
    BIST_INSN_FP_FCVT_W_S_SAT_POS,
    BIST_INSN_FP_FCVT_W_S_SAT_NEG,
    BIST_INSN_FP_FCVT_W_S_NAN,
    BIST_INSN_FP_FCVT_WU_S_NEGATIVE,
    BIST_INSN_FP_FCVT_WU_S_LARGE,
    BIST_INSN_FP_FCVT_S_W_NEG,
    BIST_INSN_FP_FCVT_S_W_INT_MIN,
    BIST_INSN_FP_FCVT_S_WU_MAX,
    BIST_INSN_FP_FMV_BIT_PATTERN,
    BIST_INSN_FP_FCLASS_NEG_INF,
    BIST_INSN_FP_FCLASS_NEG_NORMAL,
    BIST_INSN_FP_FCLASS_NEG_ZERO,
    BIST_INSN_FP_FCLASS_POS_ZERO,
    BIST_INSN_FP_FCLASS_POS_NORMAL,
    BIST_INSN_FP_FCLASS_QNAN,
    BIST_INSN_FP_FADD_RTZ,
    BIST_INSN_FP_FADD_RNE,
    BIST_INSN_FP_FFLAGS_DIVIDE_BY_ZERO,
    BIST_INSN_FP_FFLAGS_INVALID,
    BIST_INSN_FP_FFLAGS_INEXACT,
} bist_insn_fp_id_t;

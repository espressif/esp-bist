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
 * @file bist_cpu_instructions_mul.h
 * @brief RV32M multiply / divide instruction test vector IDs
 */

#pragma once

typedef enum {
    BIST_INSN_MUL_NONE = 0,
    BIST_INSN_MUL_ZEROS = 1,
    BIST_INSN_MUL_ONE_NEG1,
    BIST_INSN_MUL_UNEQUAL,
    BIST_INSN_MUL_MAX_TWO,
    BIST_INSN_MUL_MIN_MIN,
    BIST_INSN_MUL_CHECKER,
    BIST_INSN_MULH_ZEROS,
    BIST_INSN_MULH_ONE_NEG1,
    BIST_INSN_MULH_MAX_MAX,
    BIST_INSN_MULH_MIN_MIN,
    BIST_INSN_MULH_MIN_NEG1,
    BIST_INSN_MULH_NEG1_NEG1,
    BIST_INSN_MULHSU_ONE,
    BIST_INSN_MULHSU_NEG1_ONE,
    BIST_INSN_MULHSU_NEG1_MAXU,
    BIST_INSN_MULHSU_MIN_ONE,
    BIST_INSN_MULHSU_MAX_MAXU,
    BIST_INSN_MULHSU_MIN_MAXU,
    BIST_INSN_MULHU_ZERO,
    BIST_INSN_MULHU_ONE,
    BIST_INSN_MULHU_MAXU_MAXU,
    BIST_INSN_MULHU_HALF,
    BIST_INSN_MULHU_CHECKER,
    BIST_INSN_MULHU_MAXU_TWO,
    BIST_INSN_DIV_POS,
    BIST_INSN_DIV_NEG_POS,
    BIST_INSN_DIV_POS_NEG,
    BIST_INSN_DIV_NEG_NEG,
    BIST_INSN_DIV_OVERFLOW,
    BIST_INSN_DIV_BY_ZERO,
    BIST_INSN_DIVU_POS,
    BIST_INSN_DIVU_MAXU_ONE,
    BIST_INSN_DIVU_MAXU_MAXU,
    BIST_INSN_DIVU_HALF,
    BIST_INSN_DIVU_BY_ZERO,
    BIST_INSN_DIVU_SMALL,
    BIST_INSN_REM_POS,
    BIST_INSN_REM_NEG_POS,
    BIST_INSN_REM_POS_NEG,
    BIST_INSN_REM_NEG_NEG,
    BIST_INSN_REM_OVERFLOW,
    BIST_INSN_REM_BY_ZERO,
    BIST_INSN_REMU_POS,
    BIST_INSN_REMU_MAXU,
    BIST_INSN_REMU_HALF,
    BIST_INSN_REMU_ONE,
    BIST_INSN_REMU_BY_ZERO,
    BIST_INSN_REMU_SEVEN,
} bist_insn_mul_id_t;

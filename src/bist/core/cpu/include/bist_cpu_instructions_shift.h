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
 * @file bist_cpu_instructions_shift.h
 * @brief Shift instruction test vector IDs
 */

#pragma once

typedef enum {
    BIST_INSN_SHIFT_NONE = 0,
    BIST_INSN_SLL_ZERO = 1,
    BIST_INSN_SLL_COUNT0,
    BIST_INSN_SLL_COUNT1,
    BIST_INSN_SLL_COUNT31,
    BIST_INSN_SLL_CHECKER,
    BIST_INSN_SLL_AMT32,
    BIST_INSN_SLLI_ZERO,
    BIST_INSN_SLLI_COUNT0,
    BIST_INSN_SLLI_COUNT1,
    BIST_INSN_SLLI_COUNT31,
    BIST_INSN_SLLI_CHECKER,
    BIST_INSN_SLLI_MAX,
    BIST_INSN_SRL_COUNT0,
    BIST_INSN_SRL_COUNT1,
    BIST_INSN_SRL_COUNT31,
    BIST_INSN_SRL_CHECKER,
    BIST_INSN_SRL_ONE,
    BIST_INSN_SRL_AMT32,
    BIST_INSN_SRLI_COUNT0,
    BIST_INSN_SRLI_COUNT1,
    BIST_INSN_SRLI_COUNT31,
    BIST_INSN_SRLI_CHECKER,
    BIST_INSN_SRLI_ONES,
    BIST_INSN_SRLI_ONE_31,
    BIST_INSN_SRA_COUNT0,
    BIST_INSN_SRA_COUNT1,
    BIST_INSN_SRA_COUNT31,
    BIST_INSN_SRA_NEG1,
    BIST_INSN_SRA_MAX,
    BIST_INSN_SRA_AMT32,
    BIST_INSN_SRAI_COUNT0,
    BIST_INSN_SRAI_COUNT1,
    BIST_INSN_SRAI_COUNT31,
    BIST_INSN_SRAI_NEG1,
    BIST_INSN_SRAI_MAX,
    BIST_INSN_SRAI_CHECKER,
} bist_insn_shift_id_t;

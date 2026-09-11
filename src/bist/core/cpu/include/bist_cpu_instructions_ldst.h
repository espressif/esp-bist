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
 * @file bist_cpu_instructions_ldst.h
 * @brief Load / store instruction test vector IDs
 */

#pragma once

typedef enum {
    BIST_INSN_LDST_NONE = 0,
    BIST_INSN_SW_LW = 1,
    BIST_INSN_SH_LH,
    BIST_INSN_SH_LHU,
    BIST_INSN_SB_LB,
    BIST_INSN_SB_LBU,
    BIST_INSN_LW_POS_OFF,
    BIST_INSN_LW_NEG_OFF,
    BIST_INSN_LH_NEG_OFF,
    BIST_INSN_LB_HIGH_BYTE,
    BIST_INSN_LBU_HIGH_BYTE,
    BIST_INSN_LH_SIGN,
    BIST_INSN_LB_SIGN,
    BIST_INSN_SW_ZERO,
    BIST_INSN_SH_LOW,
    BIST_INSN_SB_MID,
} bist_insn_ldst_id_t;

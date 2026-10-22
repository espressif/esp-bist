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
 * @file bist_cpu_instructions_ctrl.h
 * @brief Branch / jump instruction test vector IDs
 */

#pragma once

typedef enum {
    BIST_INSN_CTRL_NONE = 0,
    BIST_INSN_BEQ_FWD_TAKEN = 1,
    BIST_INSN_BEQ_FWD_NOT,
    BIST_INSN_BEQ_BWD_TAKEN,
    BIST_INSN_BEQ_BWD_NOT,
    BIST_INSN_BNE_FWD_TAKEN,
    BIST_INSN_BNE_FWD_NOT,
    BIST_INSN_BNE_BWD_TAKEN,
    BIST_INSN_BNE_BWD_NOT,
    BIST_INSN_BLT_FWD_TAKEN,
    BIST_INSN_BLT_FWD_NOT,
    BIST_INSN_BLT_BWD_TAKEN,
    BIST_INSN_BLT_EQ_NOT,
    BIST_INSN_BGE_FWD_TAKEN,
    BIST_INSN_BGE_FWD_NOT,
    BIST_INSN_BGE_EQ_TAKEN,
    BIST_INSN_BGE_BWD_TAKEN,
    BIST_INSN_BLTU_FWD_TAKEN,
    BIST_INSN_BLTU_FWD_NOT,
    BIST_INSN_BLTU_NEG_POS_NOT,
    BIST_INSN_BLTU_BWD_TAKEN,
    BIST_INSN_BGEU_FWD_TAKEN,
    BIST_INSN_BGEU_FWD_NOT,
    BIST_INSN_BGEU_EQ_TAKEN,
    BIST_INSN_BGEU_BWD_TAKEN,
    BIST_INSN_JAL_FWD_LINK,
    BIST_INSN_JAL_BWD_LINK,
    BIST_INSN_JALR_LINK,
    BIST_INSN_JALR_OFFSET,
} bist_insn_ctrl_id_t;

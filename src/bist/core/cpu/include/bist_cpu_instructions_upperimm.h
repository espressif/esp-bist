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
 * @file bist_cpu_instructions_upperimm.h
 * @brief LUI / AUIPC instruction test vector IDs
 */

#pragma once

typedef enum {
    BIST_INSN_UPPERIMM_NONE = 0,
    BIST_INSN_LUI_ZERO = 1,
    BIST_INSN_LUI_ONE,
    BIST_INSN_LUI_MAX,
    BIST_INSN_LUI_MIN,
    BIST_INSN_LUI_FFFFF,
    BIST_INSN_LUI_AAAAA,
    BIST_INSN_AUIPC_ZERO,
    BIST_INSN_AUIPC_PLUS1,
    BIST_INSN_AUIPC_MINUS1,
    BIST_INSN_AUIPC_MAX,
    BIST_INSN_AUIPC_MIN,
} bist_insn_upperimm_id_t;

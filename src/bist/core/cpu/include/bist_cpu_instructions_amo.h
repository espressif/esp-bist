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
 * @file bist_cpu_instructions_amo.h
 * @brief RV32A (Zaamo / Zalrsc) instruction test vector IDs
 *
 * Not executed on ESP32-C3 (no A extension).
 */

#pragma once

typedef enum {
    BIST_INSN_AMO_NONE = 0,
    BIST_INSN_AMOADD_W_SMALL = 1,
    BIST_INSN_AMOADD_W_OVERFLOW,
    BIST_INSN_AMOSWAP_W,
    BIST_INSN_AMOAND_W,
    BIST_INSN_AMOOR_W,
    BIST_INSN_AMOXOR_W,
    BIST_INSN_AMOMIN_W,
    BIST_INSN_AMOMAX_W,
    BIST_INSN_AMOMINU_W,
    BIST_INSN_AMOMAXU_W,
    BIST_INSN_LRSC_W_SUCCESS,
    BIST_INSN_LRSC_W_FAIL,
    BIST_INSN_AMOSWAP_W_AQRL,
} bist_insn_amo_id_t;

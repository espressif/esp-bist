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
 * @file bist_cpu_instructions_csr.h
 * @brief Zicsr / Zifencei / FENCE instruction test vector IDs
 */

#pragma once

typedef enum {
    BIST_INSN_CSR_NONE = 0,
    BIST_INSN_CSRRW_ZERO = 1,
    BIST_INSN_CSRRW_ALLONES,
    BIST_INSN_CSRRW_AAAA,
    BIST_INSN_CSRRW_5555,
    BIST_INSN_CSRRS_SET,
    BIST_INSN_CSRRS_X0,
    BIST_INSN_CSRRC_CLEAR,
    BIST_INSN_CSRRC_X0,
    BIST_INSN_CSRRWI_UIMM0,
    BIST_INSN_CSRRWI_UIMM31,
    BIST_INSN_CSRRSI_SET,
    BIST_INSN_CSRRCI_CLEAR,
    BIST_INSN_CSRRW_RD0,
    BIST_INSN_FENCE,
    BIST_INSN_FENCE_I,
} bist_insn_csr_id_t;

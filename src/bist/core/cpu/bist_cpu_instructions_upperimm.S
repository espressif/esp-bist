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

/*
 * IEC 60730 component 1.2 — LUI and AUIPC equivalence-class vectors.
 */

    .text
    .align 2
    .global bist_cpu_instruction_upperimm
    .type   bist_cpu_instruction_upperimm, @function
    .extern bist_cpu_instruction_failure_id

.macro CHECK_LUI id, imm, exp
    lui     t2, \imm
    li      t3, \exp
    beq     t2, t3, 1f
    li      a1, \id
    j       bist_insn_upperimm_fail
1:
.endm

bist_cpu_instruction_upperimm:
    .option push
    .option norvc

    /* ---- LUI (1-6): 20-bit immediate in [31:12] ---- */
bist_insn_lui_zero:
    CHECK_LUI 1, 0, 0
bist_insn_lui_one:
    CHECK_LUI 2, 1, 0x00001000
bist_insn_lui_max:
    CHECK_LUI 3, 0x7FFFF, 0x7FFFF000
bist_insn_lui_min:
    CHECK_LUI 4, 0x80000, 0x80000000
bist_insn_lui_fffff:
    CHECK_LUI 5, 0xFFFFF, 0xFFFFF000
bist_insn_lui_aaaaa:
    CHECK_LUI 6, 0xAAAAA, 0xAAAAA000

    /* ---- AUIPC (7-11): rd = pc + (imm << 12) ---- */
    /* Link from jal is the instruction after jal (4-byte, norvc). */
bist_insn_auipc_zero:
    auipc   t2, 0
    jal     t3, 1f
    li      a1, 7
    j       bist_insn_upperimm_fail
1:
    addi    t3, t3, -8
    beq     t2, t3, bist_insn_auipc_plus1
    li      a1, 7
    j       bist_insn_upperimm_fail

bist_insn_auipc_plus1:
    auipc   t2, 1
    jal     t3, 1f
    li      a1, 8
    j       bist_insn_upperimm_fail
1:
    addi    t3, t3, -8
    lui     t0, 1
    add     t3, t3, t0
    beq     t2, t3, bist_insn_auipc_minus1
    li      a1, 8
    j       bist_insn_upperimm_fail

bist_insn_auipc_minus1:
    auipc   t2, 0xFFFFF
    jal     t3, 1f
    li      a1, 9
    j       bist_insn_upperimm_fail
1:
    addi    t3, t3, -8
    lui     t0, 0xFFFFF
    add     t3, t3, t0
    beq     t2, t3, bist_insn_auipc_max
    li      a1, 9
    j       bist_insn_upperimm_fail

bist_insn_auipc_max:
    auipc   t2, 0x7FFFF
    jal     t3, 1f
    li      a1, 10
    j       bist_insn_upperimm_fail
1:
    addi    t3, t3, -8
    lui     t0, 0x7FFFF
    add     t3, t3, t0
    beq     t2, t3, bist_insn_auipc_min
    li      a1, 10
    j       bist_insn_upperimm_fail

bist_insn_auipc_min:
    auipc   t2, 0x80000
    jal     t3, 1f
    li      a1, 11
    j       bist_insn_upperimm_fail
1:
    addi    t3, t3, -8
    lui     t0, 0x80000
    add     t3, t3, t0
    beq     t2, t3, bist_insn_upperimm_pass
    li      a1, 11
    j       bist_insn_upperimm_fail

bist_insn_upperimm_fail:
    la      t0, bist_cpu_instruction_failure_id
    sw      a1, 0(t0)
    li      a0, 14
    j       bist_insn_upperimm_done

bist_insn_upperimm_pass:
    la      t0, bist_cpu_instruction_failure_id
    sw      zero, 0(t0)
    li      a0, 0

bist_insn_upperimm_done:
    .option pop
    ret

    .size bist_cpu_instruction_upperimm, .-bist_cpu_instruction_upperimm

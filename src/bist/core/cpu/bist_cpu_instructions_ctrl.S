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
 * IEC 60730 component 1.2 — RV32I branch and jump vectors.
 * Each condition: taken/not-taken and a forward or backward path.
 */

    .text
    .align 2
    .global bist_cpu_instruction_ctrl
    .type   bist_cpu_instruction_ctrl, @function
    .extern bist_cpu_instruction_failure_id

.macro BR_FWD_TAKEN op, id, v1, v2
    li      t0, \v1
    li      t1, \v2
    \op     t0, t1, 1f
    li      a1, \id
    j       bist_insn_ctrl_fail
1:
.endm

.macro BR_FWD_NOT op, id, v1, v2
    li      t0, \v1
    li      t1, \v2
    \op     t0, t1, 1f
    j       2f
1:
    li      a1, \id
    j       bist_insn_ctrl_fail
2:
.endm

.macro BR_BWD_TAKEN op, id, v1, v2
    j       2f
1:
    j       3f
2:
    li      t0, \v1
    li      t1, \v2
    \op     t0, t1, 1b
    li      a1, \id
    j       bist_insn_ctrl_fail
3:
.endm

.macro BR_BWD_NOT op, id, v1, v2
    j       2f
1:
    li      a1, \id
    j       bist_insn_ctrl_fail
2:
    li      t0, \v1
    li      t1, \v2
    \op     t0, t1, 1b
.endm

bist_cpu_instruction_ctrl:
    .option push
    .option norvc

    /* ---- BEQ (1-4) ---- */
bist_insn_beq_fwd_taken:
    BR_FWD_TAKEN beq, 1, 1, 1
bist_insn_beq_fwd_not:
    BR_FWD_NOT beq, 2, 1, 2
bist_insn_beq_bwd_taken:
    BR_BWD_TAKEN beq, 3, -1, -1
bist_insn_beq_bwd_not:
    BR_BWD_NOT beq, 4, 0, 1

    /* ---- BNE (5-8) ---- */
bist_insn_bne_fwd_taken:
    BR_FWD_TAKEN bne, 5, 1, 2
bist_insn_bne_fwd_not:
    BR_FWD_NOT bne, 6, 5, 5
bist_insn_bne_bwd_taken:
    BR_BWD_TAKEN bne, 7, 0, 1
bist_insn_bne_bwd_not:
    BR_BWD_NOT bne, 8, -1, -1

    /* ---- BLT (9-12) ---- */
bist_insn_blt_fwd_taken:
    BR_FWD_TAKEN blt, 9, -1, 1
bist_insn_blt_fwd_not:
    BR_FWD_NOT blt, 10, 1, -1
bist_insn_blt_bwd_taken:
    BR_BWD_TAKEN blt, 11, 0x80000000, 0x7FFFFFFF
bist_insn_blt_eq_not:
    BR_FWD_NOT blt, 12, 3, 3

    /* ---- BGE (13-16) ---- */
bist_insn_bge_fwd_taken:
    BR_FWD_TAKEN bge, 13, 1, -1
bist_insn_bge_fwd_not:
    BR_FWD_NOT bge, 14, -1, 1
bist_insn_bge_eq_taken:
    BR_FWD_TAKEN bge, 15, 0, 0
bist_insn_bge_bwd_taken:
    BR_BWD_TAKEN bge, 16, 0x7FFFFFFF, 0x80000000

    /* ---- BLTU (17-20) ---- */
bist_insn_bltu_fwd_taken:
    BR_FWD_TAKEN bltu, 17, 1, 2
bist_insn_bltu_fwd_not:
    BR_FWD_NOT bltu, 18, 2, 1
bist_insn_bltu_neg_pos_not:
    BR_FWD_NOT bltu, 19, -1, 1
bist_insn_bltu_bwd_taken:
    BR_BWD_TAKEN bltu, 20, 0, -1

    /* ---- BGEU (21-24) ---- */
bist_insn_bgeu_fwd_taken:
    BR_FWD_TAKEN bgeu, 21, -1, 1
bist_insn_bgeu_fwd_not:
    BR_FWD_NOT bgeu, 22, 1, -1
bist_insn_bgeu_eq_taken:
    BR_FWD_TAKEN bgeu, 23, 8, 8
bist_insn_bgeu_bwd_taken:
    BR_BWD_TAKEN bgeu, 24, 2, 1

    /* ---- JAL (25-26): rd = pc+4, jump to target ---- */
bist_insn_jal_fwd_link:
    jal     t2, 1f
    li      a1, 25
    j       bist_insn_ctrl_fail
1:
    auipc   t3, 0
    addi    t3, t3, -8
    beq     t2, t3, bist_insn_jal_bwd_link
    li      a1, 25
    j       bist_insn_ctrl_fail

bist_insn_jal_bwd_link:
    j       2f
1:
    j       3f
2:
    jal     t2, 1b
3:
    auipc   t3, 0
    beq     t2, t3, bist_insn_jalr_link
    li      a1, 26
    j       bist_insn_ctrl_fail

    /* ---- JALR (27-28) ---- */
bist_insn_jalr_link:
    auipc   t0, 0
    addi    t0, t0, 20
    jalr    t2, t0, 0
    li      a1, 27
    j       bist_insn_ctrl_fail
    auipc   t3, 0
    addi    t3, t3, -8
    beq     t2, t3, bist_insn_jalr_offset
    li      a1, 27
    j       bist_insn_ctrl_fail

bist_insn_jalr_offset:
    auipc   t0, 0
    addi    t0, t0, 16
    jalr    t2, t0, 4
    li      a1, 28
    j       bist_insn_ctrl_fail
    auipc   t3, 0
    addi    t3, t3, -8
    beq     t2, t3, bist_insn_ctrl_pass
    li      a1, 28
    j       bist_insn_ctrl_fail

bist_insn_ctrl_fail:
    la      t0, bist_cpu_instruction_failure_id
    sw      a1, 0(t0)
    li      a0, 14
    j       bist_insn_ctrl_done

bist_insn_ctrl_pass:
    la      t0, bist_cpu_instruction_failure_id
    sw      zero, 0(t0)
    li      a0, 0

bist_insn_ctrl_done:
    .option pop
    ret

    .size bist_cpu_instruction_ctrl, .-bist_cpu_instruction_ctrl

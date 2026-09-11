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
 * IEC 60730 component 1.2 — RV32I load/store vectors.
 * Little-endian; misaligned accesses are not exercised (may trap).
 */

    .text
    .align 2
    .global bist_cpu_instruction_ldst
    .type   bist_cpu_instruction_ldst, @function
    .extern bist_cpu_instruction_failure_id

.macro CHECK_EQ id, exp
    li      t3, \exp
    beq     t2, t3, 1f
    li      a1, \id
    j       bist_insn_ldst_fail
1:
.endm

bist_cpu_instruction_ldst:
    .option push
    .option norvc

    addi    sp, sp, -32

    /* Word 0xAABBCCDD at 8(sp): bytes DD CC BB AA */
    li      t0, 0xAABBCCDD
    sw      t0, 8(sp)

bist_insn_sw_lw:
    lw      t2, 8(sp)
    CHECK_EQ 1, 0xAABBCCDD

bist_insn_sh_lh:
    lh      t2, 8(sp)
    CHECK_EQ 2, 0xFFFFCCDD

bist_insn_sh_lhu:
    lhu     t2, 8(sp)
    CHECK_EQ 3, 0x0000CCDD

bist_insn_sb_lb:
    lb      t2, 8(sp)
    CHECK_EQ 4, 0xFFFFFFDD

bist_insn_sb_lbu:
    lbu     t2, 8(sp)
    CHECK_EQ 5, 0x000000DD

bist_insn_lw_pos_off:
    lw      t2, 8(sp)
    CHECK_EQ 6, 0xAABBCCDD

bist_insn_lw_neg_off:
    addi    t1, sp, 16
    lw      t2, -8(t1)
    CHECK_EQ 7, 0xAABBCCDD

bist_insn_lh_neg_off:
    addi    t1, sp, 16
    lhu     t2, -6(t1)
    CHECK_EQ 8, 0x0000AABB

bist_insn_lb_high_byte:
    lb      t2, 11(sp)
    CHECK_EQ 9, 0xFFFFFFAA

bist_insn_lbu_high_byte:
    lbu     t2, 11(sp)
    CHECK_EQ 10, 0x000000AA

bist_insn_lh_sign:
    lh      t2, 10(sp)
    CHECK_EQ 11, 0xFFFFAABB

bist_insn_lb_sign:
    lb      t2, 10(sp)
    CHECK_EQ 12, 0xFFFFFFBB

bist_insn_sw_zero:
    sw      zero, 0(sp)
    lw      t2, 0(sp)
    CHECK_EQ 13, 0

bist_insn_sh_low:
    li      t0, 0x8001
    sh      t0, 0(sp)
    lh      t2, 0(sp)
    CHECK_EQ 14, 0xFFFF8001

bist_insn_sb_mid:
    li      t0, 0x7F
    sb      t0, 2(sp)
    lbu     t2, 2(sp)
    CHECK_EQ 15, 0x0000007F

    j       bist_insn_ldst_pass

bist_insn_ldst_fail:
    addi    sp, sp, 32
    la      t0, bist_cpu_instruction_failure_id
    sw      a1, 0(t0)
    li      a0, 14
    j       bist_insn_ldst_done

bist_insn_ldst_pass:
    addi    sp, sp, 32
    la      t0, bist_cpu_instruction_failure_id
    sw      zero, 0(t0)
    li      a0, 0

bist_insn_ldst_done:
    .option pop
    ret

    .size bist_cpu_instruction_ldst, .-bist_cpu_instruction_ldst

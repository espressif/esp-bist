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
 * IEC 60730 component 1.2 — RV32M multiply/divide equivalence-class vectors.
 * Fail fast on first mismatch; store failing vector ID in
 * bist_cpu_instruction_failure_id.
 *
 * Operand ideas adapted from riscv-tests (BSD-3-Clause); harness not imported.
 * DIV/REM follow the RISC-V privileged/unprivileged spec: divide-by-zero and
 * signed overflow (INT_MIN / -1) have defined results.
 */

    .text
    .align 2
    .global bist_cpu_instruction_mul
    .type   bist_cpu_instruction_mul, @function
    .extern bist_cpu_instruction_failure_id

.macro CHECK_RR op, id, v1, v2, exp
    li      t0, \v1
    li      t1, \v2
    \op     t2, t0, t1
    li      t3, \exp
    beq     t2, t3, 1f
    li      a1, \id
    j       bist_insn_mul_fail
1:
.endm

bist_cpu_instruction_mul:
    .option push
    .option norvc

    /* ---- MUL (1-6): low 32 bits of product ---- */
bist_insn_mul_zeros:
    CHECK_RR mul, 1, 0, 0, 0
bist_insn_mul_one_neg1:
    CHECK_RR mul, 2, 1, -1, -1
bist_insn_mul_unequal:
    CHECK_RR mul, 3, 5, 7, 35
bist_insn_mul_max_two:
    CHECK_RR mul, 4, 0x7FFFFFFF, 2, 0xFFFFFFFE
bist_insn_mul_min_min:
    CHECK_RR mul, 5, 0x80000000, 0x80000000, 0
bist_insn_mul_checker:
    CHECK_RR mul, 6, 0xAAAAAAAA, 0x55555555, 0x71C71C72

    /* ---- MULH (7-12): signed x signed, high 32 ---- */
bist_insn_mulh_zeros:
    CHECK_RR mulh, 7, 0, 0, 0
bist_insn_mulh_one_neg1:
    CHECK_RR mulh, 8, 1, -1, -1
bist_insn_mulh_max_max:
    CHECK_RR mulh, 9, 0x7FFFFFFF, 0x7FFFFFFF, 0x3FFFFFFF
bist_insn_mulh_min_min:
    CHECK_RR mulh, 10, 0x80000000, 0x80000000, 0x40000000
bist_insn_mulh_min_neg1:
    CHECK_RR mulh, 11, 0x80000000, -1, 0
bist_insn_mulh_neg1_neg1:
    CHECK_RR mulh, 12, -1, -1, 0

    /* ---- MULHSU (13-18): signed x unsigned, high 32 ---- */
bist_insn_mulhsu_one:
    CHECK_RR mulhsu, 13, 1, 1, 0
bist_insn_mulhsu_neg1_one:
    CHECK_RR mulhsu, 14, -1, 1, -1
bist_insn_mulhsu_neg1_maxu:
    CHECK_RR mulhsu, 15, -1, 0xFFFFFFFF, -1
bist_insn_mulhsu_min_one:
    CHECK_RR mulhsu, 16, 0x80000000, 1, -1
bist_insn_mulhsu_max_maxu:
    CHECK_RR mulhsu, 17, 0x7FFFFFFF, 0xFFFFFFFF, 0x7FFFFFFE
bist_insn_mulhsu_min_maxu:
    CHECK_RR mulhsu, 18, 0x80000000, 0xFFFFFFFF, 0x80000000

    /* ---- MULHU (19-24): unsigned x unsigned, high 32 ---- */
bist_insn_mulhu_zero:
    CHECK_RR mulhu, 19, 0, 0xFFFFFFFF, 0
bist_insn_mulhu_one:
    CHECK_RR mulhu, 20, 1, 1, 0
bist_insn_mulhu_maxu_maxu:
    CHECK_RR mulhu, 21, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFE
bist_insn_mulhu_half:
    CHECK_RR mulhu, 22, 0x80000000, 0x80000000, 0x40000000
bist_insn_mulhu_checker:
    CHECK_RR mulhu, 23, 0xAAAAAAAA, 0x55555555, 0x38E38E38
bist_insn_mulhu_maxu_two:
    CHECK_RR mulhu, 24, 0xFFFFFFFF, 2, 1

    /* ---- DIV (25-30): signed divide toward zero ---- */
bist_insn_div_pos:
    CHECK_RR div, 25, 10, 2, 5
bist_insn_div_neg_pos:
    CHECK_RR div, 26, -10, 2, -5
bist_insn_div_pos_neg:
    CHECK_RR div, 27, 10, -2, -5
bist_insn_div_neg_neg:
    CHECK_RR div, 28, -10, -2, 5
bist_insn_div_overflow:
    CHECK_RR div, 29, 0x80000000, -1, 0x80000000
bist_insn_div_by_zero:
    CHECK_RR div, 30, 5, 0, -1

    /* ---- DIVU (31-36): unsigned divide ---- */
bist_insn_divu_pos:
    CHECK_RR divu, 31, 10, 2, 5
bist_insn_divu_maxu_one:
    CHECK_RR divu, 32, 0xFFFFFFFF, 1, 0xFFFFFFFF
bist_insn_divu_maxu_maxu:
    CHECK_RR divu, 33, 0xFFFFFFFF, 0xFFFFFFFF, 1
bist_insn_divu_half:
    CHECK_RR divu, 34, 0x80000000, 2, 0x40000000
bist_insn_divu_by_zero:
    CHECK_RR divu, 35, 5, 0, 0xFFFFFFFF
bist_insn_divu_small:
    CHECK_RR divu, 36, 1, 2, 0

    /* ---- REM (37-42): signed remainder, same sign as dividend ---- */
bist_insn_rem_pos:
    CHECK_RR rem, 37, 10, 3, 1
bist_insn_rem_neg_pos:
    CHECK_RR rem, 38, -10, 3, -1
bist_insn_rem_pos_neg:
    CHECK_RR rem, 39, 10, -3, 1
bist_insn_rem_neg_neg:
    CHECK_RR rem, 40, -10, -3, -1
bist_insn_rem_overflow:
    CHECK_RR rem, 41, 0x80000000, -1, 0
bist_insn_rem_by_zero:
    CHECK_RR rem, 42, 5, 0, 5

    /* ---- REMU (43-48): unsigned remainder ---- */
bist_insn_remu_pos:
    CHECK_RR remu, 43, 10, 3, 1
bist_insn_remu_maxu:
    CHECK_RR remu, 44, 0xFFFFFFFF, 2, 1
bist_insn_remu_half:
    CHECK_RR remu, 45, 0x80000000, 2, 0
bist_insn_remu_one:
    CHECK_RR remu, 46, 5, 1, 0
bist_insn_remu_by_zero:
    CHECK_RR remu, 47, 5, 0, 5
bist_insn_remu_seven:
    CHECK_RR remu, 48, 7, 3, 1

    j       bist_insn_mul_pass

bist_insn_mul_fail:
    la      t0, bist_cpu_instruction_failure_id
    sw      a1, 0(t0)
    li      a0, 14          /* BIST_ESP_CPU_INSTRUCTION_TEST_ERR */
    j       bist_insn_mul_done

bist_insn_mul_pass:
    la      t0, bist_cpu_instruction_failure_id
    sw      zero, 0(t0)
    li      a0, 0           /* BIST_ESP_OK */

bist_insn_mul_done:
    .option pop
    ret

    .size bist_cpu_instruction_mul, .-bist_cpu_instruction_mul

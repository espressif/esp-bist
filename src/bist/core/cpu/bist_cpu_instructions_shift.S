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
 * IEC 60730 component 1.2 — RV32I shift equivalence-class vectors.
 * Fail fast on first mismatch; store failing vector ID in
 * bist_cpu_instruction_failure_id.
 *
 * Operand ideas adapted from riscv-tests (BSD-3-Clause); harness not imported.
 * Register-shift amounts use rs2[4:0]; 32 is included to check that mask.
 */

    .text
    .align 2
    .global bist_cpu_instruction_shift
    .type   bist_cpu_instruction_shift, @function
    .extern bist_cpu_instruction_failure_id

.macro CHECK_RR op, id, v1, v2, exp
    li      t0, \v1
    li      t1, \v2
    \op     t2, t0, t1
    li      t3, \exp
    beq     t2, t3, 1f
    li      a1, \id
    j       bist_insn_shift_fail
1:
.endm

.macro CHECK_RI op, id, v1, imm, exp
    li      t0, \v1
    \op     t2, t0, \imm
    li      t3, \exp
    beq     t2, t3, 1f
    li      a1, \id
    j       bist_insn_shift_fail
1:
.endm

bist_cpu_instruction_shift:
    .option push
    .option norvc

    /* ---- SLL (1-6) ---- */
bist_insn_sll_zero:
    CHECK_RR sll, 1, 0, 0, 0
bist_insn_sll_count0:
    CHECK_RR sll, 2, 1, 0, 1
bist_insn_sll_count1:
    CHECK_RR sll, 3, 1, 1, 2
bist_insn_sll_count31:
    CHECK_RR sll, 4, 1, 31, 0x80000000
bist_insn_sll_checker:
    CHECK_RR sll, 5, 0xAAAAAAAA, 1, 0x55555554
bist_insn_sll_amt32:
    CHECK_RR sll, 6, 1, 32, 1

    /* ---- SLLI (7-12) ---- */
bist_insn_slli_zero:
    CHECK_RI slli, 7, 0, 0, 0
bist_insn_slli_count0:
    CHECK_RI slli, 8, 1, 0, 1
bist_insn_slli_count1:
    CHECK_RI slli, 9, 1, 1, 2
bist_insn_slli_count31:
    CHECK_RI slli, 10, 1, 31, 0x80000000
bist_insn_slli_checker:
    CHECK_RI slli, 11, 0xAAAAAAAA, 1, 0x55555554
bist_insn_slli_max:
    CHECK_RI slli, 12, 0x7FFFFFFF, 1, 0xFFFFFFFE

    /* ---- SRL (13-18) ---- */
bist_insn_srl_count0:
    CHECK_RR srl, 13, 0x80000000, 0, 0x80000000
bist_insn_srl_count1:
    CHECK_RR srl, 14, 0x80000000, 1, 0x40000000
bist_insn_srl_count31:
    CHECK_RR srl, 15, 0x80000000, 31, 1
bist_insn_srl_checker:
    CHECK_RR srl, 16, 0xAAAAAAAA, 1, 0x55555555
bist_insn_srl_one:
    CHECK_RR srl, 17, 1, 1, 0
bist_insn_srl_amt32:
    CHECK_RR srl, 18, 0xFFFFFFFF, 32, 0xFFFFFFFF

    /* ---- SRLI (19-24) ---- */
bist_insn_srli_count0:
    CHECK_RI srli, 19, 0x80000000, 0, 0x80000000
bist_insn_srli_count1:
    CHECK_RI srli, 20, 0x80000000, 1, 0x40000000
bist_insn_srli_count31:
    CHECK_RI srli, 21, 0x80000000, 31, 1
bist_insn_srli_checker:
    CHECK_RI srli, 22, 0xAAAAAAAA, 1, 0x55555555
bist_insn_srli_ones:
    CHECK_RI srli, 23, 0xFFFFFFFF, 1, 0x7FFFFFFF
bist_insn_srli_one_31:
    CHECK_RI srli, 24, 1, 31, 0

    /* ---- SRA (25-30) ---- */
bist_insn_sra_count0:
    CHECK_RR sra, 25, 0x80000000, 0, 0x80000000
bist_insn_sra_count1:
    CHECK_RR sra, 26, 0x80000000, 1, 0xC0000000
bist_insn_sra_count31:
    CHECK_RR sra, 27, 0x80000000, 31, 0xFFFFFFFF
bist_insn_sra_neg1:
    CHECK_RR sra, 28, -1, 1, 0xFFFFFFFF
bist_insn_sra_max:
    CHECK_RR sra, 29, 0x7FFFFFFF, 1, 0x3FFFFFFF
bist_insn_sra_amt32:
    CHECK_RR sra, 30, 0x80000000, 32, 0x80000000

    /* ---- SRAI (31-36) ---- */
bist_insn_srai_count0:
    CHECK_RI srai, 31, 0x80000000, 0, 0x80000000
bist_insn_srai_count1:
    CHECK_RI srai, 32, 0x80000000, 1, 0xC0000000
bist_insn_srai_count31:
    CHECK_RI srai, 33, 0x80000000, 31, 0xFFFFFFFF
bist_insn_srai_neg1:
    CHECK_RI srai, 34, -1, 1, 0xFFFFFFFF
bist_insn_srai_max:
    CHECK_RI srai, 35, 0x7FFFFFFF, 1, 0x3FFFFFFF
bist_insn_srai_checker:
    CHECK_RI srai, 36, 0xAAAAAAAA, 1, 0xD5555555

    j       bist_insn_shift_pass

bist_insn_shift_fail:
    la      t0, bist_cpu_instruction_failure_id
    sw      a1, 0(t0)
    li      a0, 14          /* BIST_ESP_CPU_INSTRUCTION_TEST_ERR */
    j       bist_insn_shift_done

bist_insn_shift_pass:
    la      t0, bist_cpu_instruction_failure_id
    sw      zero, 0(t0)
    li      a0, 0           /* BIST_ESP_OK */

bist_insn_shift_done:
    .option pop
    ret

    .size bist_cpu_instruction_shift, .-bist_cpu_instruction_shift

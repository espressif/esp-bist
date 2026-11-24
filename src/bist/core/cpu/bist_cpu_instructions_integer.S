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
 * IEC 60730 component 1.2 — RV32I integer ALU equivalence-class vectors.
 * Fail fast on first mismatch; store failing vector ID in
 * bist_cpu_instruction_failure_id.
 *
 * Operand ideas adapted from riscv-tests (BSD-3-Clause); harness not imported.
 */

    .text
    .align 2
    .global bist_cpu_instruction_integer
    .type   bist_cpu_instruction_integer, @function
    .extern bist_cpu_instruction_failure_id

/* CHECK_RR: register-register op; id is numeric vector id */
.macro CHECK_RR op, id, v1, v2, exp
    li      t0, \v1
    li      t1, \v2
    \op     t2, t0, t1
    li      t3, \exp
    beq     t2, t3, 1f
    li      a1, \id
    j       bist_insn_fail
1:
.endm

/* CHECK_RI: register-immediate op */
.macro CHECK_RI op, id, v1, imm, exp
    li      t0, \v1
    \op     t2, t0, \imm
    li      t3, \exp
    beq     t2, t3, 1f
    li      a1, \id
    j       bist_insn_fail
1:
.endm

bist_cpu_instruction_integer:
    .option push
    .option norvc

    /* ---- ADD (1-6) ---- */
bist_insn_add_zeros:
    CHECK_RR add, 1, 0, 0, 0
bist_insn_add_one_neg1:
    CHECK_RR add, 2, 1, -1, 0
bist_insn_add_unequal:
    CHECK_RR add, 3, 5, 7, 12
bist_insn_add_max_one:
    CHECK_RR add, 4, 0x7FFFFFFF, 1, 0x80000000
bist_insn_add_min_min:
    CHECK_RR add, 5, 0x80000000, 0x80000000, 0
bist_insn_add_checker:
    CHECK_RR add, 6, 0xAAAAAAAA, 0x55555555, 0xFFFFFFFF

    /* ---- ADDI (7-12) ---- */
bist_insn_addi_imm_zero:
    CHECK_RI addi, 7, 0, 0, 0
bist_insn_addi_imm_neg1:
    CHECK_RI addi, 8, 1, -1, 0
bist_insn_addi_imm_min:
    CHECK_RI addi, 9, 0, -2048, 0xFFFFF800
bist_insn_addi_imm_max:
    CHECK_RI addi, 10, 0, 2047, 0x000007FF
bist_insn_addi_max_one:
    CHECK_RI addi, 11, 0x7FFFFFFF, 1, 0x80000000
bist_insn_addi_min_neg1:
    CHECK_RI addi, 12, 0x80000000, -1, 0x7FFFFFFF

    /* ---- SUB (13-18) ---- */
bist_insn_sub_zeros:
    CHECK_RR sub, 13, 0, 0, 0
bist_insn_sub_one_one:
    CHECK_RR sub, 14, 1, 1, 0
bist_insn_sub_zero_one:
    CHECK_RR sub, 15, 0, 1, 0xFFFFFFFF
bist_insn_sub_min_one:
    CHECK_RR sub, 16, 0x80000000, 1, 0x7FFFFFFF
bist_insn_sub_max_neg1:
    CHECK_RR sub, 17, 0x7FFFFFFF, -1, 0x80000000
bist_insn_sub_checker:
    CHECK_RR sub, 18, 0xAAAAAAAA, 0x55555555, 0x55555555

    /* ---- AND (19-24) ---- */
bist_insn_and_zero:
    CHECK_RR and, 19, 0, 0xFFFFFFFF, 0
bist_insn_and_ones:
    CHECK_RR and, 20, 0xFFFFFFFF, 0x12345678, 0x12345678
bist_insn_and_unequal:
    CHECK_RR and, 21, 0xF0F0F0F0, 0x0F0F0F0F, 0
bist_insn_and_checker:
    CHECK_RR and, 22, 0xAAAAAAAA, 0x55555555, 0
bist_insn_and_aa_ones:
    CHECK_RR and, 23, 0xAAAAAAAA, 0xFFFFFFFF, 0xAAAAAAAA
bist_insn_and_max_min:
    CHECK_RR and, 24, 0x7FFFFFFF, 0x80000000, 0

    /* ---- ANDI (25-30) ---- */
bist_insn_andi_imm_zero:
    CHECK_RI andi, 25, 0xFFFFFFFF, 0, 0
bist_insn_andi_imm_neg1:
    CHECK_RI andi, 26, 0xFFFFFFFF, -1, 0xFFFFFFFF
bist_insn_andi_imm_min:
    CHECK_RI andi, 27, 0xFFFFFFFF, -2048, 0xFFFFF800
bist_insn_andi_imm_max:
    CHECK_RI andi, 28, 0xFFFFFFFF, 2047, 0x000007FF
bist_insn_andi_aa_neg1:
    CHECK_RI andi, 29, 0xAAAAAAAA, -1, 0xAAAAAAAA
bist_insn_andi_aa_one:
    CHECK_RI andi, 30, 0xAAAAAAAA, 1, 0

    /* ---- OR (31-36) ---- */
bist_insn_or_zeros:
    CHECK_RR or, 31, 0, 0, 0
bist_insn_or_zero_x:
    CHECK_RR or, 32, 0, 0x12345678, 0x12345678
bist_insn_or_unequal:
    CHECK_RR or, 33, 1, 2, 3
bist_insn_or_checker:
    CHECK_RR or, 34, 0xAAAAAAAA, 0x55555555, 0xFFFFFFFF
bist_insn_or_aa_zero:
    CHECK_RR or, 35, 0xAAAAAAAA, 0, 0xAAAAAAAA
bist_insn_or_max_min:
    CHECK_RR or, 36, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF

    /* ---- ORI (37-42) ---- */
bist_insn_ori_imm_zero:
    CHECK_RI ori, 37, 0, 0, 0
bist_insn_ori_imm_neg1:
    CHECK_RI ori, 38, 0, -1, 0xFFFFFFFF
bist_insn_ori_imm_min:
    CHECK_RI ori, 39, 0, -2048, 0xFFFFF800
bist_insn_ori_imm_max:
    CHECK_RI ori, 40, 0, 2047, 0x000007FF
bist_insn_ori_aa_one:
    CHECK_RI ori, 41, 0xAAAAAAAA, 1, 0xAAAAAAAB
bist_insn_ori_55_neg1:
    CHECK_RI ori, 42, 0x55555555, -1, 0xFFFFFFFF

    /* ---- XOR (43-48) ---- */
bist_insn_xor_zeros:
    CHECK_RR xor, 43, 0, 0, 0
bist_insn_xor_same:
    CHECK_RR xor, 44, 0x12345678, 0x12345678, 0
bist_insn_xor_unequal:
    CHECK_RR xor, 45, 1, 2, 3
bist_insn_xor_checker:
    CHECK_RR xor, 46, 0xAAAAAAAA, 0x55555555, 0xFFFFFFFF
bist_insn_xor_aa_zero:
    CHECK_RR xor, 47, 0xAAAAAAAA, 0, 0xAAAAAAAA
bist_insn_xor_neg1_max:
    CHECK_RR xor, 48, 0xFFFFFFFF, 0x7FFFFFFF, 0x80000000

    /* ---- XORI (49-54) ---- */
bist_insn_xori_imm_zero:
    CHECK_RI xori, 49, 0, 0, 0
bist_insn_xori_imm_neg1:
    CHECK_RI xori, 50, 0, -1, 0xFFFFFFFF
bist_insn_xori_neg1_neg1:
    CHECK_RI xori, 51, 0xFFFFFFFF, -1, 0
bist_insn_xori_imm_min:
    CHECK_RI xori, 52, 0, -2048, 0xFFFFF800
bist_insn_xori_imm_max:
    CHECK_RI xori, 53, 0, 2047, 0x000007FF
bist_insn_xori_aa_neg1:
    CHECK_RI xori, 54, 0xAAAAAAAA, -1, 0x55555555

    /* ---- SLT (55-60) ---- */
bist_insn_slt_lt:
    CHECK_RR slt, 55, 1, 2, 1
bist_insn_slt_gt:
    CHECK_RR slt, 56, 2, 1, 0
bist_insn_slt_eq:
    CHECK_RR slt, 57, -1, -1, 0
bist_insn_slt_neg_pos:
    CHECK_RR slt, 58, -1, 1, 1
bist_insn_slt_min_max:
    CHECK_RR slt, 59, 0x80000000, 0x7FFFFFFF, 1
bist_insn_slt_max_min:
    CHECK_RR slt, 60, 0x7FFFFFFF, 0x80000000, 0

    /* ---- SLTI (61-67) ---- */
bist_insn_slti_lt:
    CHECK_RI slti, 61, 1, 2, 1
bist_insn_slti_gt:
    CHECK_RI slti, 62, 2, 1, 0
bist_insn_slti_neg_zero:
    CHECK_RI slti, 63, -1, 0, 1
bist_insn_slti_imm_min:
    CHECK_RI slti, 64, 0, -2048, 0
bist_insn_slti_imm_max:
    CHECK_RI slti, 65, 0, 2047, 1
bist_insn_slti_imm_neg1:
    CHECK_RI slti, 66, 0, -1, 0
bist_insn_slti_min_neg1:
    CHECK_RI slti, 67, 0x80000000, -1, 1

    /* ---- SLTU (68-73) ---- */
bist_insn_sltu_lt:
    CHECK_RR sltu, 68, 1, 2, 1
bist_insn_sltu_gt:
    CHECK_RR sltu, 69, 2, 1, 0
bist_insn_sltu_eq:
    CHECK_RR sltu, 70, 5, 5, 0
bist_insn_sltu_neg_pos:
    CHECK_RR sltu, 71, -1, 1, 0
bist_insn_sltu_zero_neg1:
    CHECK_RR sltu, 72, 0, -1, 1
bist_insn_sltu_min_max:
    CHECK_RR sltu, 73, 0x80000000, 0x7FFFFFFF, 0

    /* ---- SLTIU (74-80) ---- */
bist_insn_sltiu_lt:
    CHECK_RI sltiu, 74, 1, 2, 1
bist_insn_sltiu_gt:
    CHECK_RI sltiu, 75, 2, 1, 0
bist_insn_sltiu_neg_zero:
    CHECK_RI sltiu, 76, -1, 0, 0
bist_insn_sltiu_zero_neg1:
    CHECK_RI sltiu, 77, 0, -1, 1
bist_insn_sltiu_imm_min:
    CHECK_RI sltiu, 78, 0, -2048, 1
bist_insn_sltiu_imm_max:
    CHECK_RI sltiu, 79, 0, 2047, 1
bist_insn_sltiu_neg1_max:
    CHECK_RI sltiu, 80, -1, 2047, 0

    j       bist_insn_pass

bist_insn_fail:
    la      t0, bist_cpu_instruction_failure_id
    sw      a1, 0(t0)
    li      a0, 14          /* BIST_ESP_CPU_INSTRUCTION_TEST_ERR */
    j       bist_insn_done

bist_insn_pass:
    la      t0, bist_cpu_instruction_failure_id
    sw      zero, 0(t0)
    li      a0, 0           /* BIST_ESP_OK */

bist_insn_done:
    .option pop
    ret

    .size bist_cpu_instruction_integer, .-bist_cpu_instruction_integer

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
 * IEC 60730 component 1.2 — Zicsr / Zifencei / FENCE decode-execute vectors.
 * Target CSR is mscratch only (save/restore on pass and fail).
 * Fail fast on first mismatch; store failing vector ID in
 * bist_cpu_instruction_failure_id.
 */

    .text
    .align 2
    .global bist_cpu_instruction_csr
    .type   bist_cpu_instruction_csr, @function
    .extern bist_cpu_instruction_failure_id

.macro CHECK_EQ id, reg, exp
    li      t3, \exp
    beq     \reg, t3, 1f
    li      a1, \id
    j       bist_insn_csr_fail
1:
.endm

.macro CHECK_RD_CSR id, rd_exp, csr_exp
    li      t3, \rd_exp
    bne     t2, t3, 2f
    csrrs   t4, mscratch, x0
    li      t3, \csr_exp
    beq     t4, t3, 1f
2:
    li      a1, \id
    j       bist_insn_csr_fail
1:
.endm

bist_cpu_instruction_csr:
    .option push
    .option norvc

    addi    sp, sp, -16
    csrrs   t6, mscratch, x0
    sw      t6, 0(sp)

    /* ---- CSRRW (1-4): write rs1, rd = old ---- */
bist_insn_csrrw_zero:
    li      t0, 0x12345678
    csrrw   x0, mscratch, t0
    li      t0, 0
    csrrw   t2, mscratch, t0
    csrrs   t2, mscratch, x0
    CHECK_EQ 1, t2, 0

bist_insn_csrrw_allones:
    li      t0, 0
    csrrw   x0, mscratch, t0
    li      t0, 0xFFFFFFFF
    csrrw   t2, mscratch, t0
    CHECK_RD_CSR 2, 0, 0xFFFFFFFF

bist_insn_csrrw_aaaa:
    li      t0, 0xFFFFFFFF
    csrrw   x0, mscratch, t0
    li      t0, 0xAAAAAAAA
    csrrw   t2, mscratch, t0
    CHECK_RD_CSR 3, 0xFFFFFFFF, 0xAAAAAAAA

bist_insn_csrrw_5555:
    li      t0, 0xAAAAAAAA
    csrrw   x0, mscratch, t0
    li      t0, 0x55555555
    csrrw   t2, mscratch, t0
    CHECK_RD_CSR 4, 0xAAAAAAAA, 0x55555555

    /* ---- CSRRS (5-6): csr |= rs1; rs1=x0 is read-only ---- */
bist_insn_csrrs_set:
    li      t0, 0x55555555
    csrrw   x0, mscratch, t0
    li      t0, 0xAAAAAAAA
    csrrs   t2, mscratch, t0
    CHECK_RD_CSR 5, 0x55555555, 0xFFFFFFFF

bist_insn_csrrs_x0:
    li      t0, 0xAAAAAAAA
    csrrw   x0, mscratch, t0
    csrrs   t2, mscratch, x0
    CHECK_RD_CSR 6, 0xAAAAAAAA, 0xAAAAAAAA

    /* ---- CSRRC (7-8): csr &= ~rs1; rs1=x0 is read-only ---- */
bist_insn_csrrc_clear:
    li      t0, 0xFFFFFFFF
    csrrw   x0, mscratch, t0
    li      t0, 0xAAAAAAAA
    csrrc   t2, mscratch, t0
    CHECK_RD_CSR 7, 0xFFFFFFFF, 0x55555555

bist_insn_csrrc_x0:
    li      t0, 0x55555555
    csrrw   x0, mscratch, t0
    csrrc   t2, mscratch, x0
    CHECK_RD_CSR 8, 0x55555555, 0x55555555

    /* ---- Immediate forms: 5-bit uimm, zero-extended ---- */
bist_insn_csrrwi_uimm0:
    li      t0, 0xAAAAAAAA
    csrrw   x0, mscratch, t0
    csrrwi  t2, mscratch, 0
    CHECK_RD_CSR 9, 0xAAAAAAAA, 0

bist_insn_csrrwi_uimm31:
    li      t0, 0
    csrrw   x0, mscratch, t0
    csrrwi  t2, mscratch, 31
    CHECK_RD_CSR 10, 0, 0x1F

bist_insn_csrrsi_set:
    li      t0, 0x10
    csrrw   x0, mscratch, t0
    csrrsi  t2, mscratch, 15
    CHECK_RD_CSR 11, 0x10, 0x1F

bist_insn_csrrci_clear:
    li      t0, 0x1F
    csrrw   x0, mscratch, t0
    csrrci  t2, mscratch, 15
    CHECK_RD_CSR 12, 0x1F, 0x10

    /* ---- CSRRW rd=x0 still writes the CSR ---- */
bist_insn_csrrw_rd0:
    li      t0, 0xA5A5A5A5
    csrrw   x0, mscratch, t0
    csrrs   t2, mscratch, x0
    CHECK_EQ 13, t2, 0xA5A5A5A5

    /* ---- FENCE / FENCE.I: decode and continue ---- */
bist_insn_fence:
    fence   iorw, iorw
    li      t0, 40
    li      t1, 2
    add     t2, t0, t1
    CHECK_EQ 14, t2, 42

bist_insn_fence_i:
    fence.i
    li      t0, 1
    li      t1, 2
    add     t2, t0, t1
    CHECK_EQ 15, t2, 3

    j       bist_insn_csr_pass

bist_insn_csr_fail:
    lw      t6, 0(sp)
    csrrw   x0, mscratch, t6
    addi    sp, sp, 16
    la      t0, bist_cpu_instruction_failure_id
    sw      a1, 0(t0)
    li      a0, 14          /* BIST_ESP_CPU_INSTRUCTION_TEST_ERR */
    j       bist_insn_csr_done

bist_insn_csr_pass:
    lw      t6, 0(sp)
    csrrw   x0, mscratch, t6
    addi    sp, sp, 16
    la      t0, bist_cpu_instruction_failure_id
    sw      zero, 0(t0)
    li      a0, 0           /* BIST_ESP_OK */

bist_insn_csr_done:
    .option pop
    ret

    .size bist_cpu_instruction_csr, .-bist_cpu_instruction_csr

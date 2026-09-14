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
 * IEC 60730 component 1.2 — RV32A (Zaamo + Zalrsc) word atomic vectors.
 * Encoded with aq=0, rl=0 except one compact amoswap.w.aqrl check.
 * Fail fast on first mismatch; store failing vector ID in
 * bist_cpu_instruction_failure_id.
 *
 * A is enabled in this file only (.option arch, +a), not via global -march.
 * Do not build or call this group on ESP32-C3 (no A extension).
 *
 * LR.W/SC.W fail vector uses a same-hart SW to the reservation set. Some
 * cores may still complete SC after that store; the sequence is still
 * encoded so both lr.w and sc.w appear. If SC never fails, this vector
 * reports a mismatch (rd==0 or memory is the SC data).
 */

    .text
    .option push
    .option norvc
    .option arch, +a
    .align 2
    .global bist_cpu_instruction_amo
    .type   bist_cpu_instruction_amo, @function
    .extern bist_cpu_instruction_failure_id

/* rd = old memory; memory := op(old, rs2). Check both. Address is 0(sp). */
.macro CHECK_AMO op, id, mem_init, rs2, rd_exp, mem_exp
    li      t0, \mem_init
    sw      t0, 0(sp)
    li      t1, \rs2
    \op     t2, t1, (sp)
    li      t3, \rd_exp
    bne     t2, t3, 2f
    lw      t2, 0(sp)
    li      t3, \mem_exp
    beq     t2, t3, 1f
2:
    li      a1, \id
    j       bist_insn_amo_fail
1:
.endm

bist_cpu_instruction_amo:
    addi    sp, sp, -16

    /* ---- Zaamo: AMO*.W (rd = old; mem = op(old, rs2)) ---- */
bist_insn_amoadd_w_small:
    CHECK_AMO amoadd.w, 1, 10, 5, 10, 15

bist_insn_amoadd_w_overflow:
    CHECK_AMO amoadd.w, 2, 0x7FFFFFFF, 1, 0x7FFFFFFF, 0x80000000

bist_insn_amoswap_w:
    CHECK_AMO amoswap.w, 3, 0x12345678, 0xA5A5A5A5, 0x12345678, 0xA5A5A5A5

bist_insn_amoand_w:
    CHECK_AMO amoand.w, 4, 0xFFFFFFFF, 0x0F0F0F0F, 0xFFFFFFFF, 0x0F0F0F0F

bist_insn_amoor_w:
    CHECK_AMO amoor.w, 5, 0, 0xAAAAAAAA, 0, 0xAAAAAAAA

bist_insn_amoxor_w:
    CHECK_AMO amoxor.w, 6, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0xFFFFFFFF

bist_insn_amomin_w:
    CHECK_AMO amomin.w, 7, -1, 1, -1, -1

bist_insn_amomax_w:
    CHECK_AMO amomax.w, 8, -1, 1, -1, 1

bist_insn_amominu_w:
    CHECK_AMO amominu.w, 9, -1, 1, -1, 1

bist_insn_amomaxu_w:
    CHECK_AMO amomaxu.w, 10, -1, 1, -1, -1

    /* ---- Zalrsc: LR.W + SC.W success (tight reservation window) ---- */
bist_insn_lrsc_w_success:
    li      t0, 0x11111111
    sw      t0, 0(sp)
    lr.w    t2, (sp)
    li      t1, 0x22222222
    sc.w    t3, t1, (sp)
    bnez    t3, 2f
    li      t4, 0x11111111
    bne     t2, t4, 2f
    lw      t2, 0(sp)
    li      t4, 0x22222222
    beq     t2, t4, 1f
2:
    li      a1, 11
    j       bist_insn_amo_fail
1:

    /* ---- LR.W + SC.W fail: intervening SW to the reserved word ---- */
bist_insn_lrsc_w_fail:
    li      t0, 0x11111111
    sw      t0, 0(sp)
    lr.w    t2, (sp)
    li      t1, 0xDEADBEEF
    sw      t1, 0(sp)
    li      t4, 0xCAFEBABE
    sc.w    t3, t4, (sp)
    beqz    t3, 2f              /* SC must fail (rd != 0) */
    lw      t2, 0(sp)
    li      t4, 0xDEADBEEF
    beq     t2, t4, 1f          /* mem is intervening store, not SC data */
2:
    li      a1, 12
    j       bist_insn_amo_fail
1:

    /* ---- Optional aq/rl encoding: amoswap.w.aqrl ---- */
bist_insn_amoswap_w_aqrl:
    CHECK_AMO amoswap.w.aqrl, 13, 0x01234567, 0x89ABCDEF, 0x01234567, 0x89ABCDEF

    j       bist_insn_amo_pass

bist_insn_amo_fail:
    addi    sp, sp, 16
    la      t0, bist_cpu_instruction_failure_id
    sw      a1, 0(t0)
    li      a0, 14          /* BIST_ESP_CPU_INSTRUCTION_TEST_ERR */
    j       bist_insn_amo_done

bist_insn_amo_pass:
    addi    sp, sp, 16
    la      t0, bist_cpu_instruction_failure_id
    sw      zero, 0(t0)
    li      a0, 0           /* BIST_ESP_OK */

bist_insn_amo_done:
    .option pop
    ret

    .size bist_cpu_instruction_amo, .-bist_cpu_instruction_amo

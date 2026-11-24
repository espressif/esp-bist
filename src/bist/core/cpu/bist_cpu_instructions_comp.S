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
 * IEC 60730 component 1.2 — RV32C compressed instruction vectors.
 * The arithmetic itself is already covered by the RV32I group; what is new
 * here is the decode: immediate fields scattered across non-contiguous
 * encoding bits, a 3-bit register field biased to x8-x15, and mnemonics that
 * share a major opcode and are told apart only by a register value.
 *
 * Unlike every other group this file runs with .option rvc, because each
 * vector must execute the 16-bit encoding rather than its 32-bit form.
 * Two independent mechanisms establish that:
 *
 *   CINSN         asserts at assembly time that the instruction encoded in
 *                 16 bits. The assembler rejects rather than widens every
 *                 arithmetic, logic and load/store compressed mnemonic, so
 *                 this is exact.
 *   size witness  c.j, c.jal, c.beqz and c.bnez are the exception: the
 *                 assembler silently widens them when the target is out of
 *                 range, and their length is not a parse-time constant. The
 *                 encoding size is emitted as a label difference in .rodata,
 *                 which the linker resolves after relaxation, and checked at
 *                 run time. That describes the linked image, not the object.
 *
 * No .align may appear inside a measured region: it emits R_RISCV_ALIGN
 * padding the linker later deletes, which would make the size stale.
 *
 * ra, s0 and s1 are clobbered (c.jal/c.jalr write ra; s0/s1 cover the low
 * end of the 3-bit register field) and restored on both exit paths. t4 holds
 * the frame base for the whole group so c.addi16sp can restore sp.
 */

    .text
    .option push
    .option rvc
    .align 2
    .global bist_cpu_instruction_comp
    .type   bist_cpu_instruction_comp, @function
    .extern bist_cpu_instruction_failure_id

/* Frame layout, sp-relative:
 *
 *   0..255     c.lwsp / c.swsp scratch, reached by literal offsets
 *   256..383   c.lw / c.sw scratch
 *   528..539   saved ra, s0, s1
 *
 * c.addi16sp moves sp up by as much as 496, so the saved registers sit above
 * that mark: an interrupt taken while sp is displaced still pushes below
 * them. Their offsets are out of c.swsp range anyway, so they use plain sw.
 */
    .equ COMP_FRAME,    544
    .equ COMP_LWBUF,    256
    .equ COMP_SAVE_RA,  528
    .equ COMP_SAVE_S0,  532
    .equ COMP_SAVE_S1,  536

/* Execute exactly one compressed instruction, failing the build if the
 * assembler did not encode it in 16 bits.
 */
.macro CINSN op, args:vararg
8:  \op     \args
9:
    .if (9b - 8b) != 2
    .error "vector did not assemble to a 16-bit compressed encoding"
    .endif
.endm

/* Compare \reg against \ref; on mismatch record \id and fail. */
.macro CHECK_REG id, reg, ref
    beq     \reg, \ref, 7f
    li      a1, \id
    j       bist_insn_comp_fail
7:
.endm

/* Compare \reg against the immediate \exp. */
.macro CHECK id, reg, exp
    li      t3, \exp
    CHECK_REG \id, \reg, t3
.endm

/* Fail \id unless the linker-resolved encoding size at \sym is 2 bytes. */
.macro CHECK_WITNESS id, sym
    la      t0, \sym
    lbu     t1, 0(t0)
    li      t3, 2
    CHECK_REG \id, t1, t3
.endm

/* rd = imm, no prior value (c.li, c.lui). */
.macro C_II id, op, rd, imm, exp
    CINSN   \op, \rd, \imm
    CHECK   \id, \rd, \exp
.endm

/* rd = rd op imm (c.addi, c.slli, c.srli, c.srai, c.andi). */
.macro C_RI id, op, rd, init, imm, exp
    li      \rd, \init
    CINSN   \op, \rd, \imm
    CHECK   \id, \rd, \exp
.endm

/* rd = rd op rs (c.mv, c.add, c.sub, c.xor, c.or, c.and). */
.macro C_RR id, op, rd, rs, a, b, exp
    li      \rd, \a
    li      \rs, \b
    CINSN   \op, \rd, \rs
    CHECK   \id, \rd, \exp
.endm

/* rd = sp + nzuimm, compared against the same sum built uncompressed.
 * The largest offsets point above the frame; they are only compared,
 * never dereferenced.
 */
.macro C_ADDI4SPN id, rd, imm
    addi    t3, sp, \imm
    CINSN   c.addi4spn, \rd, sp, \imm
    CHECK_REG \id, \rd, t3
.endm

/* Round-trip \val through memory with a compressed store/load pair. */
.macro C_LDST id, base, data, off, val
    li      \data, \val
    CINSN   c.sw, \data, \off(\base)
    li      \data, 0
    CINSN   c.lw, \data, \off(\base)
    CHECK   \id, \data, \val
.endm

/* Same, through the sp-relative encodings, which scramble the offset bits
 * differently in each direction.
 */
.macro C_LDSTSP id, data, off, val
    li      \data, \val
    CINSN   c.swsp, \data, \off(sp)
    li      \data, 0
    CINSN   c.lwsp, \data, \off(sp)
    CHECK   \id, \data, \val
.endm

/* sp += nzimm, restored from the frame base before the result is checked so
 * a failure never leaves sp displaced.
 */
.macro C_ADDI16SP id, imm
    mv      sp, t4
    addi    t3, t4, \imm
    CINSN   c.addi16sp, sp, \imm
    mv      t5, sp
    mv      sp, t4
    CHECK_REG \id, t5, t3
.endm

bist_cpu_instruction_comp:
    addi    sp, sp, -COMP_FRAME
    sw      ra, COMP_SAVE_RA(sp)
    sw      s0, COMP_SAVE_S0(sp)
    sw      s1, COMP_SAVE_S1(sp)
    mv      t4, sp

    /* ---- CI: C.ADDI (1-6): nzimm[5:0], sign-extended, rd unrestricted ---- */
bist_insn_comp_addi_pos_min:
    C_RI    1, c.addi, a2, 0, 1, 1
bist_insn_comp_addi_pos_max:
    C_RI    2, c.addi, a2, 0, 31, 31
bist_insn_comp_addi_neg_min:
    C_RI    3, c.addi, a2, 0, -1, 0xFFFFFFFF
bist_insn_comp_addi_neg_max:
    C_RI    4, c.addi, a2, 0, -32, 0xFFFFFFE0
bist_insn_comp_addi_carry:
    C_RI    5, c.addi, a2, 0xFFFFFFFF, 1, 0
/* x31: the high end of the 5-bit field, unreachable from the 3-bit one. */
bist_insn_comp_addi_reg_high:
    C_RI    6, c.addi, t6, 0x1000, 1, 0x1001

    /* ---- C.NOP (7): c.addi x0, 0, architecturally a no-op ---- */
bist_insn_comp_nop:
    li      a2, 0x5A5A5A5A
    CINSN   c.nop
    CHECK   7, a2, 0x5A5A5A5A

    /* ---- CI: C.LI (8-11): imm[5:0] sign-extended to 32 bits ---- */
bist_insn_comp_li_zero:
    C_II    8, c.li, a2, 0, 0
bist_insn_comp_li_pos_max:
    C_II    9, c.li, a2, 31, 31
bist_insn_comp_li_neg_max:
    C_II    10, c.li, a2, -32, 0xFFFFFFE0
bist_insn_comp_li_reg_high:
    C_II    11, c.li, t6, -1, 0xFFFFFFFF

    /* ---- CI: C.LUI (12-15): nzimm[17|16:12] sign-extended into 31:12 ---- */
bist_insn_comp_lui_min:
    C_II    12, c.lui, a2, 1, 0x00001000
bist_insn_comp_lui_pos_max:
    C_II    13, c.lui, a2, 31, 0x0001F000
bist_insn_comp_lui_neg_min:
    C_II    14, c.lui, a2, 0xFFFE0, 0xFFFE0000
bist_insn_comp_lui_neg_max:
    C_II    15, c.lui, a2, 0xFFFFF, 0xFFFFF000

    /* ---- Shifts (16-23): shamt[4:0], shamt 0 and bit 12 set are reserved.
     * c.slli is CI and takes any rd; c.srli and c.srai are CB and are limited
     * to x8-x15.
     */
bist_insn_comp_slli_one:
    C_RI    16, c.slli, a2, 1, 1, 2
bist_insn_comp_slli_max:
    C_RI    17, c.slli, a2, 1, 31, 0x80000000
bist_insn_comp_slli_discard:
    C_RI    18, c.slli, a2, 0xF0000000, 4, 0
bist_insn_comp_slli_reg_high:
    C_RI    19, c.slli, t6, 1, 31, 0x80000000
bist_insn_comp_srli_one:
    C_RI    20, c.srli, a2, 0x80000000, 1, 0x40000000
bist_insn_comp_srli_max:
    C_RI    21, c.srli, a2, 0x80000000, 31, 1
bist_insn_comp_srai_sign_fill:
    C_RI    22, c.srai, a2, 0x80000000, 31, 0xFFFFFFFF
bist_insn_comp_srai_positive:
    C_RI    23, c.srai, a2, 0x40000000, 30, 1

    /* ---- CB: C.ANDI (24-26): imm[5:0] sign-extended, zero permitted ---- */
bist_insn_comp_andi_mask_low:
    C_RI    24, c.andi, a2, 0xFFFFFFFF, 31, 31
bist_insn_comp_andi_sign_ext:
    C_RI    25, c.andi, a2, 0xFFFFFFFF, -32, 0xFFFFFFE0
bist_insn_comp_andi_zero:
    C_RI    26, c.andi, a2, 0xFFFFFFFF, 0, 0

    /* ---- CR: C.MV / C.ADD (27-32): both operands use the 5-bit field ---- */
bist_insn_comp_mv_low:
    C_RR    27, c.mv, a2, a3, 0, 0xA5A5A5A5, 0xA5A5A5A5
bist_insn_comp_mv_reg_high:
    C_RR    28, c.mv, t6, a2, 0, 0x5A5A5A5A, 0x5A5A5A5A
/* x1: the low end of the 5-bit field. ra is saved in the prologue. */
bist_insn_comp_mv_reg_ra:
    C_RR    29, c.mv, a2, ra, 0, 0x12345678, 0x12345678
bist_insn_comp_add_normal:
    C_RR    30, c.add, a2, a3, 5, 7, 12
bist_insn_comp_add_carry:
    C_RR    31, c.add, a2, a3, 0xFFFFFFFF, 1, 0
bist_insn_comp_add_reg_high:
    C_RR    32, c.add, t6, a2, 0x1000, 0x234, 0x1234

    /* ---- CA: C.SUB / C.XOR / C.OR / C.AND (33-39) ----
     * All four share funct3 100 in quadrant C1 and are selected by bits 6:5,
     * so a swapped field is only visible if every one of them runs.
     */
bist_insn_comp_sub_normal:
    C_RR    33, c.sub, a2, a3, 10, 3, 7
bist_insn_comp_sub_borrow:
    C_RR    34, c.sub, a2, a3, 0, 1, 0xFFFFFFFF
bist_insn_comp_xor:
    C_RR    35, c.xor, a2, a3, 0xF0F0F0F0, 0xFFFFFFFF, 0x0F0F0F0F
bist_insn_comp_or:
    C_RR    36, c.or, a2, a3, 0xF0F0F0F0, 0x0F0F0F0F, 0xFFFFFFFF
bist_insn_comp_and:
    C_RR    37, c.and, a2, a3, 0xF0F0F0F0, 0xFF00FF00, 0xF000F000
/* x8/x9 and x14/x15: the two ends of the 3-bit field, which a decoder that
 * dropped the +8 bias would still get right in the middle of the range.
 */
bist_insn_comp_and_reg_low:
    C_RR    38, c.and, s0, s1, 0xFFFF0000, 0x0F0F0F0F, 0x0F0F0000
bist_insn_comp_or_reg_high:
    C_RR    39, c.or, a5, a4, 0xF0, 0x0F, 0xFF

    /* ---- CIW: C.ADDI4SPN (40-46) ----
     * nzuimm[5:4|9:6|2|3] is the second most scrambled field in RVC: one
     * vector per immediate bit. Reads sp, never writes it.
     */
bist_insn_comp_addi4spn_min:
    C_ADDI4SPN 40, a2, 4
bist_insn_comp_addi4spn_bit3:
    C_ADDI4SPN 41, a2, 8
bist_insn_comp_addi4spn_bit4:
    C_ADDI4SPN 42, a2, 16
bist_insn_comp_addi4spn_bit5:
    C_ADDI4SPN 43, a2, 32
bist_insn_comp_addi4spn_bit6:
    C_ADDI4SPN 44, a2, 64
bist_insn_comp_addi4spn_bit9:
    C_ADDI4SPN 45, a2, 512
bist_insn_comp_addi4spn_max:
    C_ADDI4SPN 46, a2, 1020

    /* ---- CL / CS: C.LW / C.SW (47-51) ----
     * uimm[5:3] sits at bits 12:10 but uimm[2|6] at bits 6:5, so bit 6 and
     * bit 2 are adjacent and out of order.
     */
    addi    a3, sp, COMP_LWBUF
bist_insn_comp_ldst_zero_off:
    C_LDST  47, a3, a4, 0, 0x89ABCDEF
bist_insn_comp_ldst_min_off:
    C_LDST  48, a3, a4, 4, 0x01234567
bist_insn_comp_ldst_bit6_off:
    C_LDST  49, a3, a4, 64, 0xDEADBEEF
bist_insn_comp_ldst_max_off:
    C_LDST  50, a3, a4, 124, 0xFEEDFACE
bist_insn_comp_ldst_reg_low:
    addi    s0, sp, COMP_LWBUF
    C_LDST  51, s0, s1, 8, 0xC0FFEE00

    /* ---- CI / CSS: C.LWSP / C.SWSP (52-58) ----
     * c.lwsp packs uimm[5] at bit 12 and uimm[4:2|7:6] at 6:2, while c.swsp
     * packs uimm[5:2|7:6] at 12:7 — the same offset, two different layouts,
     * so both directions need their own vectors.
     */
bist_insn_comp_ldstsp_zero_off:
    C_LDSTSP 52, a2, 0, 0x11223344
bist_insn_comp_ldstsp_min_off:
    C_LDSTSP 53, a2, 4, 0x55667788
bist_insn_comp_ldstsp_bit5_off:
    C_LDSTSP 54, a2, 32, 0x99AABBCC
bist_insn_comp_ldstsp_bit6_off:
    C_LDSTSP 55, a2, 64, 0xDDEEFF00
bist_insn_comp_ldstsp_bit7_off:
    C_LDSTSP 56, a2, 128, 0x13579BDF
bist_insn_comp_ldstsp_max_off:
    C_LDSTSP 57, a2, 252, 0x2468ACE0
bist_insn_comp_ldstsp_reg_high:
    C_LDSTSP 58, t6, 4, 0xA5A5F0F0

    /* ---- CI: C.ADDI16SP (59-65) ----
     * nzimm[9|4|6|8:7|5] is the most scrambled field in RVC, and the only
     * instruction here that writes sp. Every movement stays inside the frame
     * reserved above, so the saved registers are never left below sp.
     */
bist_insn_comp_addi16sp_bit4:
    C_ADDI16SP 59, 16
bist_insn_comp_addi16sp_bit5:
    C_ADDI16SP 60, 32
bist_insn_comp_addi16sp_bit6:
    C_ADDI16SP 61, 64
bist_insn_comp_addi16sp_bit7:
    C_ADDI16SP 62, 128
bist_insn_comp_addi16sp_bit8:
    C_ADDI16SP 63, 256
bist_insn_comp_addi16sp_max:
    C_ADDI16SP 64, 496
bist_insn_comp_addi16sp_neg:
    C_ADDI16SP 65, -16

    /* ---- Decode discrimination (66) ----
     * c.lui and c.addi16sp share funct3 011 in quadrant C1 and are told apart
     * only by rd == x2. Run both back to back and require both results.
     */
bist_insn_comp_lui_vs_addi16sp:
    mv      sp, t4
    CINSN   c.lui, a2, 1
    CINSN   c.addi16sp, sp, 16
    mv      t5, sp
    mv      sp, t4
    CHECK   66, a2, 0x00001000
    addi    t3, t4, 16
    CHECK_REG 66, t5, t3

    /* ---- CJ / CB: control flow (67-75) ----
     * The offsets here cannot be pushed to their encoding limits: forcing a
     * far target makes the assembler widen the instruction instead. Only the
     * short forward and backward directions are reachable.
     */
bist_insn_comp_j_forward:
bist_insn_comp_j_fwd_start:
    c.j     1f
bist_insn_comp_j_fwd_end:
    li      a1, 67
    j       bist_insn_comp_fail
1:
    CHECK_WITNESS 67, bist_insn_comp_j_fwd_size

bist_insn_comp_j_backward:
    c.j     2f
1:
    c.j     3f
2:
bist_insn_comp_j_bwd_start:
    c.j     1b
bist_insn_comp_j_bwd_end:
    li      a1, 68
    j       bist_insn_comp_fail
3:
    CHECK_WITNESS 68, bist_insn_comp_j_bwd_size

bist_insn_comp_beqz_taken:
    li      a2, 0
bist_insn_comp_beqz_t_start:
    c.beqz  a2, 1f
bist_insn_comp_beqz_t_end:
    li      a1, 69
    j       bist_insn_comp_fail
1:
    CHECK_WITNESS 69, bist_insn_comp_beqz_t_size

bist_insn_comp_beqz_not_taken:
    li      a2, 1
bist_insn_comp_beqz_n_start:
    c.beqz  a2, 1f
bist_insn_comp_beqz_n_end:
    j       2f
1:
    li      a1, 70
    j       bist_insn_comp_fail
2:
    CHECK_WITNESS 70, bist_insn_comp_beqz_n_size

bist_insn_comp_bnez_taken:
    li      a2, 1
bist_insn_comp_bnez_t_start:
    c.bnez  a2, 1f
bist_insn_comp_bnez_t_end:
    li      a1, 71
    j       bist_insn_comp_fail
1:
    CHECK_WITNESS 71, bist_insn_comp_bnez_t_size

bist_insn_comp_bnez_not_taken:
    li      a2, 0
bist_insn_comp_bnez_n_start:
    c.bnez  a2, 1f
bist_insn_comp_bnez_n_end:
    j       2f
1:
    li      a1, 72
    j       bist_insn_comp_fail
2:
    CHECK_WITNESS 72, bist_insn_comp_bnez_n_size

/* c.jal links to the instruction after itself, so ra is a direct second
 * proof that the encoding was two bytes rather than four.
 */
bist_insn_comp_jal_link:
bist_insn_comp_jal_start:
    c.jal   1f
bist_insn_comp_jal_end:
    li      a1, 73
    j       bist_insn_comp_fail
1:
    la      t3, bist_insn_comp_jal_end
    CHECK_REG 73, ra, t3
    CHECK_WITNESS 73, bist_insn_comp_jal_size

bist_insn_comp_jr:
    la      a2, 1f
    CINSN   c.jr, a2
    li      a1, 74
    j       bist_insn_comp_fail
1:

bist_insn_comp_jalr_link:
    la      a2, 1f
    CINSN   c.jalr, a2
bist_insn_comp_jalr_end:
    li      a1, 75
    j       bist_insn_comp_fail
1:
    la      t3, bist_insn_comp_jalr_end
    CHECK_REG 75, ra, t3

    j       bist_insn_comp_pass

bist_insn_comp_fail:
    mv      sp, t4
    lw      ra, COMP_SAVE_RA(sp)
    lw      s0, COMP_SAVE_S0(sp)
    lw      s1, COMP_SAVE_S1(sp)
    addi    sp, sp, COMP_FRAME
    la      t0, bist_cpu_instruction_failure_id
    sw      a1, 0(t0)
    li      a0, 14          /* BIST_ESP_CPU_INSTRUCTION_TEST_ERR */
    j       bist_insn_comp_done

bist_insn_comp_pass:
    mv      sp, t4
    lw      ra, COMP_SAVE_RA(sp)
    lw      s0, COMP_SAVE_S0(sp)
    lw      s1, COMP_SAVE_S1(sp)
    addi    sp, sp, COMP_FRAME
    la      t0, bist_cpu_instruction_failure_id
    sw      zero, 0(t0)
    li      a0, 0           /* BIST_ESP_OK */

bist_insn_comp_done:
    .option pop
    ret

    .size bist_cpu_instruction_comp, .-bist_cpu_instruction_comp

/* Encoding sizes for the four mnemonics the assembler may widen without
 * diagnosing it. Each is a label difference the linker resolves after
 * relaxation, so it reports the size in the linked image.
 */
    .section .rodata
bist_insn_comp_j_fwd_size:
    .byte   bist_insn_comp_j_fwd_end - bist_insn_comp_j_fwd_start
bist_insn_comp_j_bwd_size:
    .byte   bist_insn_comp_j_bwd_end - bist_insn_comp_j_bwd_start
bist_insn_comp_beqz_t_size:
    .byte   bist_insn_comp_beqz_t_end - bist_insn_comp_beqz_t_start
bist_insn_comp_beqz_n_size:
    .byte   bist_insn_comp_beqz_n_end - bist_insn_comp_beqz_n_start
bist_insn_comp_bnez_t_size:
    .byte   bist_insn_comp_bnez_t_end - bist_insn_comp_bnez_t_start
bist_insn_comp_bnez_n_size:
    .byte   bist_insn_comp_bnez_n_end - bist_insn_comp_bnez_n_start
bist_insn_comp_jal_size:
    .byte   bist_insn_comp_jal_end - bist_insn_comp_jal_start

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
 * IEC 60730 component 1.2 — RV32F single-precision vectors.
 * Results are compared as exact IEEE-754 binary32 bit patterns, never with
 * a tolerance. Fail fast on first mismatch; store failing vector ID in
 * bist_cpu_instruction_failure_id.
 *
 * F is enabled in this file only (.option arch, +f), not via global -march.
 * Build and call this group only on FPU targets (ESP32-H4, ESP32-P4).
 *
 * Only caller-saved float registers (ft0-ft3) are used, so fs0-fs11 keep
 * their ABI-preserved contents. fcsr is written (rounding mode and exception
 * flags) and restored on both the pass and fail paths.
 *
 * Every NaN produced by an arithmetic operation is the RISC-V canonical
 * quiet NaN 0x7FC00000; NaN payloads only survive the non-arithmetic
 * flw/fsw and fsgnj* moves.
 */

    .text
    .option push
    .option norvc
    .option arch, +f
    .align 2
    .global bist_cpu_instruction_fp
    .type   bist_cpu_instruction_fp, @function
    .extern bist_cpu_instruction_failure_id

/* IEEE-754 binary32 bit patterns used by the vectors below. */
    .equ FP_P_ZERO,     0x00000000  /* +0.0                         */
    .equ FP_N_ZERO,     0x80000000  /* -0.0                         */
    .equ FP_SUBNORMAL,  0x00000001  /* smallest positive subnormal  */
    .equ FP_ONE,        0x3F800000  /*  1.0                         */
    .equ FP_N_ONE,      0xBF800000  /* -1.0                         */
    .equ FP_TWO,        0x40000000  /*  2.0                         */
    .equ FP_N_TWO,      0xC0000000  /* -2.0                         */
    .equ FP_THREE,      0x40400000  /*  3.0                         */
    .equ FP_FOUR,       0x40800000  /*  4.0                         */
    .equ FP_FIVE,       0x40A00000  /*  5.0                         */
    .equ FP_SIX,        0x40C00000  /*  6.0                         */
    .equ FP_SEVEN,      0x40E00000  /*  7.0                         */
    .equ FP_N_FIVE,     0xC0A00000  /* -5.0                         */
    .equ FP_N_SEVEN,    0xC0E00000  /* -7.0                         */
    .equ FP_ONE_HALF,   0x3FC00000  /*  1.5                         */
    .equ FP_P_INF,      0x7F800000  /* +infinity                    */
    .equ FP_N_INF,      0xFF800000  /* -infinity                    */
    .equ FP_QNAN,       0x7FC00000  /* canonical quiet NaN          */
    .equ FP_SNAN,       0x7F812345  /* signaling NaN, nonzero payload */
    .equ FP_ONE_ULP,    0x3F800001  /* 1.0 + 2^-23                  */
    .equ FP_ONE_MULP,   0x3F7FFFFF  /* 1.0 - 2^-24                  */
    .equ FP_HALF_ULP,   0x33C00000  /* 1.5 * 2^-24, rounds up in RNE */
    .equ FP_FMA_EXACT,  0x337FFFFE  /* single-rounded (1+2^-23)(1-2^-24)-1 */
    .equ FP_1E10,       0x501502F9  /*  1e10, above INT32_MAX       */
    .equ FP_N_1E10,     0xD01502F9  /* -1e10, below INT32_MIN       */
    .equ FP_4E9,        0x4F6E6B28  /*  4e9, above INT32_MAX, exact */
    .equ FP_N_2POW31,   0xCF000000  /* -2147483648.0                */
    .equ FP_2POW32,     0x4F800000  /*  4294967296.0                */

/* li the bit pattern \bits into \xreg, then move it into \freg. */
.macro LOADF freg, xreg, bits
    li      \xreg, \bits
    fmv.w.x \freg, \xreg
.endm

/* Compare the raw bits of \freg against \exp. */
.macro CHECK_BITS id, freg, exp
    fmv.x.w t2, \freg
    li      t3, \exp
    beq     t2, t3, 1f
    li      a1, \id
    j       bist_insn_fp_fail
1:
.endm

/* Compare an integer result (compare / convert / classify) against \exp. */
.macro CHECK_INT id, xreg, exp
    li      t3, \exp
    beq     \xreg, t3, 1f
    li      a1, \id
    j       bist_insn_fp_fail
1:
.endm

/* fd = op(fs1, fs2), dynamic rounding mode (frm = RNE). */
.macro CHECK_FRR op, id, a, b, exp
    LOADF   ft0, t0, \a
    LOADF   ft1, t1, \b
    \op     ft2, ft0, ft1
    CHECK_BITS \id, ft2, \exp
.endm

/* fd = op(fs1, fs2) with a static rounding mode in the encoding. */
.macro CHECK_FRR_RM op, id, a, b, rm, exp
    LOADF   ft0, t0, \a
    LOADF   ft1, t1, \b
    \op     ft2, ft0, ft1, \rm
    CHECK_BITS \id, ft2, \exp
.endm

/* fd = op(fs1). */
.macro CHECK_FUN op, id, a, exp
    LOADF   ft0, t0, \a
    \op     ft1, ft0
    CHECK_BITS \id, ft1, \exp
.endm

/* fd = op(fs1, fs2, fs3), one rounding of the exact product-sum. */
.macro CHECK_FMA op, id, a, b, c, exp
    LOADF   ft0, t0, \a
    LOADF   ft1, t1, \b
    LOADF   ft2, t2, \c
    \op     ft3, ft0, ft1, ft2
    CHECK_BITS \id, ft3, \exp
.endm

/* xd = op(fs1, fs2) for feq.s / flt.s / fle.s. */
.macro CHECK_FCMP op, id, a, b, exp
    LOADF   ft0, t0, \a
    LOADF   ft1, t1, \b
    \op     t2, ft0, ft1
    CHECK_INT \id, t2, \exp
.endm

/* xd = op(fs1) for fcvt.w.s / fcvt.wu.s, static rounding mode. */
.macro CHECK_FCVT_X op, id, a, rm, exp
    LOADF   ft0, t0, \a
    \op     t2, ft0, \rm
    CHECK_INT \id, t2, \exp
.endm

/* fd = op(xs1) for fcvt.s.w / fcvt.s.wu, static rounding mode. */
.macro CHECK_FCVT_F op, id, a, rm, exp
    li      t0, \a
    \op     ft0, t0, \rm
    CHECK_BITS \id, ft0, \exp
.endm

/* fclass.s one-hot class mask. */
.macro CHECK_FCLASS id, a, exp
    LOADF   ft0, t0, \a
    fclass.s t2, ft0
    CHECK_INT \id, t2, \exp
.endm

/* Clear fflags, run a binary op, check the accrued flags. */
.macro CHECK_FFLAGS_RR op, id, a, b, exp
    csrw    fflags, zero
    LOADF   ft0, t0, \a
    LOADF   ft1, t1, \b
    \op     ft2, ft0, ft1
    csrr    t2, fflags
    CHECK_INT \id, t2, \exp
.endm

/* Clear fflags, run a unary op, check the accrued flags. */
.macro CHECK_FFLAGS_UN op, id, a, exp
    csrw    fflags, zero
    LOADF   ft0, t0, \a
    \op     ft1, ft0
    csrr    t2, fflags
    CHECK_INT \id, t2, \exp
.endm

bist_cpu_instruction_fp:
    addi    sp, sp, -16
    csrr    t6, fcsr
    sw      t6, 0(sp)
    csrw    frm, zero           /* dynamic rounding mode := RNE */
    csrw    fflags, zero

    /* ---- FSW / FLW (1-2): bit-exact word moves, no canonicalization ---- */
bist_insn_fp_fsw_flw_roundtrip:
    LOADF   ft0, t0, FP_ONE
    fsw     ft0, 4(sp)
    flw     ft1, 4(sp)
    CHECK_BITS 1, ft1, FP_ONE

bist_insn_fp_flw_nan_payload:
    LOADF   ft0, t0, FP_SNAN
    fsw     ft0, 4(sp)
    flw     ft1, 4(sp)
    CHECK_BITS 2, ft1, FP_SNAN

    /* ---- FADD.S / FSUB.S (3-7) ---- */
bist_insn_fp_fadd_normal:
    CHECK_FRR fadd.s, 3, FP_ONE, FP_TWO, FP_THREE
bist_insn_fp_fadd_inf_sub_inf:
    CHECK_FRR fadd.s, 4, FP_P_INF, FP_N_INF, FP_QNAN
bist_insn_fp_fadd_subnormal:
    CHECK_FRR fadd.s, 5, FP_SUBNORMAL, FP_SUBNORMAL, 0x00000002
bist_insn_fp_fsub_normal:
    CHECK_FRR fsub.s, 6, FP_THREE, FP_ONE, FP_TWO
bist_insn_fp_fsub_neg_zeros:
    CHECK_FRR fsub.s, 7, FP_N_ZERO, FP_P_ZERO, FP_N_ZERO

    /* ---- FMUL.S / FDIV.S / FSQRT.S (8-13) ---- */
bist_insn_fp_fmul_normal:
    CHECK_FRR fmul.s, 8, FP_TWO, FP_THREE, FP_SIX
bist_insn_fp_fmul_sign_zero:
    CHECK_FRR fmul.s, 9, FP_N_ONE, FP_P_ZERO, FP_N_ZERO
bist_insn_fp_fdiv_normal:
    CHECK_FRR fdiv.s, 10, FP_THREE, FP_TWO, FP_ONE_HALF
bist_insn_fp_fdiv_neg_by_zero:
    CHECK_FRR fdiv.s, 11, FP_N_ONE, FP_P_ZERO, FP_N_INF
bist_insn_fp_fsqrt_exact:
    CHECK_FUN fsqrt.s, 12, FP_FOUR, FP_TWO
bist_insn_fp_fsqrt_negative:
    CHECK_FUN fsqrt.s, 13, FP_N_ONE, FP_QNAN

    /* ---- Fused multiply-add (14-19) ----
     * fmadd  =  (a*b)+c    fmsub  =  (a*b)-c
     * fnmsub = -(a*b)+c    fnmadd = -(a*b)-c
     * Vector 15 separates fused from unfused: an unfused (a*b)+c would
     * round the product first and yield +0.0 instead.
     */
bist_insn_fp_fmadd_normal:
    CHECK_FMA fmadd.s, 14, FP_TWO, FP_THREE, FP_ONE, FP_SEVEN
bist_insn_fp_fmadd_single_rounding:
    CHECK_FMA fmadd.s, 15, FP_ONE_ULP, FP_ONE_MULP, FP_N_ONE, FP_FMA_EXACT
bist_insn_fp_fmadd_inf_zero:
    CHECK_FMA fmadd.s, 16, FP_P_INF, FP_P_ZERO, FP_ONE, FP_QNAN
bist_insn_fp_fmsub_normal:
    CHECK_FMA fmsub.s, 17, FP_TWO, FP_THREE, FP_ONE, FP_FIVE
bist_insn_fp_fnmsub_normal:
    CHECK_FMA fnmsub.s, 18, FP_TWO, FP_THREE, FP_ONE, FP_N_FIVE
bist_insn_fp_fnmadd_normal:
    CHECK_FMA fnmadd.s, 19, FP_TWO, FP_THREE, FP_ONE, FP_N_SEVEN

    /* ---- Sign injection (20-24): rd = {sign(rs2) op, magnitude(rs1)} ---- */
bist_insn_fp_fsgnj_take_neg:
    CHECK_FRR fsgnj.s, 20, FP_ONE, FP_N_ONE, FP_N_ONE
bist_insn_fp_fsgnjn_negate:
    CHECK_FRR fsgnjn.s, 21, FP_ONE, FP_ONE, FP_N_ONE
bist_insn_fp_fsgnjx_opposite_signs:
    CHECK_FRR fsgnjx.s, 22, FP_ONE, FP_N_ONE, FP_N_ONE

/* rs1 == rs2 is the fmv.s pseudo-instruction: NaN payload must survive. */
bist_insn_fp_fsgnj_same_reg_nan:
    LOADF   ft0, t0, FP_SNAN
    fsgnj.s ft1, ft0, ft0
    CHECK_BITS 23, ft1, FP_SNAN

/* rs1 == rs2 is the fabs.s pseudo-instruction. */
bist_insn_fp_fsgnjx_same_reg_abs:
    LOADF   ft0, t0, FP_N_TWO
    fsgnjx.s ft1, ft0, ft0
    CHECK_BITS 24, ft1, FP_TWO

    /* ---- FMIN.S / FMAX.S (25-30): NaN operand yields the other operand ---- */
bist_insn_fp_fmin_normal:
    CHECK_FRR fmin.s, 25, FP_ONE, FP_TWO, FP_ONE
bist_insn_fp_fmin_signed_zeros:
    CHECK_FRR fmin.s, 26, FP_P_ZERO, FP_N_ZERO, FP_N_ZERO
bist_insn_fp_fmin_nan_operand:
    CHECK_FRR fmin.s, 27, FP_QNAN, FP_TWO, FP_TWO
bist_insn_fp_fmax_normal:
    CHECK_FRR fmax.s, 28, FP_ONE, FP_TWO, FP_TWO
bist_insn_fp_fmax_signed_zeros:
    CHECK_FRR fmax.s, 29, FP_N_ZERO, FP_P_ZERO, FP_P_ZERO
bist_insn_fp_fmax_nan_operand:
    CHECK_FRR fmax.s, 30, FP_ONE, FP_QNAN, FP_ONE

    /* ---- FEQ.S / FLT.S / FLE.S (31-38): NaN is unordered, +0 == -0 ---- */
bist_insn_fp_feq_equal:
    CHECK_FCMP feq.s, 31, FP_ONE, FP_ONE, 1
bist_insn_fp_feq_signed_zeros:
    CHECK_FCMP feq.s, 32, FP_P_ZERO, FP_N_ZERO, 1
bist_insn_fp_feq_nan:
    CHECK_FCMP feq.s, 33, FP_QNAN, FP_ONE, 0
bist_insn_fp_flt_infinities:
    CHECK_FCMP flt.s, 34, FP_N_INF, FP_P_INF, 1
bist_insn_fp_flt_signed_zeros:
    CHECK_FCMP flt.s, 35, FP_N_ZERO, FP_P_ZERO, 0
bist_insn_fp_flt_nan:
    CHECK_FCMP flt.s, 36, FP_QNAN, FP_QNAN, 0
bist_insn_fp_fle_signed_zeros:
    CHECK_FCMP fle.s, 37, FP_P_ZERO, FP_N_ZERO, 1
bist_insn_fp_fle_nan:
    CHECK_FCMP fle.s, 38, FP_ONE, FP_QNAN, 0

    /* ---- FCVT float -> int (39-45): out-of-range and NaN saturate ---- */
bist_insn_fp_fcvt_w_s_rtz:
    CHECK_FCVT_X fcvt.w.s, 39, FP_ONE_HALF, rtz, 1
bist_insn_fp_fcvt_w_s_rne:
    CHECK_FCVT_X fcvt.w.s, 40, FP_ONE_HALF, rne, 2
bist_insn_fp_fcvt_w_s_sat_pos:
    CHECK_FCVT_X fcvt.w.s, 41, FP_1E10, rtz, 0x7FFFFFFF
bist_insn_fp_fcvt_w_s_sat_neg:
    CHECK_FCVT_X fcvt.w.s, 42, FP_N_1E10, rtz, 0x80000000
bist_insn_fp_fcvt_w_s_nan:
    CHECK_FCVT_X fcvt.w.s, 43, FP_QNAN, rtz, 0x7FFFFFFF
bist_insn_fp_fcvt_wu_s_negative:
    CHECK_FCVT_X fcvt.wu.s, 44, FP_N_ONE, rtz, 0
bist_insn_fp_fcvt_wu_s_large:
    CHECK_FCVT_X fcvt.wu.s, 45, FP_4E9, rtz, 0xEE6B2800

    /* ---- FCVT int -> float (46-48) ---- */
bist_insn_fp_fcvt_s_w_neg:
    CHECK_FCVT_F fcvt.s.w, 46, -1, rne, FP_N_ONE
bist_insn_fp_fcvt_s_w_int_min:
    CHECK_FCVT_F fcvt.s.w, 47, 0x80000000, rne, FP_N_2POW31
bist_insn_fp_fcvt_s_wu_max:
    CHECK_FCVT_F fcvt.s.wu, 48, 0xFFFFFFFF, rne, FP_2POW32

    /* ---- FMV.W.X / FMV.X.W (49): raw bits, not an FP value ---- */
bist_insn_fp_fmv_bit_pattern:
    LOADF   ft0, t0, 0xDEADBEEF
    CHECK_BITS 49, ft0, 0xDEADBEEF

    /* ---- FCLASS.S (50-55): one-hot class mask ---- */
bist_insn_fp_fclass_neg_inf:
    CHECK_FCLASS 50, FP_N_INF, 0x001
bist_insn_fp_fclass_neg_normal:
    CHECK_FCLASS 51, FP_N_ONE, 0x002
bist_insn_fp_fclass_neg_zero:
    CHECK_FCLASS 52, FP_N_ZERO, 0x008
bist_insn_fp_fclass_pos_zero:
    CHECK_FCLASS 53, FP_P_ZERO, 0x010
bist_insn_fp_fclass_pos_normal:
    CHECK_FCLASS 54, FP_ONE, 0x040
bist_insn_fp_fclass_qnan:
    CHECK_FCLASS 55, FP_QNAN, 0x200

    /* ---- Static rounding mode field (56-57) ----
     * 1.0 + 1.5*2^-24 sits above the midpoint between 1.0 and 1.0+2^-23,
     * so rtz truncates while rne rounds up.
     */
bist_insn_fp_fadd_rtz:
    CHECK_FRR_RM fadd.s, 56, FP_ONE, FP_HALF_ULP, rtz, FP_ONE
bist_insn_fp_fadd_rne:
    CHECK_FRR_RM fadd.s, 57, FP_ONE, FP_HALF_ULP, rne, FP_ONE_ULP

    /* ---- fflags accrual (58-60): DZ, NV, NX ---- */
bist_insn_fp_fflags_divide_by_zero:
    CHECK_FFLAGS_RR fdiv.s, 58, FP_ONE, FP_P_ZERO, 0x08
bist_insn_fp_fflags_invalid:
    CHECK_FFLAGS_UN fsqrt.s, 59, FP_N_ONE, 0x10
bist_insn_fp_fflags_inexact:
    CHECK_FFLAGS_RR fdiv.s, 60, FP_ONE, FP_THREE, 0x01

    j       bist_insn_fp_pass

bist_insn_fp_fail:
    lw      t6, 0(sp)
    csrw    fcsr, t6
    addi    sp, sp, 16
    la      t0, bist_cpu_instruction_failure_id
    sw      a1, 0(t0)
    li      a0, 14          /* BIST_ESP_CPU_INSTRUCTION_TEST_ERR */
    j       bist_insn_fp_done

bist_insn_fp_pass:
    lw      t6, 0(sp)
    csrw    fcsr, t6
    addi    sp, sp, 16
    la      t0, bist_cpu_instruction_failure_id
    sw      zero, 0(t0)
    li      a0, 0           /* BIST_ESP_OK */

bist_insn_fp_done:
    .option pop
    ret

    .size bist_cpu_instruction_fp, .-bist_cpu_instruction_fp

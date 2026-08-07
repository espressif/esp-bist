/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ESP32-H4 vector tables (CLIC interrupt controller).
 *
 * MTVEC points to _vector_table for exceptions and non-vectored interrupts.
 * MTVT points to _mtvt_table for hardware-vectored peripheral interrupts.
 *
 * Based on ESP32-P4 BIST vectors with H4-specific DSP and FPU coprocessor support.
 */

#include "riscv/rvruntime-frames.h"
#include "soc/soc_caps.h"
#include "riscv/csr_dsp.h"
#include "esp_private/vectors_const.h"

#if SOC_CPU_COPROC_NUM > 0
/* Illegal-instruction coprocessor reason (read early; writing zero clears). */
#define EXT_ILL_CSR 0x7F0
.equ EXT_ILL_RSN_FPU,  1
.equ EXT_ILL_RSN_DSP,  8
.equ EXT_ILL_RSN_COPROC, (EXT_ILL_RSN_FPU | EXT_ILL_RSN_DSP)
#endif

/* Bare-metal defaults (mirrors IDF vectors_clic.S without full Kconfig). */
#define MEMPROT_ISR         _interrupt_handler
#define ASTDBG_ISR          _panic_handler
#define IPC_ISR_HANDLER     _interrupt_handler
#define _system_int_handler _panic_handler

    .equ SAVE_REGS, 32
    .equ CONTEXT_SIZE, (SAVE_REGS * 4)
    .equ EXC_ILLEGAL_INSTRUCTION, 0x2
    .equ panic_from_exception, xt_unhandled_exception
    .equ panic_from_isr, panicHandler

.macro save_general_regs cxt_size=CONTEXT_SIZE
    addi sp, sp, -\cxt_size
    sw   ra, RV_STK_RA(sp)
    sw   tp, RV_STK_TP(sp)
    sw   t0, RV_STK_T0(sp)
    sw   t1, RV_STK_T1(sp)
    sw   t2, RV_STK_T2(sp)
    sw   s0, RV_STK_S0(sp)
    sw   s1, RV_STK_S1(sp)
    sw   a0, RV_STK_A0(sp)
    sw   a1, RV_STK_A1(sp)
    sw   a2, RV_STK_A2(sp)
    sw   a3, RV_STK_A3(sp)
    sw   a4, RV_STK_A4(sp)
    sw   a5, RV_STK_A5(sp)
    sw   a6, RV_STK_A6(sp)
    sw   a7, RV_STK_A7(sp)
    sw   s2, RV_STK_S2(sp)
    sw   s3, RV_STK_S3(sp)
    sw   s4, RV_STK_S4(sp)
    sw   s5, RV_STK_S5(sp)
    sw   s6, RV_STK_S6(sp)
    sw   s7, RV_STK_S7(sp)
    sw   s8, RV_STK_S8(sp)
    sw   s9, RV_STK_S9(sp)
    sw   s10, RV_STK_S10(sp)
    sw   s11, RV_STK_S11(sp)
    sw   t3, RV_STK_T3(sp)
    sw   t4, RV_STK_T4(sp)
    sw   t5, RV_STK_T5(sp)
    sw   t6, RV_STK_T6(sp)
.endm

.macro restore_general_regs cxt_size=CONTEXT_SIZE
    lw   ra, RV_STK_RA(sp)
    lw   tp, RV_STK_TP(sp)
    lw   t0, RV_STK_T0(sp)
    lw   t1, RV_STK_T1(sp)
    lw   t2, RV_STK_T2(sp)
    lw   s0, RV_STK_S0(sp)
    lw   s1, RV_STK_S1(sp)
    lw   a0, RV_STK_A0(sp)
    lw   a1, RV_STK_A1(sp)
    lw   a2, RV_STK_A2(sp)
    lw   a3, RV_STK_A3(sp)
    lw   a4, RV_STK_A4(sp)
    lw   a5, RV_STK_A5(sp)
    lw   a6, RV_STK_A6(sp)
    lw   a7, RV_STK_A7(sp)
    lw   s2, RV_STK_S2(sp)
    lw   s3, RV_STK_S3(sp)
    lw   s4, RV_STK_S4(sp)
    lw   s5, RV_STK_S5(sp)
    lw   s6, RV_STK_S6(sp)
    lw   s7, RV_STK_S7(sp)
    lw   s8, RV_STK_S8(sp)
    lw   s9, RV_STK_S9(sp)
    lw   s10, RV_STK_S10(sp)
    lw   s11, RV_STK_S11(sp)
    lw   t3, RV_STK_T3(sp)
    lw   t4, RV_STK_T4(sp)
    lw   t5, RV_STK_T5(sp)
    lw   t6, RV_STK_T6(sp)
    addi sp, sp, \cxt_size
.endm

#if SOC_CPU_HAS_FPU
.macro fpu_save_regs frame=sp
    fsw     ft0,  RV_FPU_FT0(\frame)
    fsw     ft1,  RV_FPU_FT1(\frame)
    fsw     ft2,  RV_FPU_FT2(\frame)
    fsw     ft3,  RV_FPU_FT3(\frame)
    fsw     ft4,  RV_FPU_FT4(\frame)
    fsw     ft5,  RV_FPU_FT5(\frame)
    fsw     ft6,  RV_FPU_FT6(\frame)
    fsw     ft7,  RV_FPU_FT7(\frame)
    fsw     fs0,  RV_FPU_FS0(\frame)
    fsw     fs1,  RV_FPU_FS1(\frame)
    fsw     fa0,  RV_FPU_FA0(\frame)
    fsw     fa1,  RV_FPU_FA1(\frame)
    fsw     fa2,  RV_FPU_FA2(\frame)
    fsw     fa3,  RV_FPU_FA3(\frame)
    fsw     fa4,  RV_FPU_FA4(\frame)
    fsw     fa5,  RV_FPU_FA5(\frame)
    fsw     fa6,  RV_FPU_FA6(\frame)
    fsw     fa7,  RV_FPU_FA7(\frame)
    fsw     fs2,  RV_FPU_FS2(\frame)
    fsw     fs3,  RV_FPU_FS3(\frame)
    fsw     fs4,  RV_FPU_FS4(\frame)
    fsw     fs5,  RV_FPU_FS5(\frame)
    fsw     fs6,  RV_FPU_FS6(\frame)
    fsw     fs7,  RV_FPU_FS7(\frame)
    fsw     fs8,  RV_FPU_FS8(\frame)
    fsw     fs9,  RV_FPU_FS9(\frame)
    fsw     fs10, RV_FPU_FS10(\frame)
    fsw     fs11, RV_FPU_FS11(\frame)
    fsw     ft8,  RV_FPU_FT8 (\frame)
    fsw     ft9,  RV_FPU_FT9 (\frame)
    fsw     ft10, RV_FPU_FT10(\frame)
    fsw     ft11, RV_FPU_FT11(\frame)
    csrr    a1,   fcsr
    sw      a1,   RV_FPU_FCSR(\frame)
.endm

.macro fpu_restore_regs frame=sp
    flw     ft0,  RV_FPU_FT0(\frame)
    flw     ft1,  RV_FPU_FT1(\frame)
    flw     ft2,  RV_FPU_FT2(\frame)
    flw     ft3,  RV_FPU_FT3(\frame)
    flw     ft4,  RV_FPU_FT4(\frame)
    flw     ft5,  RV_FPU_FT5(\frame)
    flw     ft6,  RV_FPU_FT6(\frame)
    flw     ft7,  RV_FPU_FT7(\frame)
    flw     fs0,  RV_FPU_FS0(\frame)
    flw     fs1,  RV_FPU_FS1(\frame)
    flw     fa0,  RV_FPU_FA0(\frame)
    flw     fa1,  RV_FPU_FA1(\frame)
    flw     fa2,  RV_FPU_FA2(\frame)
    flw     fa3,  RV_FPU_FA3(\frame)
    flw     fa4,  RV_FPU_FA4(\frame)
    flw     fa5,  RV_FPU_FA5(\frame)
    flw     fa6,  RV_FPU_FA6(\frame)
    flw     fa7,  RV_FPU_FA7(\frame)
    flw     fs2,  RV_FPU_FS2(\frame)
    flw     fs3,  RV_FPU_FS3(\frame)
    flw     fs4,  RV_FPU_FS4(\frame)
    flw     fs5,  RV_FPU_FS5(\frame)
    flw     fs6,  RV_FPU_FS6(\frame)
    flw     fs7,  RV_FPU_FS7(\frame)
    flw     fs8,  RV_FPU_FS8(\frame)
    flw     fs9,  RV_FPU_FS9(\frame)
    flw     fs10, RV_FPU_FS10(\frame)
    flw     fs11, RV_FPU_FS11(\frame)
    flw     ft8,  RV_FPU_FT8(\frame)
    flw     ft9,  RV_FPU_FT9(\frame)
    flw     ft10, RV_FPU_FT10(\frame)
    flw     ft11, RV_FPU_FT11(\frame)
    lw      a1,   RV_FPU_FCSR(\frame)
    csrw    fcsr, a1
.endm

/* Set \reg to mstatus.FS (non-zero when FPU is not Off). */
.macro fpu_is_enabled reg
    csrr    \reg, mstatus
    srli    \reg, \reg, 13
    andi    \reg, \reg, 0x3
.endm

#endif  /* SOC_CPU_HAS_FPU */

#if SOC_CPU_HAS_DSP
.macro dsp_save_regs frame=sp
    csrr    a1,   CSR_DSP_XACC_L
    sw      a1,   RV_DSP_XACC_L(\frame)
    csrr    a1,   CSR_DSP_XACC_H
    sw      a1,   RV_DSP_XACC_H(\frame)
    csrr    a1,   CSR_DSP_SAR
    sw      a1,   RV_DSP_SAR(\frame)
    csrr    a1,   CSR_DSP_STATUS
    sw      a1,   RV_DSP_STATUS(\frame)
.endm

.macro dsp_restore_regs frame=sp
    lw      a1,   RV_DSP_XACC_L(\frame)
    csrw    CSR_DSP_XACC_L, a1
    lw      a1,   RV_DSP_XACC_H(\frame)
    csrw    CSR_DSP_XACC_H, a1
    lw      a1,   RV_DSP_SAR(\frame)
    csrw    CSR_DSP_SAR, a1
    lw      a1,   RV_DSP_STATUS(\frame)
    csrw    CSR_DSP_STATUS, a1
.endm

.macro dsp_is_enabled reg
    csrr    \reg, CSR_DSP_STATE_REG
    andi    \reg, \reg, 0x3
.endm
#endif /* SOC_CPU_HAS_DSP */

.macro save_mepc
    csrr t0, mepc
    sw   t0, RV_STK_MEPC(sp)
.endm

.macro restore_mepc
    lw   t0, RV_STK_MEPC(sp)
    csrw mepc, t0
.endm

    /*
     * Non-hardware vectored entry. MTVEC CSR points here.
     * Must be 64-byte aligned (MTVEC[31:6] << 6).
     */
    .global _vector_table
    .section .exception_vectors.text
    .balign 0x40
    .type _vector_table, @function
_vector_table:
    .option push
    .option norvc
    j _panic_handler
    .option pop
    .size _vector_table, .-_vector_table

    /*
     * Hardware vectored interrupt table. MTVT CSR points here.
     * CPU jumps to MTVT + 4 * interrupt_id for vectored interrupts.
     */
    .global _mtvt_table
    .section .exception_vectors_table.text
    .balign 0x40
    .type _mtvt_table, @function
_mtvt_table:
    .option push
    .option norvc
    .word _system_int_handler       /* 0 */
    .word _system_int_handler       /* 1 */
    .word _system_int_handler       /* 2 */
    .word _system_int_handler       /* 3 */
    .word _system_int_handler       /* 4 */
    .word _system_int_handler       /* 5 */
    .word _system_int_handler       /* 6 */
    .word _system_int_handler       /* 7 */
    .word _system_int_handler       /* 8 */
    .word _system_int_handler       /* 9 */
    .word _system_int_handler       /* 10 */
    .word _system_int_handler       /* 11 */
    .word _system_int_handler       /* 12 */
    .word _system_int_handler       /* 13 */
    .word _system_int_handler       /* 14 */
    .word _system_int_handler       /* 15 */
    .word _interrupt_handler        /* 16 */
    .word _interrupt_handler        /* 17 */
    .word _interrupt_handler        /* 18 */
    .word _interrupt_handler        /* 19 */
    .word _interrupt_handler        /* 20 */
    .word _interrupt_handler        /* 21 */
    .word _interrupt_handler        /* 22 */
    .word _interrupt_handler        /* 23 */
    .word _interrupt_handler        /* 24 */
    .word _interrupt_handler        /* 25 */
    .word _interrupt_handler        /* 26 */
    .word _interrupt_handler        /* 27 */
    .word _interrupt_handler        /* 28 */
    .word _interrupt_handler        /* 29 */
    .word _interrupt_handler        /* 30 */
    .word _interrupt_handler        /* 31 */
    .word _interrupt_handler        /* 32 */
    .word _interrupt_handler        /* 33 */
    .word _interrupt_handler        /* 34 */
    .word _interrupt_handler        /* 35 */
    .word _interrupt_handler        /* 36 */
    .word _interrupt_handler        /* 37 */
    .word _interrupt_handler        /* 38 */
    .word _interrupt_handler        /* 39 */
    .word _panic_handler            /* 40: ETS_INT_WDT_INUM (+16) */
    .word _panic_handler            /* 41: ETS_CACHEERR_INUM (+16) */
    .word MEMPROT_ISR               /* 42: ETS_MEMPROT_ERR_INUM (+16) */
    .word ASTDBG_ISR                /* 43: ETS_ASSIST_DEBUG_INUM (+16) */
    .word IPC_ISR_HANDLER           /* 44: ETS_IPC_ISR_INUM (+16) */
    .word _interrupt_handler        /* 45 */
    .word _interrupt_handler        /* 46 */
    .word _interrupt_handler        /* 47 */
    .option pop
    .size _mtvt_table, .-_mtvt_table

    .global _panic_handler
    .type _panic_handler, @function
_panic_handler:
    /*
     * CLIC routes non-vectored interrupts to mtvec.BASE. Check mcause bit 31
     * before touching the stack: if set, this is an interrupt, not an exception.
     */
    csrw mscratch, t0
    csrr t0, mcause
    bltz t0, _clic_interrupt_redirect
    csrr t0, mscratch

    save_general_regs RV_STK_FRMSZ
    save_mepc

    addi t0, sp, RV_STK_FRMSZ
    sw   t0, RV_STK_SP(sp)

    sw   gp, RV_STK_GP(sp)

    csrr t0, mstatus
    sw   t0, RV_STK_MSTATUS(sp)
    csrr t0, mtvec
    sw   t0, RV_STK_MTVEC(sp)
    csrr t0, mcause
    sw   t0, RV_STK_MCAUSE(sp)

#if SOC_CPU_COPROC_NUM > 0
    /* Capture coprocessor illegal-instruction reason before it is lost. */
    li   s4, 0
    andi t0, t0, 0x1f
    li   t1, 2
    bne  t0, t1, _panic_ext_ill_done
    csrrw s4, EXT_ILL_CSR, zero
_panic_ext_ill_done:
#endif

    csrr t0, mtval
    sw   t0, RV_STK_MTVAL(sp)
    csrr t0, mhartid
    sw   t0, RV_STK_MHARTID(sp)

    mv   a0, sp
#if SOC_CPU_COPROC_NUM > 0
    mv   a1, s4               /* EXT_ILL CSR value at trap (0 if not illegal insn) */
#endif
    la   t0, panic_handler_c
    jalr t0

1:  j    1b
    .size _panic_handler, .-_panic_handler

_clic_interrupt_redirect:
    csrr t0, mscratch
    j    _interrupt_handler

    .global _interrupt_handler
    .global _global_interrupt_handler
    .weak _interrupt_handler
    .type _interrupt_handler, @function
_interrupt_handler:
    save_general_regs
    save_mepc

    /* Check if FPU is enabled before calling fpu_save_regs */

#if SOC_CPU_HAS_FPU
    li      s3, 0
    fpu_is_enabled t0
    beqz    t0, _isr_fpu_save_done
    addi    s3, sp, -RvFPUSaveAreaSize
    fpu_save_regs s3
_isr_fpu_save_done:
#endif

#if SOC_CPU_HAS_DSP
    li      s6, 0
    dsp_is_enabled t1
    beqz    t1, _isr_dsp_save_done
    addi    t0, t0, -RvDSPSaveAreaSize
    mv      s6, t0
    dsp_save_regs s6
_isr_dsp_save_done:
#endif

    csrr    s1, mcause
    csrr    s2, mstatus

    mv      a0, sp
    mv      a1, s1
    li      t0, 0x00000fff
    and     a1, a1, t0
    jal     _global_interrupt_handler

    /* CLIC restores interrupt priority from mcause on mret. */
    csrw    mcause, s1

#if SOC_CPU_HAS_DSP
    beqz    s6, _isr_dsp_restore_done
    dsp_restore_regs s6
_isr_dsp_restore_done:
#endif

#if SOC_CPU_HAS_FPU
    beqz    s3, _isr_fpu_restore_done
    fpu_restore_regs s3
_isr_fpu_restore_done:
#endif

    restore_mepc
    restore_general_regs

    mret
    .size _interrupt_handler, .-_interrupt_handler

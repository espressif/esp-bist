/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * This file is part of Espressif's BIST (Built-In Self Test) Library.
 *
 * BIST library is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * BIST library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with BIST library. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "bist_cpu_csr_regs.h"
#include "soc/soc_caps.h"

#define MASK_32BIT      0xFFFFFFFF
#define CSR_MTVEC_MASK  0xFFFFFF00
#define CSR_MEPC_MASK   0xFFFFFFFE
#define CSR_MCAUSE_MASK 0x8000001F

/* Per 8-bit PMP entry: exclude Lock (bit 7), reserved (bits 6:5), and
 * W (bit 1).  W must be excluded because the 0xAA test pattern sets
 * W=1, R=0 which is a reserved encoding in the RISC-V spec — hardware
 * WARL-clears W when R=0.
 * C5 additionally excludes bit 4 (upper A-field bit): with G=5 (128-byte
 * granularity), A=NA4 is not selectable, so the 0x55 pattern (A=10)
 * would WARL to A=00. */
#if defined(SOC_TARGET_ESP32C5) || defined(SOC_TARGET_ESP32C61)
#define CSR_PMPCFG_MASK 0x0D0D0D0D
#else
#define CSR_PMPCFG_MASK 0x1D1D1D1D
#endif

/* C5/C61 PMP have 128-byte granularity (bottom 5 bits hardwired 0, top 2 hardwired 0);
 * C3/C6/H2 have 4-byte granularity (full 32 bits writable) */
#if defined(SOC_TARGET_ESP32C5) || defined(SOC_TARGET_ESP32C61)
#define CSR_PMPADDR_MASK 0x3FFFFFE0
#else
#define CSR_PMPADDR_MASK MASK_32BIT
#endif

/* PMA addr: same physical width as PMP addr on C5 (25 writable bits) */
#define CSR_PMAADDR_MASK 0x3FFFFFE0

/* mexstatus: only safely writable bits -- PBEXE(10), PPBEXE(11), NMFT(13), CLIC_INHV(20) */
#define CSR_MEXSTATUS_MASK 0x00102C00

/* mhint: only SBE (bit 20) */
#define CSR_MHINT_MASK  0x00100000

#if defined(ESP_BIST_USE_FPU)
/* FPU CSRs (RV32F): fflags[4:0], frm[2:0], fcsr[7:0] */
#define CSR_FFLAGS_MASK 0x1F
#define CSR_FRM_MASK    0x07
#define CSR_FCSR_MASK   0xFF
#endif

#define CSR_GPIO_OEN_USER 0x803
#define CSR_GPIO_IN_USER  0x804
#define CSR_GPIO_OUT_USER 0x805

#define BIST_TEST_CSR_REG_NOT_STACKED(reg, mask, j_error)   \
    do {                                                    \
        ASM(" li  t0, 0xAAAAAAAA");                         \
        ASM(" li  t3, " #mask);                             \
        ASM(" and  t0, t0, t3");                            \
        ASM(" csrw " #reg ", t0");                          \
        ASM(" csrr t1, " #reg);                             \
        ASM(" and  t1, t1, t3");                            \
        ASM("testRegA_" #reg ": bne t1, t0, " #j_error);    \
        ASM(" li  t0, 0x55555555");                         \
        ASM(" and  t0, t0, t3");                            \
        ASM(" csrw " #reg ", t0");                          \
        ASM(" csrr t1, " #reg);                             \
        ASM(" and  t1, t1, t3");                            \
        ASM("testReg5_" #reg ":  bne t1, t0, " #j_error);   \
    }                                                       \
    while (0)

#define BIST_TEST_CSR_REG_STACKED(reg, mask, j_error)       \
    do {                                                    \
        ASM(" csrr t2," #reg);                              \
        ASM(" sw t2, 4(sp)");                               \
        BIST_TEST_CSR_REG_NOT_STACKED(reg, mask, j_error);  \
        ASM(" lw t2, 4(sp)");                               \
        ASM(" csrw " #reg ", t2");                          \
    }                                                       \
    while (0)

#if defined(CONFIG_ESP_BIST_CPU_CSR_REG_TEST)

__attribute__((naked))
bist_esp_err_t bist_cpu_csr_regs_test(void)
{
    ASM(" addi sp, sp, -16");
    // Machine Trap Setup CSRs
    BIST_TEST_CSR_REG_STACKED(mtvec, CSR_MTVEC_MASK, errorCSR);

    // Machine Trap Handling CSRs
    BIST_TEST_CSR_REG_STACKED(mscratch, MASK_32BIT, errorCSR);
    BIST_TEST_CSR_REG_STACKED(mepc, CSR_MEPC_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(mcause, CSR_MCAUSE_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(mtval, MASK_32BIT, errorCSR);

#if !defined(IS_ULP_COCPU)
    // Physical Memory Protection (PMP) address CSRs
    BIST_TEST_CSR_REG_STACKED(pmpaddr0, CSR_PMPADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr1, CSR_PMPADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr2, CSR_PMPADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr3, CSR_PMPADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr4, CSR_PMPADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr5, CSR_PMPADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr6, CSR_PMPADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr7, CSR_PMPADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr8, CSR_PMPADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr9, CSR_PMPADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr10, CSR_PMPADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr11, CSR_PMPADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr12, CSR_PMPADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr13, CSR_PMPADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr14, CSR_PMPADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr15, CSR_PMPADDR_MASK, errorCSR);

    // PMP configuration CSRs (mask excludes Lock and reserved bits)
    BIST_TEST_CSR_REG_STACKED(pmpcfg0, CSR_PMPCFG_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpcfg1, CSR_PMPCFG_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpcfg2, CSR_PMPCFG_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpcfg3, CSR_PMPCFG_MASK, errorCSR);
#endif

#if SOC_CPU_HAS_PMA && !defined(IS_ULP_COCPU)
    // PMA address CSRs (entries 0-11 only; 12-15 may be active NAPOT regions)
    BIST_TEST_CSR_REG_STACKED(0xBD0, CSR_PMAADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(0xBD1, CSR_PMAADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(0xBD2, CSR_PMAADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(0xBD3, CSR_PMAADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(0xBD4, CSR_PMAADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(0xBD5, CSR_PMAADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(0xBD6, CSR_PMAADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(0xBD7, CSR_PMAADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(0xBD8, CSR_PMAADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(0xBD9, CSR_PMAADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(0xBDA, CSR_PMAADDR_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(0xBDB, CSR_PMAADDR_MASK, errorCSR);
#endif

#if defined(SOC_TARGET_ESP32C5) && !defined(IS_ULP_COCPU)
    // mexstatus and mhint: C5-only (CLIC-capable core).
    // C6/H2 mexstatus only has SOFT_RST bits (unsafe to test); mhint absent.
    BIST_TEST_CSR_REG_STACKED(0x7E1, CSR_MEXSTATUS_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(0x7C5, CSR_MHINT_MASK, errorCSR);
#endif

#if defined(ESP_BIST_USE_FPU)
    // FPU CSRs (RV32F).
    BIST_TEST_CSR_REG_STACKED(fflags, CSR_FFLAGS_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(frm, CSR_FRM_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(fcsr, CSR_FCSR_MASK, errorCSR);
#endif

    ASM(" li a0, 0x0");
    ASM(" addi	sp,sp,16");
    ASM(" ret");

    ASM(" errorCSR: li a0, 0x2");
    ASM(" addi	sp,sp,16");
    ASM(" ret");
}

#endif // CONFIG_ESP_BIST_CPU_CSR_REG_TEST

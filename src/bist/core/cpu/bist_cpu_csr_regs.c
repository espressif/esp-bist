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

#define MASK_32BIT      0xFFFFFFFF
#define CSR_MTVEC_MASK  0xFFFFFF00
#define CSR_MEPC_MASK   0xFFFFFFFE
#define CSR_MCAUSE_MASK 0x8000001F

#define CSR_GPIO_OEN_USER 0x803
#define CSR_GPIO_IN_USER  0x804
#define CSR_GPIO_OUT_USER 0x805

#define BIST_TEST_CSR_REG_NOT_STACKED(reg, mask, j_error)                                                              \
    do {                                                                                                               \
        ASM(" li  t0, 0xAAAAAAAA");                                                                                    \
        ASM(" li  t3, " #mask);                                                                                        \
        ASM(" and  t0, t0, t3");                                                                                       \
        ASM(" csrw " #reg ", t0");                                                                                     \
        ASM(" csrr t1, " #reg);                                                                                        \
        ASM(" and  t1, t1, t3");                                                                                       \
        ASM("testRegA_" #reg ": bne t1, t0, " #j_error);                                                               \
        ASM(" li  t0, 0x55555555");                                                                                    \
        ASM(" and  t0, t0, t3");                                                                                       \
        ASM(" csrw " #reg ", t0");                                                                                     \
        ASM(" csrr t1, " #reg);                                                                                        \
        ASM(" and  t1, t1, t3");                                                                                       \
        ASM("testReg5_" #reg ":  bne t1, t0, " #j_error);                                                              \
    }                                                                                                                  \
    while (0)

#define BIST_TEST_CSR_REG_STACKED(reg, mask, j_error)                                                                  \
    do {                                                                                                               \
        ASM(" csrr t2," #reg);                                                                                         \
        ASM(" sw t2, 4(sp)");                                                                                          \
        BIST_TEST_CSR_REG_NOT_STACKED(reg, mask, j_error);                                                             \
        ASM(" lw t2, 4(sp)");                                                                                          \
        ASM(" csrw " #reg ", t2");                                                                                     \
    }                                                                                                                  \
    while (0)

bist_esp_err_t bist_cpu_csr_regs_test(void)
{
    // Machine Trap Setup CSRs
    BIST_TEST_CSR_REG_STACKED(mtvec, CSR_MTVEC_MASK, errorCSR);

    // Machine Trap Handling CSRs
    BIST_TEST_CSR_REG_STACKED(mscratch, MASK_32BIT, errorCSR);
    BIST_TEST_CSR_REG_STACKED(mepc, CSR_MEPC_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(mcause, CSR_MCAUSE_MASK, errorCSR);
    BIST_TEST_CSR_REG_STACKED(mtval, MASK_32BIT, errorCSR);

    // Physical Memory Protection (PMP) CSRs
    BIST_TEST_CSR_REG_STACKED(pmpaddr0, MASK_32BIT, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr1, MASK_32BIT, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr2, MASK_32BIT, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr3, MASK_32BIT, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr4, MASK_32BIT, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr5, MASK_32BIT, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr6, MASK_32BIT, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr7, MASK_32BIT, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr8, MASK_32BIT, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr9, MASK_32BIT, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr10, MASK_32BIT, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr11, MASK_32BIT, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr12, MASK_32BIT, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr13, MASK_32BIT, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr14, MASK_32BIT, errorCSR);
    BIST_TEST_CSR_REG_STACKED(pmpaddr15, MASK_32BIT, errorCSR);

    ASM(" li a0, 0x0");
    ASM(" addi	sp,sp,16");
    ASM(" ret");

    ASM(" errorCSR: li a0, 0x2");
    ASM(" addi	sp,sp,16");
    ASM(" ret");
    return BIST_ESP_CPU_CSR_TEST_ERR;
}

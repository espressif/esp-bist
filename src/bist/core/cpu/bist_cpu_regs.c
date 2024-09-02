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

#include "bist_cpu_regs.h"

/**
 * The lui instruction loads the immediate value into the upper 20 bits of the destination register.
 * To fill all 32 bits of the register, we need to perform some shifts operations.
 */
#define BIST_TEST_CPU_REG_NOT_STACKED(reg, temp_reg, j_error)                                                          \
    do {                                                                                                               \
        ASM(" li " #reg ", 0xAAAAAAAA");                                                                               \
        ASM(" li " #temp_reg ", 0xAAAAAAAA");                                                                          \
        ASM("testRegA_" #reg ": bne " #reg ", " #temp_reg ", " #j_error);                                              \
        ASM(" li " #reg ", 0x55555555");                                                                               \
        ASM(" li " #temp_reg ", 0x55555555");                                                                          \
        ASM("testReg5_" #reg ":  bne " #reg ", " #temp_reg ", " #j_error);                                             \
    }                                                                                                                  \
    while (0)

#define BIST_TEST_CPU_REG_STACKED(reg, temp_reg, j_error)                                                              \
    do {                                                                                                               \
        ASM(" sw " #reg ", 4(sp)");                                                                                    \
        BIST_TEST_CPU_REG_NOT_STACKED(reg, temp_reg, j_error);                                                         \
        ASM(" lw " #reg ", 4(sp)");                                                                                    \
    }                                                                                                                  \
    while (0)

/*
 * RISC-V Register Mnemonics and Roles
 *
 * ========================
 * |  x Registers (Integer Registers)   |
 * ========================
 * | Register | ABI Name | Role                |
 * |----------|----------|---------------------|
 * |    x0    |   zero   | Hardwired zero      |
 * |    x1    |   ra     | Return address      |
 * |    x2    |   sp     | Stack pointer       |
 * |    x3    |   gp     | Global pointer      |
 * |    x4    |   tp     | Thread pointer      |
 * |    x5    |   t0     | Temporary/scratch   |
 * |    x6-x7 |   t1-t2  | Temporary/scratch   |
 * |    x8    |   s0/fp  | Saved frame pointer |
 * |    x9    |   s1     | Saved register      |
 * |    x10-x11|  a0-a1  | Function arguments  |
 * |    x12-x17|  a2-a7  | Function arguments/return values |
 * |    x18-x27|  s2-s11 | Saved registers     |
 * |    x28    |   t3     | Temporary/scratch   |
 * |    x29    |   t4     | Temporary/scratch   |
 * |    x30    |   t5     | Temporary/scratch   |
 * |    x31    |   t6     | Temporary/scratch   |
 * ========================
 */

bist_esp_err_t bist_cpu_regs_test(void)
{
    // t0 test
    BIST_TEST_CPU_REG_NOT_STACKED(t0, t1, errorCPU);
    // t1 test
    BIST_TEST_CPU_REG_NOT_STACKED(t1, t0, errorCPU);
    // t2 test
    BIST_TEST_CPU_REG_NOT_STACKED(t2, t0, errorCPU);
    // t3 test
    BIST_TEST_CPU_REG_NOT_STACKED(t3, t0, errorCPU);
    //  t4 test
    BIST_TEST_CPU_REG_NOT_STACKED(t4, t0, errorCPU);
    //  t5 test
    BIST_TEST_CPU_REG_NOT_STACKED(t5, t0, errorCPU);
    //  t6 test
    BIST_TEST_CPU_REG_NOT_STACKED(t6, t0, errorCPU);
    // Push Ra to stack so we can return in case it fails
    ASM(" sw ra, 0(sp)");
    // ra test
    BIST_TEST_CPU_REG_STACKED(ra, t0, restoreRA);
    // sp test
    // Push SP to temp so we can restore it later
    ASM(" mv t3, sp");
    BIST_TEST_CPU_REG_NOT_STACKED(sp, t0, restoreSP);
    // Restore SP and continue
    ASM(" mv sp, t3");
    // gp test
    ASM(" mv t3, gp");
    BIST_TEST_CPU_REG_STACKED(gp, t0, restoreGP);
    // Restore GP and continue
    ASM(" mv gp, t3");
    // tp test
    BIST_TEST_CPU_REG_STACKED(tp, t0, errorCPU);
    // s0 test
    BIST_TEST_CPU_REG_STACKED(s0, t0, errorCPU);
    // s1 test
    BIST_TEST_CPU_REG_STACKED(s1, t0, errorCPU);
    // s2 test
    BIST_TEST_CPU_REG_STACKED(s2, t0, errorCPU);
    // s3 test
    BIST_TEST_CPU_REG_STACKED(s3, t0, errorCPU);
    // s4 test
    BIST_TEST_CPU_REG_STACKED(s4, t0, errorCPU);
    // s5 test
    BIST_TEST_CPU_REG_STACKED(s5, t0, errorCPU);
    // s6 test
    BIST_TEST_CPU_REG_STACKED(s6, t0, errorCPU);
    // s7 test
    BIST_TEST_CPU_REG_STACKED(s7, t0, errorCPU);
    // s8 test
    BIST_TEST_CPU_REG_STACKED(s8, t0, errorCPU);
    // s9 test
    BIST_TEST_CPU_REG_STACKED(s9, t0, errorCPU);
    // s10 test
    BIST_TEST_CPU_REG_STACKED(s10, t0, errorCPU);
    // s11 test
    BIST_TEST_CPU_REG_STACKED(s11, t0, errorCPU);
    // a0 test
    BIST_TEST_CPU_REG_NOT_STACKED(a0, t0, errorCPU);
    // a1 test
    BIST_TEST_CPU_REG_NOT_STACKED(a1, t0, errorCPU);
    // a2 test
    BIST_TEST_CPU_REG_NOT_STACKED(a2, t0, errorCPU);
    // a3 test
    BIST_TEST_CPU_REG_NOT_STACKED(a3, t0, errorCPU);
    // a4 test
    BIST_TEST_CPU_REG_NOT_STACKED(a4, t0, errorCPU);
    // a5 test
    BIST_TEST_CPU_REG_NOT_STACKED(a5, t0, errorCPU);
    // a6 test
    BIST_TEST_CPU_REG_NOT_STACKED(a6, t0, errorCPU);
    // a7 test
    BIST_TEST_CPU_REG_NOT_STACKED(a7, t0, errorCPU);

    // Success
    ASM(" li a0, 0x0");
    // Restore RA and SP and return
    ASM(" lw ra, 0(sp)");
    ASM(" addi	sp,sp,16");
    ASM(" ret");
    // ErrorCPU
    ASM(" restoreSP: mv sp, t3");
    ASM(" j errorCPU");
    ASM(" restoreRA: lw ra, 0(sp)");
    ASM(" j errorCPU");
    ASM(" restoreGP: mv gp, t3");
    ASM(" errorCPU: li a0, 0x1");
    return BIST_ESP_CPU_TEST_ERR;
}

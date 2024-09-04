
# Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0


set(CMAKE_SYSTEM_NAME Generic)
set(BIST_TOOLCHAIN_PATH ${BIST_ROOT_DIR}/tools/riscv32-esp-elf/bin)

set(CMAKE_C_COMPILER ${BIST_TOOLCHAIN_PATH}/riscv32-esp-elf-gcc)
set(CMAKE_CXX_COMPILER ${BIST_TOOLCHAIN_PATH}/riscv32-esp-elf-g++)
set(CMAKE_ASM_COMPILER ${BIST_TOOLCHAIN_PATH}/riscv32-esp-elf-gcc)
# set(CMAKE_OBJCOPY ${BIST_TOOLCHAIN_PATH}/riscv32-esp-elf-objcopy)

set(CMAKE_C_FLAGS "-march=rv32imc_zicsr_zifencei" CACHE STRING "C Compiler Base Flags")
set(CMAKE_CXX_FLAGS "-march=rv32imc_zicsr_zifencei" CACHE STRING "C++ Compiler Base Flags")
set(CMAKE_EXE_LINKER_FLAGS "-nostartfiles -march=rv32imc_zicsr_zifencei --specs=nosys.specs" CACHE STRING "Linker Base Flags")

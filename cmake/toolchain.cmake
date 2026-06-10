# Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

set(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_C_COMPILER riscv32-esp-elf-gcc)
set(CMAKE_CXX_COMPILER riscv32-esp-elf-g++)
set(CMAKE_ASM_COMPILER riscv32-esp-elf-gcc)
set(CMAKE_OBJCOPY riscv32-esp-elf-objcopy)
set(CMAKE_OBJDUMP riscv32-esp-elf-objdump)

if(DEFINED ESP_BIST_USE_FPU)
set(CMAKE_C_FLAGS "-march=rv32imafcb_zicsr_zifencei -mabi=ilp32f" CACHE STRING "C Compiler Base Flags")
set(CMAKE_CXX_FLAGS "-march=rv32imafcb_zicsr_zifencei -mabi=ilp32f" CACHE STRING "C++ Compiler Base Flags")
set(CMAKE_EXE_LINKER_FLAGS "-nostartfiles -march=rv32imafcb_zicsr_zifencei -mabi=ilp32f --specs=nosys.specs" CACHE STRING "Linker Base Flags")
set(CMAKE_ASM_FLAGS "-march=rv32imafcb_zicsr_zifencei -mabi=ilp32f" CACHE STRING "Asm Compiler Base Flags")
else()
set(CMAKE_C_FLAGS "-march=rv32imc_zicsr_zifencei" CACHE STRING "C Compiler Base Flags")
set(CMAKE_CXX_FLAGS "-march=rv32imc_zicsr_zifencei" CACHE STRING "C++ Compiler Base Flags")
set(CMAKE_EXE_LINKER_FLAGS "-nostartfiles -march=rv32imc_zicsr_zifencei --specs=nosys.specs" CACHE STRING "Linker Base Flags")
endif()

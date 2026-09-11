# Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

set(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_C_COMPILER riscv32-esp-elf-gcc)
set(CMAKE_CXX_COMPILER riscv32-esp-elf-g++)
set(CMAKE_ASM_COMPILER riscv32-esp-elf-gcc)
set(CMAKE_OBJCOPY riscv32-esp-elf-objcopy)
set(CMAKE_OBJDUMP riscv32-esp-elf-objdump)

list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES SOC_TARGET)

# Keep these profiles aligned with ESP-IDF components/soc/project_include.cmake.
# They represent IDF defaults; P4 uses BIST's minimum supported revision 3.
if("${SOC_TARGET}" STREQUAL "esp32c3")
    set(_march "rv32imc_zicsr_zifencei")
elseif("${SOC_TARGET}" STREQUAL "esp32c5" OR
       "${SOC_TARGET}" STREQUAL "esp32c6" OR
       "${SOC_TARGET}" STREQUAL "esp32c61" OR
       "${SOC_TARGET}" STREQUAL "esp32h2")
    set(_march "rv32imac_zicsr_zifencei_zaamo_zalrsc")
elseif("${SOC_TARGET}" STREQUAL "esp32h4")
    set(_march "rv32imafcb_zicsr_zifencei_zaamo_zalrsc_zba_zbb_zbs_xespdsp")
    set(_mabi "ilp32f")
elseif("${SOC_TARGET}" STREQUAL "esp32p4")
    set(_march "rv32imafc_zicsr_zifencei_zaamo_zalrsc_xesploop_xespv")
    set(_mabi "ilp32f")
else()
    message(FATAL_ERROR "Unsupported RISC-V target: ${SOC_TARGET}")
endif()

set(_arch_flags "-march=${_march}")
# Match IDF: only set -mabi for FPU SoCs; otherwise use the compiler default (ilp32).
if(DEFINED _mabi)
    set(_arch_flags "${_arch_flags} -mabi=${_mabi}")
endif()

set(CMAKE_C_FLAGS "${_arch_flags}" CACHE STRING "C Compiler Base Flags" FORCE)
set(CMAKE_CXX_FLAGS "${_arch_flags}" CACHE STRING "C++ Compiler Base Flags" FORCE)
set(CMAKE_ASM_FLAGS "${_arch_flags}" CACHE STRING "Asm Compiler Base Flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS
    "-nostartfiles ${_arch_flags} --specs=nosys.specs"
    CACHE STRING "Linker Base Flags" FORCE
)

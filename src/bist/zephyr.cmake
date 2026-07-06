#  Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.

#  This file is part of Espressif's BIST (Built-In Self Test) Library.
#  BIST library is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
#  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#  BIST library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
#  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
#  You should have received a copy of the GNU Lesser General Public License along with BIST library. If not, see
#  <https://www.gnu.org/licenses/lgpl-3.0.html>.

if(CONFIG_ESP_BIST)
        message(STATUS "Espressif's Built-In Self Test (BIST) Library for Zephyr")

        string(TOUPPER "${CONFIG_SOC}" SOC_TARGET_UPPER)
        zephyr_compile_definitions(SOC_TARGET_${SOC_TARGET_UPPER} IS_ULP_COCPU)

        zephyr_include_directories(
                ulp_include
                include
                core/include
                core/cpu/include
                core/memory/include
                core/clock/include
                core/wdt/include
                core/io/include
                drivers/include
        )

        zephyr_library()

        zephyr_library_sources(drivers/lp_wdt.c)
        zephyr_library_sources_ifdef(CONFIG_ESP_BIST_CPU_REG_TEST core/cpu/bist_cpu_regs.c)
        zephyr_library_sources_ifdef(CONFIG_ESP_BIST_CPU_CSR_REG_TEST core/cpu/bist_cpu_csr_regs.c)
        zephyr_library_sources_ifdef(CONFIG_ESP_BIST_STACK_TEST core/cpu/bist_cpu_stack.c)
        zephyr_library_sources_ifdef(CONFIG_ESP_BIST_MEMORY_RAM_TEST core/memory/bist_ram.c)
        zephyr_library_sources_ifdef(CONFIG_ESP_BIST_MEMORY_FLASH_TEST core/memory/bist_flash.c)

        if(CONFIG_ESP_BIST_MEMORY_FLASH_TEST)
                set(BIST_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
                set_property(GLOBAL APPEND PROPERTY post_build_patch_elf_commands
                        COMMAND ${CMAKE_COMMAND} -E env OBJCOPY=${CMAKE_OBJCOPY}
                        ARGS ${Python3_EXECUTABLE} ${BIST_ROOT_DIR}/scripts/calculate_crc32.py
                             ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME} .text .crc_section_text
                        COMMAND ${CMAKE_COMMAND} -E env OBJCOPY=${CMAKE_OBJCOPY}
                        ARGS ${Python3_EXECUTABLE} ${BIST_ROOT_DIR}/scripts/calculate_crc32.py
                             ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME} .rodata .crc_section_data)
        endif()

        zephyr_compile_options(-Os)

        set(SOC_LINKER_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/../soc/${CONFIG_SOC}/ld/zephyr.ld" CACHE INTERNAL "Custom linker script for BIST")
endif()

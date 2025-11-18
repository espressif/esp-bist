#  Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.

#  This file is part of Espressif's BIST (Built-In Self Test) Library.
#  BIST library is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#  BIST library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
#  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#  You should have received a copy of the GNU General Public License along with BIST library. If not, see
#  <https://www.gnu.org/licenses/>.

if(CONFIG_ESP_BIST)
        message(STATUS "Espressif's Built-In Self Test (BIST) Library for Zephyr")

        string(TOUPPER "${CONFIG_SOC}" SOC_TARGET_UPPER)
        zephyr_compile_definitions(SOC_TARGET_${SOC_TARGET_UPPER})

        zephyr_include_directories(
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

        zephyr_library_sources(
                core/cpu/bist_cpu_regs.c
                core/cpu/bist_cpu_csr_regs.c
                core/memory/bist_ram.c
        )

        zephyr_compile_options(-O0)

        set(SOC_LINKER_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/../soc/${CONFIG_SOC}/ld/bist_lpcore.ld" CACHE INTERNAL "Custom linker script for BIST")
endif()

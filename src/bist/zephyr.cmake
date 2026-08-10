#  Copyright (c) 2024-2026 Espressif Systems (Shanghai) Co., Ltd.

#  This file is part of Espressif's BIST (Built-In Self Test) Library.
#  BIST library is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
#  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#  BIST library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
#  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
#  You should have received a copy of the GNU Lesser General Public License along with BIST library. If not, see
#  <https://www.gnu.org/licenses/lgpl-3.0.html>.

include(${CMAKE_CURRENT_LIST_DIR}/cmake/sources_stl.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/sources_hd.cmake)

set(_bist_is_lpcore FALSE)
if(CONFIG_SOC_ESP32C5_LPCORE OR CONFIG_SOC_ESP32C6_LPCORE OR CONFIG_SOC_ESP32P4_LPCORE)
        set(_bist_is_lpcore TRUE)
endif()

# CONFIG_ESP_BIST selects the STL (lpcore images define IS_ULP_COCPU).
# CONFIG_ESP_BIST_HOST_DIAGNOSTICS additionally selects the LP companion on
# lpcore images and the HP host agent on hpcore images.
if(NOT CONFIG_ESP_BIST AND NOT CONFIG_ESP_BIST_HOST_DIAGNOSTICS)
        return()
endif()

string(TOUPPER "${CONFIG_SOC}" SOC_TARGET_UPPER)
if(_bist_is_lpcore)
        zephyr_compile_definitions(SOC_TARGET_${SOC_TARGET_UPPER} IS_ULP_COCPU)
else()
        zephyr_compile_definitions(SOC_TARGET_${SOC_TARGET_UPPER})
endif()

set(_bist_incs "")
set(_bist_srcs "")

if(CONFIG_ESP_BIST)
        message(STATUS "Espressif's Built-In Self Test (BIST) Library for Zephyr")

        bist_include_dirs(_bist_incs)

        list(APPEND _bist_srcs drivers/lp_wdt.c)
        bist_collect_sources(_bist_stl_srcs)
        list(APPEND _bist_srcs ${_bist_stl_srcs})
endif()

if(CONFIG_ESP_BIST_HOST_DIAGNOSTICS)
        if(_bist_is_lpcore)
                message(STATUS "ESP-BIST Host Diagnostics: LP companion sources")
                bist_hd_collect_lp_sources("zephyr" _bist_hd_srcs _bist_hd_incs)
        else()
                message(STATUS "ESP-BIST Host Diagnostics: HP agent sources (no IS_ULP_COCPU)")
                bist_hd_collect_hp_sources("zephyr" _bist_hd_srcs _bist_hd_incs)
        endif()
        list(APPEND _bist_srcs ${_bist_hd_srcs})
        list(APPEND _bist_incs ${_bist_hd_incs})
endif()

list(REMOVE_DUPLICATES _bist_incs)
list(REMOVE_DUPLICATES _bist_srcs)

foreach(_inc IN LISTS _bist_incs)
        zephyr_include_directories(${_inc})
endforeach()

zephyr_library()
foreach(_src IN LISTS _bist_srcs)
        zephyr_library_sources(${_src})
endforeach()
if(CONFIG_ESP_BIST)
        # LP SRAM is ~16 KiB: keep the whole image size-optimized, not just the
        # BIST library.
        zephyr_compile_options(-Os)
else()
        zephyr_library_compile_options(-Os)
endif()

if(CONFIG_ESP_BIST)
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

        set(SOC_LINKER_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/../soc/${CONFIG_SOC}/ld/zephyr.ld" CACHE INTERNAL "Custom linker script for BIST")
endif()

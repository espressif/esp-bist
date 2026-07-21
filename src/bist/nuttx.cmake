# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# NuttX / LP-core build integration for ESP-BIST.
#
# Appends to ULP_APP_C_SRCS / ULP_APP_INCLUDES for esp_ulp.cmake, and
# provides bist_add_crc_postbuild() (IDF-equivalent) for CRC injection.

message(STATUS "Building Espressif's Built-In Self Test (BIST) Library for NuttX (LP-core)")

get_filename_component(ESP_BIST_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(BIST_DIR "${CMAKE_CURRENT_LIST_DIR}")

if(NOT CHIP_SERIES)
  string(REPLACE "\"" "" CHIP_SERIES "${CONFIG_ESPRESSIF_CHIP_SERIES}")
endif()

list(APPEND ULP_APP_C_SRCS "${BIST_DIR}/drivers/lp_wdt.c")

if(CONFIG_ESP_BIST_CPU_REG_TEST)
  list(APPEND ULP_APP_C_SRCS "${BIST_DIR}/core/cpu/bist_cpu_regs.c")
endif()
if(CONFIG_ESP_BIST_CPU_CSR_REG_TEST)
  list(APPEND ULP_APP_C_SRCS "${BIST_DIR}/core/cpu/bist_cpu_csr_regs.c")
endif()
if(CONFIG_ESP_BIST_STACK_TEST)
  list(APPEND ULP_APP_C_SRCS "${BIST_DIR}/core/cpu/bist_cpu_stack.c")
endif()
if(CONFIG_ESP_BIST_MEMORY_RAM_TEST)
  list(APPEND ULP_APP_C_SRCS "${BIST_DIR}/core/memory/bist_ram.c")
endif()
if(CONFIG_ESP_BIST_MEMORY_FLASH_TEST)
  list(APPEND ULP_APP_C_SRCS "${BIST_DIR}/core/memory/bist_flash.c")
endif()

list(
  APPEND
  ULP_APP_INCLUDES
  "${BIST_DIR}/ulp_include"
  "${BIST_DIR}/include"
  "${BIST_DIR}/core/include"
  "${BIST_DIR}/core/cpu/include"
  "${BIST_DIR}/core/memory/include"
  "${BIST_DIR}/core/clock/include"
  "${BIST_DIR}/core/wdt/include"
  "${BIST_DIR}/core/io/include"
  "${BIST_DIR}/core/interrupt/include"
  "${BIST_DIR}/drivers/include"
  "${ESP_BIST_ROOT}/src/soc/${CHIP_SERIES}/include")

string(TOUPPER "${CHIP_SERIES}" _bist_soc_upper)
list(APPEND ULP_EXTRA_DEFINES NUTTX_ESP_BIST_MODULE
     "SOC_TARGET_${_bist_soc_upper}")

set(ULP_APP_C_SRCS ${ULP_APP_C_SRCS} PARENT_SCOPE)
set(ULP_APP_INCLUDES ${ULP_APP_INCLUDES} PARENT_SCOPE)
set(ULP_EXTRA_DEFINES ${ULP_EXTRA_DEFINES} PARENT_SCOPE)
set(ULP_CUSTOM_SECTIONS_LD
    "${ESP_BIST_ROOT}/src/soc/${CHIP_SERIES}/ld/nuttx.ld" PARENT_SCOPE)

# --- Post-link CRC injection (flash integrity test) ---
#
# Same pattern as idf.cmake: expose bist_add_crc_postbuild() for the parent
# sample CMakeLists (which owns the ULP ELF custom command from esp_ulp.cmake).
# NuttX links via add_custom_command(OUTPUT), so we APPEND to that rule rather
# than using TARGET POST_BUILD.
#
# The function injects CRC32 checksums into the ELF:
#   .text    -> .crc_section_text  (code integrity)
#   .rodata  -> .crc_section_data  (read-only data integrity)
set(BIST_CRC_SCRIPT
    "${ESP_BIST_ROOT}/scripts/calculate_crc32.py" CACHE INTERNAL "")

function(bist_add_crc_postbuild elf_file)
  if(CONFIG_ESP_BIST_MEMORY_FLASH_TEST)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_custom_command(
      OUTPUT ${elf_file}
      APPEND
      COMMAND ${CMAKE_COMMAND} -E env OBJCOPY=${CMAKE_OBJCOPY}
              ${Python3_EXECUTABLE} ${BIST_CRC_SCRIPT} ${elf_file} .text
              .crc_section_text
      COMMAND ${CMAKE_COMMAND} -E env OBJCOPY=${CMAKE_OBJCOPY}
              ${Python3_EXECUTABLE} ${BIST_CRC_SCRIPT} ${elf_file} .rodata
              .crc_section_data
      COMMENT "Calculating CRC32 for LP .text/.rodata sections")
  endif()
endfunction()

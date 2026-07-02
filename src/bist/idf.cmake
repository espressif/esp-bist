#  Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.

#  This file is part of Espressif's BIST (Built-In Self Test) Library.
#  BIST library is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
#  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#  BIST library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
#  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
#  You should have received a copy of the GNU Lesser General Public License along with BIST library. If not, see
#  <https://www.gnu.org/licenses/lgpl-3.0.html>.

# IDF/LP-core build integration for ESP-BIST.
#
# CONFIG_* variables are provided by the parent build via SDKCONFIG_CMAKE
# (CMake) and sdkconfig.h (C), included through idf_include/bist_conf.h.

message("Building Espressif's Built-In Self Test (BIST) Library for ${SOC_TARGET} (IDF/LP-core)")

get_filename_component(_sdkconfig_dir ${SDKCONFIG_HEADER} DIRECTORY)

# --- Source selection ---

set(bist_src
    drivers/lp_wdt.c
)

if(CONFIG_ESP_BIST_CPU_REG_TEST)
    list(APPEND bist_src core/cpu/bist_cpu_regs.c)
endif()
if(CONFIG_ESP_BIST_CPU_CSR_REG_TEST)
    list(APPEND bist_src core/cpu/bist_cpu_csr_regs.c)
endif()
if(CONFIG_ESP_BIST_STACK_TEST)
    list(APPEND bist_src core/cpu/bist_cpu_stack.c)
endif()
if(CONFIG_ESP_BIST_MEMORY_RAM_TEST)
    list(APPEND bist_src core/memory/bist_ram.c)
endif()
if(CONFIG_ESP_BIST_MEMORY_FLASH_TEST)
    list(APPEND bist_src core/memory/bist_flash.c)
endif()

# --- Library ---
add_library(bist_esp ${bist_src})

string(TOUPPER "${SOC_TARGET}" SOC_TARGET_UPPER)
target_compile_definitions(bist_esp
    PUBLIC
    "SOC_TARGET_${SOC_TARGET_UPPER}"
    "IS_ULP_COCPU"
)

target_compile_definitions(bist_esp
    PRIVATE
    "SOC_MMU_PAGE_SIZE=CONFIG_MMU_PAGE_SIZE"
)

target_compile_options(bist_esp
    PRIVATE
    "-Wno-frame-address"
    "-Wall"
    "-Wextra"
    "-W"
    "-Wdeclaration-after-statement"
    "-Wwrite-strings"
    "-Wshadow"
    "-ffunction-sections"
    "-fdata-sections"
    "-fstrict-volatile-bitfields"
    "-Werror=all"
    "-Wno-error=unused-function"
    "-Wno-error=unused-but-set-variable"
    "-Wno-error=unused-variable"
    "-Wno-error=deprecated-declarations"
    "-Wno-unused-parameter"
    "-Wno-sign-compare"
    "-ggdb"
    "-Os"
    "-D_GNU_SOURCE"
    "-std=gnu17"
    "-Wno-old-style-declaration"
    "-Wno-implicit-int"
    "-Wno-declaration-after-statement"
)

target_include_directories(bist_esp
    PUBLIC
    ulp_include
    ${_sdkconfig_dir}
    include
    core/include
    core/cpu/include
    core/memory/include
    core/clock/include
    core/wdt/include
    core/io/include
    drivers/include
    ../soc/${SOC_TARGET}/include
    ${IDF_PATH}/components/riscv/include
    ${IDF_PATH}/components/esp_common/include
    ${IDF_PATH}/components/esp_rom/${SOC_TARGET}
    ${IDF_PATH}/components/esp_rom/${SOC_TARGET}/include
    ${IDF_PATH}/components/esp_rom/${SOC_TARGET}/include/${SOC_TARGET}
    ${IDF_PATH}/components/esp_rom/include
    ${IDF_PATH}/components/esp_rom/include/${SOC_TARGET}
    ${IDF_PATH}/components/esp_hw_support/port/${SOC_TARGET}/include
    ${IDF_PATH}/components/soc/include
    ${IDF_PATH}/components/soc/${SOC_TARGET}/include
    ${IDF_PATH}/components/soc/${SOC_TARGET}/register
    ${IDF_PATH}/components/log/include
    ${IDF_PATH}/components/esp_system/include
    ${IDF_PATH}/components/esp_hw_support/include
    ${IDF_PATH}/components/esp_hw_support/include/soc
    ${IDF_PATH}/components/hal/include
    ${IDF_PATH}/components/hal/${SOC_TARGET}/include
    ${IDF_PATH}/components/hal/platform_port/include
    ${IDF_PATH}/components/esp_hal_wdt/include
    ${IDF_PATH}/components/esp_hal_wdt/${SOC_TARGET}/include
    ${IDF_PATH}/components/esp_hal_gpio/include
    ${IDF_PATH}/components/esp_hal_timg/${SOC_TARGET}/include
    ${IDF_PATH}/components/ulp/lp_core/lp_core/include
    ${IDF_PATH}/components/ulp/lp_core/shared/include
)

target_link_libraries(bist_esp PUBLIC c)

set_target_properties(bist_esp PROPERTIES VERSION ${PROJECT_VERSION})

# --- Post-build CRC injection (flash integrity test) ---
#
# CMake's add_custom_command(TARGET ... POST_BUILD) requires the target to be
# defined in the SAME directory scope. Since idf.cmake is processed inside a
# subdirectory (via add_subdirectory), it cannot attach POST_BUILD commands to
# the ULP executable directly.
#
# The workaround is to expose bist_add_crc_postbuild() as a globally-visible
# function. The parent CMakeLists.txt (which owns the ULP target) calls it
# after linking. This is the IDF equivalent of Zephyr's
# extra_post_build_commands global property pattern.
#
# The function injects CRC32 checksums into the ELF:
#   .text    -> .crc_section_text  (code integrity)
#   .rodata  -> .crc_section_data  (read-only data integrity)
set(BIST_CRC_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/../../scripts/calculate_crc32.py" CACHE INTERNAL "")

function(bist_add_crc_postbuild target)
    if(CONFIG_ESP_BIST_MEMORY_FLASH_TEST)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E env OBJCOPY=${CMAKE_OBJCOPY}
            ${PYTHON} ${BIST_CRC_SCRIPT}
            $<TARGET_FILE:${target}> .text .crc_section_text
            COMMENT "Calculating CRC32 for LP .text section"
        )
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E env OBJCOPY=${CMAKE_OBJCOPY}
            ${PYTHON} ${BIST_CRC_SCRIPT}
            $<TARGET_FILE:${target}> .rodata .crc_section_data
            COMMENT "Calculating CRC32 for LP rodata section"
        )
    endif()
endfunction()

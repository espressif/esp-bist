#  Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.

#  This file is part of Espressif's BIST (Built-In Self Test) Library.
#  BIST library is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
#  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#  BIST library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
#  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
#  You should have received a copy of the GNU Lesser General Public License along with BIST library. If not, see
#  <https://www.gnu.org/licenses/lgpl-3.0.html>.

cmake_minimum_required(VERSION 3.22)
set(PROJECT_VERSION 1.0.0)
project(bist_esp VERSION 0.0.1 DESCRIPTION "Espressif's Built-In Self Test (BIST) Library")

if(DEFINED ZEPHYR_BASE)
        include(zephyr.cmake)
        return()
endif()

# SOC_TARGET must be defined
if(DEFINED IDF_TARGET)
    set(SOC_TARGET ${IDF_TARGET})
endif()
if(NOT DEFINED SOC_TARGET)
    message(FATAL_ERROR "SOC_TARGET not defined. Please set -DSOC_TARGET.")
endif()

# Load Kconfig and parse config so CONFIG_* variables are available below
include(${BIST_ROOT_DIR}/cmake/kconfig.cmake)

message("Building Espressif's Built-In Self Test (BIST) Library for ${SOC_TARGET}")

set(bist_src
    drivers/wdt.c
    drivers/gpio.c
    drivers/esp_timer.c
    )
if(SOC_TARGET STREQUAL "esp32c3")
    list(APPEND bist_src drivers/xt_wdt.c)
endif()
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
if(CONFIG_ESP_BIST_CLOCK_TEST)
    list(APPEND bist_src core/clock/bist_clock_fail.c)
endif()
if(CONFIG_ESP_BIST_GPIO_TEST)
    list(APPEND bist_src core/io/bist_gpio.c)
endif()
if(CONFIG_ESP_BIST_PROGRAM_COUNTER_TEST)
    list(APPEND bist_src core/cpu/bist_pc.c)
endif()
if(CONFIG_ESP_BIST_WATCHDOG_TEST)
    list(APPEND bist_src core/wdt/bist_wdt.c)
endif()

set(idf_srcs
    ${IDF_PATH}/components/log/src/log.c
    # ${IDF_PATH}/components/log/log_noos.c
    ${IDF_PATH}/components/soc/${SOC_TARGET}/gpio_periph.c
    ${IDF_PATH}/components/hal/systimer_hal.c
    ${IDF_PATH}/components/esp_hw_support/port/${SOC_TARGET}/systimer.c
    )

if(SOC_TARGET STREQUAL "esp32c3")
    list(APPEND idf_srcs
        ${IDF_PATH}/components/esp_hal_wdt/xt_wdt_hal.c
    )
endif()

add_library(bist_esp
    ${idf_srcs}
    ${bist_src}
)

# Ensure BIST header is generated before building (CONFIG_* were set when kconfig.cmake was included above)
add_dependencies(bist_esp bist_conf_h)

# Add the soc target definition to the build
string(TOUPPER "${SOC_TARGET}" SOC_TARGET_UPPER)
target_compile_definitions(bist_esp
    PUBLIC
    "SOC_TARGET_${SOC_TARGET_UPPER}"
)

target_compile_definitions(bist_esp
    PUBLIC
    "SOC_MMU_PAGE_SIZE=CONFIG_MMU_PAGE_SIZE"
)

set(CFLAGS
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
    "-O0"
    "-D_GNU_SOURCE"
    "-std=gnu17"
    "-Wno-old-style-declaration"
    "-Wno-implicit-int"
    "-Wno-declaration-after-statement"
    )

target_compile_options(
    bist_esp
    PUBLIC
    ${CFLAGS}
    )

target_include_directories(
    bist_esp PUBLIC
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
    ${IDF_PATH}/components/soc/include
    ${IDF_PATH}/components/soc/${SOC_TARGET}/include
    ${IDF_PATH}/components/soc/${SOC_TARGET}/register
    ${IDF_PATH}/components/log/include
    ${IDF_PATH}/components/esp_system/include
    ${IDF_PATH}/components/esp_hw_support/include
    ${IDF_PATH}/components/esp_hw_support/include/soc
    ${IDF_PATH}/components/hal/include
    ${IDF_PATH}/components/esp_hal_wdt/include
    ${IDF_PATH}/components/esp_hal_wdt/${SOC_TARGET}/include
    ${IDF_PATH}/components/esp_hal_gpio/include
    ${IDF_PATH}/components/esp_hal_gpio/${SOC_TARGET}/include
    ${IDF_PATH}/components/esp_hal_timg/${SOC_TARGET}/include
    ${IDF_PATH}/components/esp_hal_clock/${SOC_TARGET}/include
    ${IDF_PATH}/components/esp_hw_support/etm/include
    ${IDF_PATH}/components/esp_hw_support/port/${SOC_TARGET}/include
    ../../components/hal/${SOC_TARGET}/include
    ${IDF_PATH}/components/hal/${SOC_TARGET}/include
    ${IDF_PATH}/components/hal/platform_port/include
    ${CMAKE_CURRENT_BINARY_DIR}/include
    )

# Per-file compiler override for IDF source (see docs/en/software_safety_requirements.rst)
if(SOC_TARGET STREQUAL "esp32c3")
    set_source_files_properties(
        ${IDF_PATH}/components/esp_hal_wdt/xt_wdt_hal.c
        PROPERTIES COMPILE_OPTIONS "-fgnu89-inline"
        )
endif()

set_target_properties(bist_esp PROPERTIES VERSION ${PROJECT_VERSION})

target_link_libraries(bist_esp
    PUBLIC
    c
)

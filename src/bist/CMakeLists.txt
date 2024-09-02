#  Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.

#  This file is part of Espressif's BIST (Built-In Self Test) Library.
#  BIST library is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#  BIST library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
#  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#  You should have received a copy of the GNU General Public License along with BIST library. If not, see
#  <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.16)
set(PROJECT_VERSION 0.0.1)
project(bist_esp VERSION 0.0.1 DESCRIPTION "Espressif's Built-In Self Test (BIST) Library")

# SOC_TARGET must be defined
if(NOT DEFINED SOC_TARGET)
    message(FATAL_ERROR "SOC_TARGET not defined. Please set -DSOC_TARGET.")
endif()

message("Building Espressif's Built-In Self Test (BIST) Library for ${SOC_TARGET}")

set(bist_src
    drivers/xt_wdt.c
    drivers/wdt.c
    drivers/gpio.c
    core/cpu/bist_cpu_regs.c
    core/cpu/bist_cpu_csr_regs.c
    core/cpu/bist_cpu_stack.c
    core/memory/bist_ram.c
    core/memory/bist_flash.c
    core/cpu/bist_pc.c
    core/clock/bist_clock_fail.c
    core/wdt/bist_wdt.c
    )

set(idf_srcs
    ${IDF_PATH}/components/hal/xt_wdt_hal.c
    ${IDF_PATH}/components/log/log.c
    ${IDF_PATH}/components/log/log_noos.c
    ${IDF_PATH}/components/soc/${SOC_TARGET}/gpio_periph.c
    )

add_library(bist_esp
    ${idf_srcs}
    ${bist_src}
)

set(CFLAGS
    "-Wno-frame-address"
    "-Wall"
    "-Wextra"
    "-W"
    "-Wdeclaration-after-statement"
    "-Wwrite-strings"
    "-Wlogical-op"
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
    drivers/include
    ../soc/${SOC_TARGET}/include
    ${IDF_PATH}/components/riscv/include
    ${IDF_PATH}/components/esp_common/include
    ${IDF_PATH}/components/esp_rom/${SOC_TARGET}
    ${IDF_PATH}/components/esp_rom/include
    ${IDF_PATH}/components/esp_rom/include/${SOC_TARGET}
    ${IDF_PATH}/components/soc/include
    ${IDF_PATH}/components/soc/${SOC_TARGET}/include
    ${IDF_PATH}/components/log/include
    ${IDF_PATH}/components/esp_system/include
    ${IDF_PATH}/components/esp_hw_support/include
    ${IDF_PATH}/components/esp_hw_support/include/soc
    ${IDF_PATH}/components/hal/include
    ../../components/hal/${SOC_TARGET}/include
    ${IDF_PATH}/components/hal/platform_port/include
    )

set_target_properties(bist_esp PROPERTIES VERSION ${PROJECT_VERSION})

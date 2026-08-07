cmake_minimum_required(VERSION 3.22)

# Enable compile_commands.json by default unless the user explicitly sets it to OFF
if (NOT CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

if (NOT DEFINED SOC_TARGET)
    message(FATAL_ERROR "SOC_TARGET not defined. Please set -DSOC_TARGET.")
else()
    # Set the minimum revision for each supported chip
    if ("${SOC_TARGET}" STREQUAL "esp32c3")
        set(ESP_MIN_REVISION 3)
        set(BOOTLOADER_ADDR 0x0)
        set(APP_ADDR 0x20000)
    elseif ("${SOC_TARGET}" STREQUAL "esp32c6")
        set(ESP_MIN_REVISION 0)
        set(BOOTLOADER_ADDR 0x0)
        set(APP_ADDR 0x20000)
    elseif ("${SOC_TARGET}" STREQUAL "esp32h2")
        set(ESP_MIN_REVISION 0)
        set(BOOTLOADER_ADDR 0x0)
        set(APP_ADDR 0x20000)
    elseif ("${SOC_TARGET}" STREQUAL "esp32c5")
        set(ESP_MIN_REVISION 0)
        set(BOOTLOADER_ADDR 0x2000)
        set(APP_ADDR 0x20000)
    elseif ("${SOC_TARGET}" STREQUAL "esp32c61")
        set(ESP_MIN_REVISION 0)
        set(BOOTLOADER_ADDR 0x0)
        set(APP_ADDR 0x20000)
    elseif ("${SOC_TARGET}" STREQUAL "esp32p4")
        set(ESP_MIN_REVISION 3)
        set(BOOTLOADER_ADDR 0x2000)
        set(APP_ADDR 0x20000)
    elseif ("${SOC_TARGET}" STREQUAL "esp32h4")
        set(ESP_MIN_REVISION 0)
        set(BOOTLOADER_ADDR 0x2000)
        set(APP_ADDR 0x20000)
    else()
        message(FATAL_ERROR "Unsupported target ${SOC_TARGET}")
    endif()
endif()

if("${SOC_TARGET}" STREQUAL "esp32h4" OR "${SOC_TARGET}" STREQUAL "esp32p4")
    set(ESP_BIST_USE_FPU 1)
    message(STATUS "FPU enabled for ${SOC_TARGET}")
endif()

message("Building BIST project for ${SOC_TARGET}")

if (DEFINED ENV{IDF_PATH})
    set (IDF_PATH $ENV{IDF_PATH})
else()
    message(FATAL_ERROR "IDF_PATH environment variable is not set.")
endif()

if (DEFINED ENV{MCUBOOT_PATH})
    set(MCUBOOT_PATH $ENV{MCUBOOT_PATH})
    set(MCUBOOT_BIN_PATH "${MCUBOOT_PATH}/boot/espressif/build")
else()
    message(FATAL_ERROR "MCUBOOT_PATH environment variable is not set.")
endif()

# Find installed esptool, if not found falls to IDF's
find_program(ESPTOOL_COMMAND
    NAMES esptool esptool.py
    )
if ("${ESPTOOL_COMMAND}" MATCHES "ESPTOOL_COMMAND-NOTFOUND")
        set(esptool_path "${IDF_PATH}/components/esptool_py/esptool/esptool.py")
else()
    set(esptool_path "${ESPTOOL_COMMAND}")
endif()

# Find imgtool.
# Go with an explicitly installed imgtool first, falling
# back to mcuboot/scripts/imgtool.py.
find_program(IMGTOOL_COMMAND
    NAMES imgtool imgtool.py
    )
if ("${IMGTOOL_COMMAND}" MATCHES "IMGTOOL_COMMAND-NOTFOUND")
    set(imgtool_path "${MCUBOOT_PATH}/scripts/imgtool.py")
else()
    set(imgtool_path "${IMGTOOL_COMMAND}")
endif()

# **************************************************************************************************

if (NOT DEFINED APP_SOURCES)
    message(FATAL_ERROR "APP_SOURCES not defined. Please set APP_SOURCES before including project.cmake")
endif()

set(CMAKE_TOOLCHAIN_FILE ${BIST_ROOT_DIR}/cmake/toolchain.cmake)
include(${CMAKE_TOOLCHAIN_FILE})

set(APP_EXECUTABLE ${APP_NAME}.elf)

add_executable(${APP_EXECUTABLE} ${APP_SOURCES})

add_subdirectory(${BIST_ROOT_DIR}/src/bist ${CMAKE_BINARY_DIR}/bist)

# **************************************************************************************************
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
    "-Os"
    "-D_GNU_SOURCE"
    "-std=gnu17"
    "-Wno-old-style-declaration"
    "-Wno-implicit-int"
    "-Wno-declaration-after-statement"
    )

set(LDFLAGS
    "-Wno-frame-address"
    "-Wl,--cref"
    "-Wl,--Map=${APP_NAME}.map"
    "-fno-rtti"
    "-fno-lto"
    "-Wl,--gc-sections"
    "-Wl,--undefined=uxTopUsedPriority"
    "-Wl,--no-warn-rwx-segments"
    )

target_link_libraries(${APP_EXECUTABLE} PUBLIC bist_esp)
target_link_libraries(${APP_EXECUTABLE} PRIVATE m gcc gcov)

set(LINKER_SCRIPT ${BIST_ROOT_DIR}/src/soc/${SOC_TARGET}/ld/linker.ld)
set_property(TARGET ${APP_EXECUTABLE} PROPERTY LINK_DEPENDS ${LINKER_SCRIPT})

target_compile_options(
    ${APP_EXECUTABLE}
    PUBLIC
    ${CFLAGS}
    )

target_compile_options(
    ${APP_EXECUTABLE}
    PUBLIC
    $<$<COMPILE_LANGUAGE:C>:-include>
    $<$<COMPILE_LANGUAGE:C>:esp_assert.h>
    )

target_compile_definitions(
    ${APP_EXECUTABLE} PUBLIC
    -DUNITY_INCLUDE_CONFIG_H
)

# Target-specific includes, sources, linker script, and ROM LD files
include(${BIST_ROOT_DIR}/cmake/${SOC_TARGET}.cmake)

# Per-file warning suppression for third-party IDF sources (see docs/en/software_safety_requirements.rst)
set_source_files_properties(
    ${IDF_PATH}/components/esp_system/panic.c
    PROPERTIES COMPILE_OPTIONS "-Wno-shadow"
    )

# IDF v6.0's esp32c61/ocode_init.c is missing an explicit
# `#include "esp_attr.h"` and uses NOINLINE_ATTR / IRAM_ATTR.  Force-include
# the header for that single file until upstream IDF is fixed.
if ("${SOC_TARGET}" STREQUAL "esp32c61")
    set_source_files_properties(
        ${IDF_PATH}/components/esp_hw_support/port/${SOC_TARGET}/ocode_init.c
        PROPERTIES COMPILE_OPTIONS "-include;esp_attr.h"
        )
endif()

set(include_unity
    ${IDF_PATH}/components/unity/include
    ${IDF_PATH}/components/unity/unity/src
    )

target_include_directories(
    ${APP_EXECUTABLE}
    PUBLIC
    ${include_soc}
    ${include_hal}
    ${include_unity}
    ${CMAKE_CURRENT_BINARY_DIR}/include
    )

target_include_directories(
    ${APP_EXECUTABLE}
    SYSTEM PUBLIC
    ${IDF_PATH}/components/esp_hw_support/include/esp_private
    )

set(unity_srcs
    ${IDF_PATH}/components/unity/unity/src/unity.c
    ${IDF_PATH}/components/unity/unity_port_esp32.c
    )

target_sources(
    ${APP_EXECUTABLE}
    PUBLIC
    ${soc_srcs}
    ${idf_srcs}
    ${idf_ow_srcs}
    ${unity_srcs}
    )

get_directory_property(configs COMPILE_DEFINITIONS)
foreach(c ${configs})
    list(APPEND conf_defines "-D${c}")
endforeach()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ld")

set(ld_input ${LINKER_SCRIPT})
set(ld_output ${CMAKE_CURRENT_BINARY_DIR}/ld/linker.ld)
set(bist_conf ${CMAKE_CURRENT_BINARY_DIR}/bist/include)

add_custom_command(
    TARGET ${APP_EXECUTABLE} PRE_LINK
    COMMAND ${CMAKE_C_COMPILER} -x c -E -P -o ${ld_output} -I ${bist_conf} ${conf_defines} ${ld_input}
    COMMENT "Preprocessing linker scripts..."
    )

target_link_options(
    ${APP_EXECUTABLE}
    PUBLIC
    -T${CMAKE_CURRENT_BINARY_DIR}/ld/linker.ld
    ${rom_ld}
    ${LDFLAGS}
    )

# **************************************************************************************************
# Output disassembled file
add_custom_command(TARGET ${APP_EXECUTABLE} POST_BUILD
    COMMAND
    ${CMAKE_OBJDUMP}
    --disassemble-all ${APP_EXECUTABLE} > ${APP_NAME}.dis
    )

# # **************************************************************************************************
# Calculate Flash CRC
add_custom_command(TARGET ${APP_EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E env OBJCOPY=${CMAKE_OBJCOPY}
    python
    ${BIST_ROOT_DIR}/scripts/calculate_crc32.py
    ${APP_EXECUTABLE} .flash.text .crc_section_text
    )

add_custom_command(TARGET ${APP_EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E env OBJCOPY=${CMAKE_OBJCOPY}
    python
    ${BIST_ROOT_DIR}/scripts/calculate_crc32.py
    ${APP_EXECUTABLE} .flash.rodata .crc_section_data
    )

# **************************************************************************************************
# Convert to bin
add_custom_command(TARGET ${APP_EXECUTABLE} POST_BUILD
    COMMAND
    ${CMAKE_OBJCOPY}
    -O binary ${APP_EXECUTABLE} ${APP_NAME}.bin
    )

# Sign the image
add_custom_command(TARGET ${APP_EXECUTABLE} POST_BUILD
    COMMAND
    ${imgtool_path}
    sign --pad --confirm --pad-sig --align 4 -v 0 -H 32 -S 0x100000 ${APP_NAME}.bin ${APP_NAME}_signed.bin
    )

# Qemu image
if(EXISTS ${MCUBOOT_BIN_PATH}/mcuboot_${SOC_TARGET}.bin)
add_custom_command(TARGET ${APP_EXECUTABLE} POST_BUILD
    COMMAND
    ${esptool_path}
    --chip ${SOC_TARGET} merge_bin ${BOOTLOADER_ADDR}  ${MCUBOOT_BIN_PATH}/mcuboot_${SOC_TARGET}.bin ${APP_ADDR} ${APP_NAME}_signed.bin --fill-flash-size 4MB -o ${APP_NAME}_qemu_image.bin > /dev/null
    )
endif()

# Copy flasher_args.json to build folder
add_custom_command(TARGET ${APP_EXECUTABLE} POST_BUILD
    COMMAND
    ${CMAKE_COMMAND} -E copy ${BIST_ROOT_DIR}/scripts/flasher_args.json.in ${CMAKE_BINARY_DIR}/flasher_args.json.in
    COMMAND
    ${CMAKE_COMMAND}
    -DINPUT_FILE=${CMAKE_BINARY_DIR}/flasher_args.json.in
    -DOUTPUT_FILE=${CMAKE_BINARY_DIR}/flasher_args.json
    -DAPP_NAME=${APP_NAME}
    -DSOC_TARGET=${SOC_TARGET}
    -DAPP_ADDR=${APP_ADDR}
    -P ${BIST_ROOT_DIR}/cmake/flash_target.cmake
    )

# **************************************************************************************************
# Flash command

if (DEFINED ENV{ESPPORT})
    set (ESPPORT $ENV{ESPPORT})
else()
    set (ESPPORT /dev/ttyUSB0)
endif()

add_custom_target(flash DEPENDS ${APP_NAME}.bin)
add_custom_command(TARGET flash POST_BUILD
    USES_TERMINAL
    COMMAND
    ${esptool_path}
    -p ${ESPPORT} -b 460800 --before default-reset --after hard-reset
    --chip ${SOC_TARGET} write-flash
    --flash-mode dio --flash-size detect
    --flash-freq 40m ${APP_ADDR}
    ${APP_NAME}_signed.bin
    )

# Flash bootloader command
add_custom_target(flash_boot DEPENDS ${APP_NAME}.bin)
add_custom_command(TARGET flash_boot POST_BUILD
    USES_TERMINAL
    COMMAND
    ${esptool_path}
    -p ${ESPPORT} -b 460800 --before default-reset --after hard-reset
    --chip ${SOC_TARGET} write-flash
    --flash-mode dio --flash-size detect --force
    --flash-freq keep ${BOOTLOADER_ADDR}
    ${MCUBOOT_BIN_PATH}/mcuboot_${SOC_TARGET}.bin
    )

# **************************************************************************************************
# Monitor command
add_custom_target(monitor DEPENDS ${APP_NAME}.bin)
add_custom_command(TARGET monitor POST_BUILD
    USES_TERMINAL
    COMMAND
    python
    ${IDF_PATH}/tools/idf_monitor.py
    --port ${ESPPORT}
    ${APP_EXECUTABLE}
    )

# **************************************************************************************************
# Qemu Command
add_custom_target(qemu DEPENDS ${APP_NAME}_qemu_image.bin)
add_custom_command(TARGET qemu POST_BUILD
    USES_TERMINAL
    COMMAND
    qemu-system-riscv32 -nographic -icount 3 -machine ${SOC_TARGET}
    -drive file=${APP_NAME}_qemu_image.bin,if=mtd,format=raw
    )

# Qemu Debug Command
add_custom_target(qemu_debug DEPENDS ${APP_NAME}_qemu_image.bin)
add_custom_command(TARGET qemu_debug POST_BUILD
    USES_TERMINAL
    COMMAND
    qemu-system-riscv32 -s -S -nographic -icount 3 -machine ${SOC_TARGET}
    -drive file=${APP_NAME}_qemu_image.bin,if=mtd,format=raw
    )

# **************************************************************************************************
# Debug Commands

add_custom_target(debug DEPENDS ${APP_EXECUTABLE})
add_custom_command(TARGET debug POST_BUILD
    USES_TERMINAL
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND
    ${BIST_ROOT_DIR}/scripts/debug.sh
    ${APP_EXECUTABLE}
    )

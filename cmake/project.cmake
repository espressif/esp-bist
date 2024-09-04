cmake_minimum_required(VERSION 3.16)

enable_language(C ASM)

if (NOT DEFINED SOC_TARGET)
    message(FATAL_ERROR "SOC_TARGET not defined. Please set -DSOC_TARGET.")
else()
    # Set the minimum revision for each supported chip
    if ("${SOC_TARGET}" STREQUAL "esp32c3")
        set(ESP_MIN_REVISION 3)
    else()
        message(FATAL_ERROR "Unsupported target ${SOC_TARGET}")
    endif()
endif()

message("Building Critical Safety FW for ${SOC_TARGET}")

set(MODULES_PATH ${BIST_ROOT_DIR}/modules)
set(IDF_PATH ${MODULES_PATH}/esp-idf)
set(MCUBOOT_PATH ${MODULES_PATH}/mcuboot)
set(MCUBOOT_BIN_PATH ${MODULES_PATH}/mcuboot/boot/espressif/build)

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
add_subdirectory(${BIST_ROOT_DIR}/src/bist ${CMAKE_BINARY_DIR}/bist)
target_link_libraries(${APP_EXECUTABLE} PUBLIC bist_esp)

# **************************************************************************************************
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
    "-Os"
    "-D_GNU_SOURCE"
    "-std=gnu17"
    "-Wno-old-style-declaration"
    "-Wno-implicit-int"
    "-Wno-declaration-after-statement"
    )

set(LDFLAGS
    "-nostdlib"
    "-Wno-frame-address"
    "-Wl,--cref"
    "-Wl,--Map=${APP_NAME}.map"
    "-fno-rtti"
    "-fno-lto"
    "-Wl,--gc-sections"
    "-Wl,--undefined=uxTopUsedPriority"
    "-lm"
    "-lgcc"
    "-lgcov"
    "-Wl,--no-warn-rwx-segments"
    )

set(LINKER_SCRIPT ${BIST_ROOT_DIR}/src/soc/${SOC_TARGET}/ld/linker.ld)
set_property(TARGET ${APP_EXECUTABLE} PROPERTY LINK_DEPENDS ${LINKER_SCRIPT})

target_compile_options(
    ${APP_EXECUTABLE}
    PUBLIC
    ${CFLAGS}
    )

target_compile_definitions(
    ${APP_EXECUTABLE} PUBLIC
    -DUNITY_INCLUDE_CONFIG_H
)

set(include_soc
    ${BIST_ROOT_DIR}/src/soc/include
    ${BIST_ROOT_DIR}/src/soc/${SOC_TARGET}/include
    )

set(include_hal
    ${IDF_PATH}/components/newlib/platform_include
    ${IDF_PATH}/components/hal/include
    ${IDF_PATH}/components/hal/${SOC_TARGET}/include
    ${IDF_PATH}/components/hal/platform_port/include
    ${IDF_PATH}/components/hal/platform_port/include/hal
    ${BIST_ROOT_DIR}/components/esp_common/include
    ${IDF_PATH}/components/soc/include
    ${IDF_PATH}/components/soc/${SOC_TARGET}/include
    ${IDF_PATH}/components/esp_rom/${SOC_TARGET}
    ${IDF_PATH}/components/esp_rom/include
    ${IDF_PATH}/components/esp_rom/include/${SOC_TARGET}
    ${IDF_PATH}/components/riscv/include
    ${IDF_PATH}/components/esp_system/include
    ${IDF_PATH}/components/esp_system/port/include
    ${IDF_PATH}/components/esp_hw_support/include
    ${IDF_PATH}/components/esp_hw_support/port/include
    ${IDF_PATH}/components/esp_hw_support/include/soc
    ${IDF_PATH}/components/spi_flash/include
    ${IDF_PATH}/components/log/include
    )

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
    )

set(soc_srcs
    ${BIST_ROOT_DIR}/src/soc/${SOC_TARGET}/start.c
    ${BIST_ROOT_DIR}/src/soc/${SOC_TARGET}/vectors.S
    )

# IDF overwritten sources
set(idf_ow_srcs
    ${BIST_ROOT_DIR}/components/esp_hw_support/regi2c_ctrl.c
    ${BIST_ROOT_DIR}/components/esp_hw_support/port/${SOC_TARGET}/sar_periph_ctrl.c
    ${BIST_ROOT_DIR}/components/esp_hw_support/esp_clk.c
    ${BIST_ROOT_DIR}/components/esp_hw_support/periph_ctrl.c
    ${BIST_ROOT_DIR}/components/newlib/assert.c
    ${BIST_ROOT_DIR}/components/esp_system/port/esp_system_chip.c
)

set(idf_srcs
    ${IDF_PATH}/components/hal/cache_hal.c
    ${IDF_PATH}/components/hal/mmu_hal.c
    ${IDF_PATH}/components/hal/efuse_hal.c
    ${IDF_PATH}/components/hal/${SOC_TARGET}/efuse_hal.c
    ${IDF_PATH}/components/hal/wdt_hal_iram.c
    ${IDF_PATH}/components/esp_rom/patches/esp_rom_sys.c
    ${IDF_PATH}/components/esp_rom/patches/esp_rom_uart.c
    ${IDF_PATH}/components/esp_hw_support/port/${SOC_TARGET}/rtc_clk.c
    ${IDF_PATH}/components/esp_hw_support/port/${SOC_TARGET}/rtc_clk_init.c
    ${IDF_PATH}/components/esp_hw_support/port/${SOC_TARGET}/rtc_init.c
    ${IDF_PATH}/components/esp_hw_support/port/${SOC_TARGET}/rtc_sleep.c
    ${IDF_PATH}/components/esp_hw_support/port/${SOC_TARGET}/rtc_time.c
    ${IDF_PATH}/components/newlib/abort.c
    ${IDF_PATH}/components/esp_system/panic.c
    ${IDF_PATH}/components/esp_system/port/soc/${SOC_TARGET}/clk.c
    ${IDF_PATH}/components/log/log.c
    ${IDF_PATH}/components/log/log_noos.c
    ${IDF_PATH}/components/riscv/interrupt.c
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

add_custom_command(
    TARGET ${APP_EXECUTABLE} PRE_LINK
    COMMAND ${CMAKE_C_COMPILER} -x c -E -P -o ${CMAKE_CURRENT_BINARY_DIR}/ld/linker.ld ${conf_defines} ${BIST_ROOT_DIR}/src/soc/${SOC_TARGET}/ld/linker.ld
    MAIN_DEPENDENCY ${ld_input}
    COMMENT "Preprocessing linker scripts..."
    )

set(rom_ld
    -T${IDF_PATH}/components/esp_rom/${SOC_TARGET}/ld/${SOC_TARGET}.rom.ld
    -T${IDF_PATH}/components/esp_rom/${SOC_TARGET}/ld/${SOC_TARGET}.rom.api.ld
    -T${IDF_PATH}/components/esp_rom/${SOC_TARGET}/ld/${SOC_TARGET}.rom.newlib.ld
    -T${IDF_PATH}/components/soc/${SOC_TARGET}/ld/${SOC_TARGET}.peripherals.ld
    -T${IDF_PATH}/components/esp_rom/${SOC_TARGET}/ld/${SOC_TARGET}.rom.newlib-nano.ld
    -T${IDF_PATH}/components/esp_rom/${SOC_TARGET}/ld/${SOC_TARGET}.rom.libgcc.ld
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
    COMMAND
    python
    ${BIST_ROOT_DIR}/scripts/calculate_crc32.py
    ${APP_EXECUTABLE} .flash.text .crc_section_text
    )

add_custom_command(TARGET ${APP_EXECUTABLE} POST_BUILD
    COMMAND
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
add_custom_command(TARGET ${APP_EXECUTABLE} POST_BUILD
    COMMAND
    ${esptool_path}
    --chip ${SOC_TARGET} merge_bin 0x0  ${MCUBOOT_BIN_PATH}/mcuboot_${SOC_TARGET}.bin 0x10000 ${APP_NAME}_signed.bin --fill-flash-size 4MB -o ${APP_NAME}_qemu_image.bin > /dev/null
    )

# Copy flasher_args.json to build folder
add_custom_command(TARGET ${APP_EXECUTABLE} POST_BUILD
    COMMAND
    ${CMAKE_COMMAND} -E copy ${BIST_ROOT_DIR}/scripts/flasher_args.json ${CMAKE_BINARY_DIR}/flasher_args.json
    )

# **************************************************************************************************
# Flash command

if (DEFINED ENV{ESPPORT})
    set (ESPPORT $ENV{ESPPORT})
else()
    set (ESPPORT /dev/ttyUSB0)
endif()

add_custom_target(flash DEPENDS ${APP_NAME}.bin)
add_custom_command(TARGET flash
    USES_TERMINAL
    COMMAND
    ${esptool_path}
    -p ${ESPPORT} -b 460800 --before default_reset --after hard_reset
    --chip ${SOC_TARGET} write_flash
    --flash_mode dio --flash_size detect
    --flash_freq 40m 0x10000
    ${APP_NAME}_signed.bin
    )

# Flash bootloader command
add_custom_target(flash_boot DEPENDS ${APP_NAME}.bin)
add_custom_command(TARGET flash_boot
    USES_TERMINAL
    COMMAND
    ${esptool_path}
    -p ${ESPPORT} -b 460800 --before default_reset --after hard_reset
    --chip ${SOC_TARGET} write_flash
    --flash_mode dio --flash_size detect
    --flash_freq 40m 0x0
    ${MCUBOOT_BIN_PATH}/mcuboot_${SOC_TARGET}.bin
    )

# **************************************************************************************************
# Qemu Command
add_custom_target(qemu DEPENDS ${APP_NAME}_qemu_image.bin)
add_custom_command(TARGET qemu
    USES_TERMINAL
    COMMAND
    qemu-system-riscv32 -nographic -icount 3 -machine ${SOC_TARGET}
    -drive file=${APP_NAME}_qemu_image.bin,if=mtd,format=raw
    )

# Qemu Debug Command
add_custom_target(qemu_debug DEPENDS ${APP_NAME}_qemu_image.bin)
add_custom_command(TARGET qemu_debug
    USES_TERMINAL
    COMMAND
    qemu-system-riscv32 -s -S -nographic -icount 3 -machine ${SOC_TARGET}
    -drive file=${APP_NAME}_qemu_image.bin,if=mtd,format=raw
    )

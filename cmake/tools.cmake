# Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

include(FetchContent)

# Define paths and versions
set(TOOLS_PATH ${BIST_ROOT_DIR}/tools)
set(RISCV_ESP_ELF_VERSION 13.2.0_20230928)
set(RISCV_ESP_ELF_GDB_VERSION 14.2_20240403)  # Example version for GDB

# Define URLs and SHA256 checksums
set(RISCV_ESP_ELF_URL https://github.com/espressif/crosstool-NG/releases/download/esp-${RISCV_ESP_ELF_VERSION}/riscv32-esp-elf-${RISCV_ESP_ELF_VERSION}-x86_64-linux-gnu.tar.xz)
set(RISCV_ESP_ELF_SHA256 782feefe354500c5f968e8c91959651be3bdbbd7ae8a17affcee2b1bffcaad89)

set(RISCV_ESP_ELF_GDB_URL https://github.com/espressif/binutils-gdb/releases/download/esp-gdb-v${RISCV_ESP_ELF_GDB_VERSION}/riscv32-esp-elf-gdb-${RISCV_ESP_ELF_GDB_VERSION}-x86_64-linux-gnu.tar.gz)
set(RISCV_ESP_ELF_GDB_SHA256 ce004bc0bbd71b246800d2d13b239218b272a38bd528e316f21f1af2db8a4b13)  # Replace with actual SHA256

function(download_and_extract_toolchain TOOL_NAME TOOL_VERSION TOOL_URL TOOL_SHA256)
    if(NOT EXISTS ${TOOLS_PATH}/${TOOL_NAME})
        message(STATUS "${TOOL_NAME} directory does not exist. Downloading and extracting...")

        FetchContent_Declare(
            ${TOOL_NAME}_tar
            URL ${TOOL_URL}
            DOWNLOAD_DIR "${BIST_ROOT_DIR}"
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )

        # Populate the download
        FetchContent_GetProperties(${TOOL_NAME}_tar)
        if(NOT ${TOOL_NAME}_tar_POPULATED)
            FetchContent_MakeAvailable(${TOOL_NAME}_tar)
        endif()

        string(FIND "${TOOL_URL}" "/" LAST_SLASH_INDEX REVERSE)
        math(EXPR FILENAME_START_INDEX "${LAST_SLASH_INDEX} + 1")
        string(SUBSTRING "${TOOL_URL}" ${FILENAME_START_INDEX} -1 FILENAME)

        # Verify the SHA256 checksum
        file(SHA256 "${BIST_ROOT_DIR}/${FILENAME}" DOWNLOADED_SHA256)
        if(NOT DOWNLOADED_SHA256 STREQUAL ${TOOL_SHA256})
            message(FATAL_ERROR "SHA256 checksum mismatch for ${TOOL_NAME}! Expected: ${TOOL_SHA256}, Got: ${DOWNLOADED_SHA256}")
        else()
            message(STATUS "SHA256 checksum for ${TOOL_NAME} verified successfully.")
        endif()

        # Extract the .tar file to the /tools directory
        find_program(TAR_COMMAND tar)
        if(NOT TAR_COMMAND)
            message(FATAL_ERROR "tar command not found! Required for extraction.")
        endif()

        # Create the /tools directory if it doesn't exist
        file(MAKE_DIRECTORY ${TOOLS_PATH})

        # Extract the .tar file
        execute_process(
            COMMAND ${TAR_COMMAND} -xf "${BIST_ROOT_DIR}/${FILENAME}" -C ${TOOLS_PATH}
            RESULT_VARIABLE result
        )

        if(NOT result EQUAL 0)
            message(FATAL_ERROR "Failed to extract the .tar file for ${TOOL_NAME}!")
        else()
            message(STATUS "File for ${TOOL_NAME} extracted to: ${TOOLS_PATH}")
        endif()

        # Clean up the downloaded .tar.xz file
        file(REMOVE "${BIST_ROOT_DIR}/${FILENAME}")
    else()
        message(STATUS "${TOOL_NAME} directory already exists. Skipping download and extraction.")
    endif()
endfunction()

# Download and extract riscv32-esp-elf
download_and_extract_toolchain(
    "riscv32-esp-elf"
    ${RISCV_ESP_ELF_VERSION}
    ${RISCV_ESP_ELF_URL}
    ${RISCV_ESP_ELF_SHA256}
)

# Download and extract riscv32-esp-elf-gdb
download_and_extract_toolchain(
    "riscv32-esp-elf-gdb"
    ${RISCV_ESP_ELF_GDB_VERSION}
    ${RISCV_ESP_ELF_GDB_URL}
    ${RISCV_ESP_ELF_GDB_SHA256}
)

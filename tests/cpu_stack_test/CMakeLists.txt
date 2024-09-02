# Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16)
set(BIST_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../)

# Set the toolchain file path
set(CMAKE_TOOLCHAIN_FILE ${BIST_ROOT_DIR}/cmake/toolchain-${SOC_TARGET}.cmake)

set(APP_NAME critical_fw_${SOC_TARGET})
set(APP_EXECUTABLE ${APP_NAME}.elf)

add_executable(${APP_EXECUTABLE}
    main.c
)

project(cpu_stack_test C ASM)

include(${BIST_ROOT_DIR}/cmake/project.cmake)

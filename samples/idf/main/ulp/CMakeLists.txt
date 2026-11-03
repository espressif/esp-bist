# Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16)

project(${ULP_APP_NAME})
add_executable(${ULP_APP_NAME} main.c)

include(IDFULPProject)

ulp_apply_default_options(${ULP_APP_NAME})

ulp_apply_default_sources(${ULP_APP_NAME})

ulp_add_build_binary_targets(${ULP_APP_NAME})

# BIST library — BIST_ROOT_DIR is injected by esp-bist's project_include.cmake
add_subdirectory(${BIST_ROOT_DIR}/src/bist ${CMAKE_BINARY_DIR}/bist)

target_link_libraries(${ULP_APP_NAME} PUBLIC bist_esp)

bist_add_crc_postbuild(${ULP_APP_NAME})

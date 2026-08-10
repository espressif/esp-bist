# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16)

project(${ULP_APP_NAME})
add_executable(${ULP_APP_NAME} main.c)

include(IDFULPProject)

ulp_apply_default_options(${ULP_APP_NAME})

# Apply the BIST custom linker script (function provided by the IDF patch)
ulp_apply_custom_linker_script(${ULP_APP_NAME} "${CMAKE_CURRENT_LIST_DIR}/../../../../../src/soc/${IDF_TARGET}/ld/idf.ld")

ulp_apply_default_sources(${ULP_APP_NAME})

ulp_add_build_binary_targets(${ULP_APP_NAME})

# BIST library — repo root is 5 levels up from tests/integration/hd_idf/main/ulp/
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../../../../src/bist ${CMAKE_BINARY_DIR}/bist)

target_link_libraries(${ULP_APP_NAME} PUBLIC bist_esp)

bist_add_crc_postbuild(${ULP_APP_NAME})

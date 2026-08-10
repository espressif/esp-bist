# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0
#
# Host-native unit tests for bist_hd_protocol (no IDF / target required).
#
#   cmake -S tests/unit/hd_protocol -B build/hd_protocol_test
#   cmake --build build/hd_protocol_test
#   ctest --test-dir build/hd_protocol_test

cmake_minimum_required(VERSION 3.16)
project(hd_protocol_test C)

set(BIST_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")

add_executable(hd_protocol_test
    test_bist_hd_protocol.c
    "${BIST_ROOT}/src/bist/bist_hd_protocol.c"
    "${BIST_ROOT}/src/bist/bist_hd_crc.c"
    "${BIST_ROOT}/src/bist/bist_hd_challenge.c"
)

target_include_directories(hd_protocol_test PRIVATE
    "${BIST_ROOT}/src/bist/include"
)

target_compile_options(hd_protocol_test PRIVATE -Wall -Wextra -Werror)

enable_testing()
add_test(NAME hd_protocol_test COMMAND hd_protocol_test)

# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0
#
# Host-native unit tests for bist_clock_math (no IDF / target required).
#
#   cmake -S tests/unit/clock_math -B build/clock_math_test
#   cmake --build build/clock_math_test
#   ctest --test-dir build/clock_math_test

cmake_minimum_required(VERSION 3.16)
project(clock_math_test C)

set(BIST_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")

add_executable(clock_math_test
    test_bist_clock_math.c
)

target_include_directories(clock_math_test PRIVATE
    "${BIST_ROOT}/src/bist/core/clock/include"
)

target_compile_options(clock_math_test PRIVATE -Wall -Wextra -Werror)
target_link_libraries(clock_math_test PRIVATE m)

enable_testing()
add_test(NAME clock_math_test COMMAND clock_math_test)

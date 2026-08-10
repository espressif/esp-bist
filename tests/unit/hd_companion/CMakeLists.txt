# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0
#
# Host-native verdict tests for the LP companion state machine. The companion is
# compiled from src/bist and driven by a scripted fake agent, so the fail-closed
# judgment is covered without a device and without any test hook in the library.
#
#   cmake -S tests/unit/hd_companion -B build/hd_companion_test
#   cmake --build build/hd_companion_test
#   ctest --test-dir build/hd_companion_test

cmake_minimum_required(VERSION 3.16)
project(hd_companion_test C)

set(BIST_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")

add_executable(hd_companion_test
    test_bist_hd_companion.c
    "${BIST_ROOT}/src/bist/bist_hd_protocol.c"
    "${BIST_ROOT}/src/bist/bist_hd_crc.c"
    "${BIST_ROOT}/src/bist/bist_hd_challenge.c"
    "${BIST_ROOT}/src/bist/companion/bist_hd_companion.c"
)

# stubs/ comes first so bist_conf.h, sdkconfig.h and bist_log.h resolve to the
# host-native shims instead of the on-target headers.
target_include_directories(hd_companion_test PRIVATE
    stubs
    "${BIST_ROOT}/src/bist/include"
    "${BIST_ROOT}/src/bist/companion/include"
    "${BIST_ROOT}/src/bist/core/cpu/include"
    "${BIST_ROOT}/src/bist/core/memory/include"
    "${BIST_ROOT}/src/bist/drivers/include"
)

target_compile_options(hd_companion_test PRIVATE -Wall -Wextra -Werror)

enable_testing()

# One process per scenario: the companion latches safe state with no reset path.
set(HD_COMPANION_SCENARIOS
    ready
    no_ready
    ready_wrong_type
    postboot_fail
    qa_pass
    qa_wrong_value
    qa_wrong_seq
    qa_wrong_type
    qa_silent
    qa_late
    safe_state_latched
    runtime_fail
)

foreach(scenario IN LISTS HD_COMPANION_SCENARIOS)
    add_test(NAME hd_companion_${scenario} COMMAND hd_companion_test ${scenario})
endforeach()

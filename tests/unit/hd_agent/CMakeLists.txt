# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0
#
# Host-native unit tests for the HP agent's checkpoint counter. The agent is
# compiled from src/bist; transport and platform adapters are stubbed so the
# private counter can be observed without a device.
#
#   cmake -S tests/unit/hd_agent -B build/hd_agent_test
#   cmake --build build/hd_agent_test
#   ctest --test-dir build/hd_agent_test

cmake_minimum_required(VERSION 3.16)
project(hd_agent_test C)

set(BIST_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")

add_executable(hd_agent_test
    test_bist_hd_agent.c
    "${BIST_ROOT}/src/bist/bist_hd_protocol.c"
    "${BIST_ROOT}/src/bist/bist_hd_crc.c"
    "${BIST_ROOT}/src/bist/host/bist_hd_agent.c"
)

# stubs/ comes first so bist_conf.h, sdkconfig.h and bist_log.h resolve to the
# host-native shims instead of the on-target headers.
target_include_directories(hd_agent_test PRIVATE
    stubs
    "${BIST_ROOT}/src/bist/include"
    "${BIST_ROOT}/src/bist/host/include"
)

target_compile_options(hd_agent_test PRIVATE -Wall -Wextra -Werror)

find_package(Threads REQUIRED)
target_link_libraries(hd_agent_test PRIVATE Threads::Threads)

enable_testing()

# One process per scenario: the agent has file-scope static state that cannot
# be reset without a fresh process.
set(HD_AGENT_SCENARIOS
    checkpoint_first_id
    checkpoint_step_is_one
    checkpoint_send_failure_consumes_id
    diag_req_handled
    diag_req_flushes_checkpoint
)

foreach(scenario IN LISTS HD_AGENT_SCENARIOS)
    add_test(NAME hd_agent_${scenario} COMMAND hd_agent_test ${scenario})
endforeach()

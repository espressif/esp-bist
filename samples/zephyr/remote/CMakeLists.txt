# Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

if(CONFIG_BOARD_ESP32C6_DEVKITC_ESP32C6_LPCORE)
  message(STATUS "${BOARD}${BOARD_QUALIFIERS} compile as remote in this sample")
else()
  message(FATAL_ERROR "${BOARD}${BOARD_QUALIFIERS} is not supported for this sample")
endif()

project(zephyr_bist_sample)

target_sources(app PRIVATE src/main.c)

# Copyright (c) 2024-2025 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.22)

set(BIST_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../)

set(APP_NAME ram_test)

set(APP_SOURCES
    main.c
)

include(${BIST_ROOT_DIR}/cmake/project.cmake)

project(${APP_NAME})

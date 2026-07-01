# Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

set(app_sources "main.c")

idf_component_register(SRCS ${app_sources}
                       REQUIRES ulp esp-bist
                       WHOLE_ARCHIVE)

ulp_add_project("ulp_idf_bist_sample" "${CMAKE_CURRENT_LIST_DIR}/ulp/")

# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

set(app_sources "main.c")

idf_component_register(SRCS ${app_sources}
                       REQUIRES ulp esp-bist
                       WHOLE_ARCHIVE)

ulp_add_project("ulp_hd_idf_test" "${CMAKE_CURRENT_LIST_DIR}/ulp/")

if(CONFIG_BIST_HD_TEST_KEY_MISMATCH)
    # HP-only override; the ULP sub-project keeps the configured key, so the two
    # ends of the wire disagree without changing anything in the library.
    idf_component_get_property(bist_lib esp-bist COMPONENT_LIB)
    target_compile_definitions(${bist_lib} PRIVATE BIST_HD_CHALLENGE_KEY=0x5A5A5A5Au)
endif()

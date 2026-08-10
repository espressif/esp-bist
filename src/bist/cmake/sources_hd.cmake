# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0
#
# Host Diagnostics source collectors (OS-agnostic lists).
#
# Paths in outputs are relative to src/bist/.
#
#   bist_hd_collect_hp_sources("idf"|"zephyr" out_srcs out_incs)
#   bist_hd_collect_lp_sources("idf"|"zephyr" out_srcs out_incs)
#
# Never defines IS_ULP_COCPU (callers decide).
# Catalog DIAG audits / app callbacks / HP STL slices are deferred.

function(bist_hd_collect_hp_sources os_port out_srcs out_incs)
    set(_srcs
        "bist_hd_protocol.c"
        "bist_hd_challenge.c"
        "bist_hd_crc.c"
        "host/bist_hd_agent.c"
        "host/bist_hd_audits.c"
    )
    set(_incs
        "include"
        "host/include"
        "ulp_include"
    )

    if(os_port STREQUAL "idf")
        list(APPEND _srcs
            "host/idf/bist_hd_transport_idf.c"
            "host/idf/bist_hd_platform_idf.c"
        )
    elseif(os_port STREQUAL "zephyr")
        list(APPEND _srcs
            "host/zephyr/bist_hd_transport_zephyr.c"
            "host/zephyr/bist_hd_platform_zephyr.c"
        )
    elseif(os_port STREQUAL "nuttx")
        list(APPEND _srcs
            "host/nuttx/bist_hd_transport_nuttx.c"
            "host/nuttx/bist_hd_platform_nuttx.c"
        )
    else()
        message(FATAL_ERROR "bist_hd_collect_hp_sources: unsupported os_port '${os_port}'")
    endif()

    set(${out_srcs} "${_srcs}" PARENT_SCOPE)
    set(${out_incs} "${_incs}" PARENT_SCOPE)
endfunction()

function(bist_hd_collect_lp_sources os_port out_srcs out_incs)
    set(_srcs
        "bist_hd_protocol.c"
        "bist_hd_challenge.c"
        "companion/bist_hd_companion.c"
    )
    # CRC lives in STL when flash test is on; otherwise pull it for challenge.
    if(NOT CONFIG_ESP_BIST_MEMORY_FLASH_TEST)
        list(APPEND _srcs "bist_hd_crc.c")
    endif()
    set(_incs
        "include"
        "companion/include"
        "ulp_include"
    )

    if(os_port STREQUAL "idf")
        list(APPEND _srcs "companion/idf/bist_hd_comp_port_idf.c")
    elseif(os_port STREQUAL "zephyr")
        list(APPEND _srcs "companion/zephyr/bist_hd_comp_port_zephyr.c")
    elseif(os_port STREQUAL "nuttx")
        list(APPEND _srcs "companion/nuttx/bist_hd_comp_port_nuttx.c")
    else()
        message(FATAL_ERROR "bist_hd_collect_lp_sources: unsupported os_port '${os_port}'")
    endif()

    set(${out_srcs} "${_srcs}" PARENT_SCOPE)
    set(${out_incs} "${_incs}" PARENT_SCOPE)
endfunction()

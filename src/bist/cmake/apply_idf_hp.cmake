# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0
#
# IDF HP component helpers. Call from the repo-root component CMakeLists.txt
# (EXTRA_COMPONENT_DIRS) so idf_component_register path resolution stays
# relative to the component root — not this included file's directory.
#
#   bist_idf_hp_collect(out_srcs out_incs [out_priv_incs])
#     → paths relative to the esp-bist component root (repo root)
#
#   bist_idf_hp_apply_definitions()
#     → SOC_TARGET_* on ${COMPONENT_LIB} when HDA is enabled (no IS_ULP_COCPU)
#
# PRIV_REQUIRES ulp must stay unconditional in the caller's
# idf_component_register(): IDF's early dependency scanner evaluates
# requirements before Kconfig values are available.

include(${CMAKE_CURRENT_LIST_DIR}/sources_hd.cmake)

function(bist_idf_hp_collect out_srcs out_incs)
    set(_srcs "")
    set(_incs "src/bist/include")
    set(_priv_incs "")

    if(CONFIG_ESP_BIST_HOST_DIAGNOSTICS)
        bist_hd_collect_hp_sources("idf" _hd_srcs _hd_incs)
        foreach(_src IN LISTS _hd_srcs)
            list(APPEND _srcs "src/bist/${_src}")
        endforeach()
        list(APPEND _incs "src/bist/host/include")
        foreach(_inc IN LISTS _hd_incs)
            if(NOT _inc STREQUAL "include" AND NOT _inc STREQUAL "host/include")
                list(APPEND _priv_incs "src/bist/${_inc}")
            endif()
        endforeach()
        message(STATUS "ESP-BIST Host Diagnostics: registering HP agent sources (no IS_ULP_COCPU)")
    endif()

    list(REMOVE_DUPLICATES _incs)
    list(REMOVE_DUPLICATES _priv_incs)
    set(${out_srcs} "${_srcs}" PARENT_SCOPE)
    set(${out_incs} "${_incs}" PARENT_SCOPE)
    if(ARGC GREATER 2)
        set(${ARGV2} "${_priv_incs}" PARENT_SCOPE)
    endif()
endfunction()

function(bist_idf_hp_apply_definitions)
    if(CONFIG_ESP_BIST_HOST_DIAGNOSTICS AND DEFINED IDF_TARGET)
        string(TOUPPER "${IDF_TARGET}" _bist_hd_soc_upper)
        target_compile_definitions(${COMPONENT_LIB} PUBLIC "SOC_TARGET_${_bist_hd_soc_upper}")
    endif()
endfunction()

# Copyright (c) 2025-2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0
#
# ESP-IDF component entry for ESP-BIST (HP side).
# LP STL / companion build via the ULP sub-project (src/bist/idf.cmake).

include("${CMAKE_CURRENT_LIST_DIR}/src/bist/cmake/apply_idf_hp.cmake")

bist_idf_hp_collect(_bist_hd_srcs _bist_hd_incs _bist_hd_priv_incs)

# Always listed so an empty LDFRAGMENTS argument cannot swallow PRIV_REQUIRES.
# The fragment places .dram0.safe_ram. The live march span is a second script,
# added only when the RAM audit is on.
# PRIV_REQUIRES ulp is unconditional — see apply_idf_hp.cmake.
idf_component_register(
    SRCS ${_bist_hd_srcs}
    INCLUDE_DIRS ${_bist_hd_incs}
    PRIV_INCLUDE_DIRS ${_bist_hd_priv_incs}
    LDFRAGMENTS "src/bist/ld/hd_ram_idf.lf"
    PRIV_REQUIRES ulp soc
    REQUIRED_IDF_TARGETS esp32c5 esp32c6 esp32p4
)

if(CONFIG_ESP_BIST_HD_AUDIT_RAM)
    target_linker_script(${COMPONENT_LIB} INTERFACE
        "${CMAKE_CURRENT_LIST_DIR}/src/bist/ld/hd_ram_idf_span.ld.in")
endif()

bist_idf_hp_apply_definitions()

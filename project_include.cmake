# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

if(IDF_BUILD_V2 AND __ULP_BUILDV2)
    return()
endif()

set(ESP_BIST_ROOT_DIR "${COMPONENT_DIR}" CACHE INTERNAL "Path to ESP-BIST component root")
set(ESP_BIST_LP_LINKER_LAYOUT "${COMPONENT_DIR}/src/soc/${IDF_TARGET}/ld/idf.ld"
    CACHE INTERNAL "Path to ESP-BIST LP-core linker layout")

idf_build_set_property(EXTRA_CMAKE_ARGS "-DBIST_ROOT_DIR=${COMPONENT_DIR}" APPEND)

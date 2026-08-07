# ##############################################################################
# samples/nuttx/nuttx_bist/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
# ##############################################################################

if(CONFIG_ESP_NUTTX_BIST)
  set(_nuttx_bist_deps)

  if(CONFIG_ESPRESSIF_USE_LP_CORE)
    get_filename_component(_nuttx_bist_dir "${CMAKE_CURRENT_LIST_DIR}" REALPATH)
    get_filename_component(ESP_BIST_ROOT "${_nuttx_bist_dir}/../../.." ABSOLUTE)

    set(ULP_APP_NAME nuttx_bist)
    set(ULP_APP_FOLDER ${CMAKE_CURRENT_LIST_DIR}/ulp)
    list(APPEND ULP_APP_C_SRCS main.c)
    list(APPEND ULP_APP_INCLUDES ${CMAKE_CURRENT_LIST_DIR})
    list(APPEND ULP_APP_INCLUDES ${NUTTX_DIR}/boards/risc-v/${CONFIG_ARCH_CHIP}/common/scripts)

    add_subdirectory(${ESP_BIST_ROOT}/src/bist ${CMAKE_CURRENT_BINARY_DIR}/bist)

    include(${NUTTX_DIR}/arch/risc-v/src/common/espressif/esp_ulp.cmake)
    bist_add_crc_postbuild(${ULP_ELF_FILE})
    list(APPEND _nuttx_bist_deps nuttx_bist_ulp_bin)
  endif()

  nuttx_add_application(
    NAME
    nuttx_bist
    PRIORITY
    ${CONFIG_ESP_NUTTX_BIST_PRIORITY}
    STACKSIZE
    ${CONFIG_ESP_NUTTX_BIST_STACKSIZE}
    MODULE
    ${CONFIG_ESP_NUTTX_BIST}
    SRCS
    nuttx_bist_main.c
    INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_LIST_DIR}
    DEPENDS
    ${_nuttx_bist_deps})
endif()

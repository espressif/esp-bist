# ##############################################################################
# samples/nuttx/nuttx_bist/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
# ##############################################################################

if(CONFIG_ESP_NUTTX_BIST)
  set(_nuttx_bist_deps)

  get_filename_component(_nuttx_bist_dir "${CMAKE_CURRENT_LIST_DIR}" REALPATH)
  get_filename_component(ESP_BIST_ROOT "${_nuttx_bist_dir}/../../.." ABSOLUTE)
  set(BIST_DIR "${ESP_BIST_ROOT}/src/bist")

  set(ULP_APP_NAME nuttx_bist)
  set(ULP_APP_FOLDER ${CMAKE_CURRENT_LIST_DIR}/ulp)
  list(APPEND ULP_APP_C_SRCS main.c)
  list(APPEND ULP_APP_INCLUDES ${CMAKE_CURRENT_LIST_DIR})
  list(APPEND ULP_APP_INCLUDES ${NUTTX_DIR}/boards/risc-v/${CONFIG_ARCH_CHIP}/common/scripts)

  add_subdirectory(${ESP_BIST_ROOT}/src/bist ${CMAKE_CURRENT_BINARY_DIR}/bist)

  include(${NUTTX_DIR}/arch/risc-v/src/common/espressif/esp_ulp.cmake)
  bist_add_crc_postbuild(${ULP_ELF_FILE})
  list(APPEND _nuttx_bist_deps nuttx_bist_ulp_bin)

  # HP-side HDA sources (agent, transport, platform, protocol)
  set(_nuttx_bist_hp_srcs nuttx_bist_main.c)
  set(_nuttx_bist_hp_incs ${CMAKE_CURRENT_LIST_DIR})
  set(_nuttx_bist_hp_defs NUTTX_ESP_BIST_MODULE)

  if(CONFIG_ESP_BIST_HOST_DIAGNOSTICS)
    include(${BIST_DIR}/cmake/sources_hd.cmake)
    bist_hd_collect_hp_sources("nuttx" _hd_hp_srcs _hd_hp_incs)
    foreach(_src IN LISTS _hd_hp_srcs)
      list(APPEND _nuttx_bist_hp_srcs "${BIST_DIR}/${_src}")
    endforeach()
    foreach(_inc IN LISTS _hd_hp_incs)
      list(APPEND _nuttx_bist_hp_incs "${BIST_DIR}/${_inc}")
    endforeach()
  endif()

  nuttx_add_application(
    NAME
    nuttx_bist
    PRIORITY
    ${CONFIG_ESP_BIST_HD_AGENT_TASK_PRIO_NUTTX}
    STACKSIZE
    ${CONFIG_ESP_BIST_HD_AGENT_TASK_STACK}
    MODULE
    ${CONFIG_ESP_NUTTX_BIST}
    SRCS
    ${_nuttx_bist_hp_srcs}
    INCLUDE_DIRECTORIES
    ${_nuttx_bist_hp_incs}
    DEFINITIONS
    ${_nuttx_bist_hp_defs}
    DEPENDS
    ${_nuttx_bist_deps})
endif()

# ##############################################################################
# tests/integration/hd_nuttx/hd_nuttx_test/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
# ##############################################################################

if(CONFIG_ESP_NUTTX_HD_TEST)
  set(_hd_nuttx_deps)

  get_filename_component(_hd_nuttx_dir "${CMAKE_CURRENT_LIST_DIR}" REALPATH)
  get_filename_component(ESP_BIST_ROOT "${_hd_nuttx_dir}/../../../.." ABSOLUTE)
  set(BIST_DIR "${ESP_BIST_ROOT}/src/bist")

  set(ULP_APP_NAME hd_nuttx_test)
  set(ULP_APP_FOLDER ${CMAKE_CURRENT_LIST_DIR}/ulp)
  list(APPEND ULP_APP_C_SRCS main.c)
  list(APPEND ULP_APP_INCLUDES ${CMAKE_CURRENT_LIST_DIR})
  list(APPEND ULP_APP_INCLUDES ${NUTTX_DIR}/boards/risc-v/${CONFIG_ARCH_CHIP}/common/scripts)

  add_subdirectory(${ESP_BIST_ROOT}/src/bist ${CMAKE_CURRENT_BINARY_DIR}/bist)

  include(${NUTTX_DIR}/arch/risc-v/src/common/espressif/esp_ulp.cmake)
  bist_add_crc_postbuild(${ULP_ELF_FILE})
  list(APPEND _hd_nuttx_deps hd_nuttx_test_ulp_bin)

  # HP-side Host Diagnostics sources (agent, transport, platform, protocol)
  set(_hd_nuttx_hp_srcs hd_nuttx_test_main.c)
  set(_hd_nuttx_hp_incs ${CMAKE_CURRENT_LIST_DIR})
  set(_hd_nuttx_hp_defs NUTTX_ESP_BIST_MODULE)

  if(CONFIG_ESP_BIST_HOST_DIAGNOSTICS)
    include(${BIST_DIR}/cmake/sources_hd.cmake)
    bist_hd_collect_hp_sources("nuttx" _hd_hp_srcs _hd_hp_incs)
    foreach(_src IN LISTS _hd_hp_srcs)
      list(APPEND _hd_nuttx_hp_srcs "${BIST_DIR}/${_src}")
    endforeach()
    foreach(_inc IN LISTS _hd_hp_incs)
      list(APPEND _hd_nuttx_hp_incs "${BIST_DIR}/${_inc}")
    endforeach()
  endif()

  # HP-only override; the ULP sub-project keeps the configured key, so the two
  # ends of the wire disagree without changing anything in the library.
  if(CONFIG_BIST_HD_TEST_KEY_MISMATCH)
    list(APPEND _hd_nuttx_hp_defs "BIST_HD_CHALLENGE_KEY=0x5A5A5A5Au")
  endif()

  nuttx_add_application(
    NAME
    hd_nuttx_test
    PRIORITY
    ${CONFIG_ESP_BIST_HD_AGENT_TASK_PRIO_NUTTX}
    STACKSIZE
    ${CONFIG_ESP_BIST_HD_AGENT_TASK_STACK}
    MODULE
    ${CONFIG_ESP_NUTTX_HD_TEST}
    SRCS
    ${_hd_nuttx_hp_srcs}
    INCLUDE_DIRECTORIES
    ${_hd_nuttx_hp_incs}
    DEFINITIONS
    ${_hd_nuttx_hp_defs}
    DEPENDS
    ${_hd_nuttx_deps})
endif()

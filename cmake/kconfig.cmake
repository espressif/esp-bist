# BIST Kconfig integration (kconfig-frontends)
#
# At configure time: ensures .config exists (from bist.conf), merges with Kconfig
# defaults (olddefconfig), generates bist_conf.h and auto.conf (silentoldconfig),
# and imports CONFIG_* into CMake for use in CMakeLists.txt.
# Build time: ninja can (re)generate .config and the header when inputs change.
# Run conf from the build directory so .config and output paths stay in one tree.

find_program(KCONFIG_CONF kconfig-conf)
find_program(KCONFIG_MCONF kconfig-mconf)
if(NOT KCONFIG_CONF OR NOT KCONFIG_MCONF)
    message(FATAL_ERROR "kconfig-conf and kconfig-mconf not found. Install kconfig-frontends.")
endif()

set(KCONFIG_HEADER        ${CMAKE_CURRENT_BINARY_DIR}/include/bist_conf.h)
set(KCONFIG_AUTO_CONF     ${CMAKE_CURRENT_BINARY_DIR}/include/config/auto.conf)
set(KCONFIG_TRISTATE_CONF ${CMAKE_CURRENT_BINARY_DIR}/include/config/tristate.conf)
set(KCONFIG_CONFIG_FILE   ${CMAKE_CURRENT_BINARY_DIR}/.config)
set(KCONFIG_DEFAULT_CONF  ${CMAKE_SOURCE_DIR}/bist.conf)
set(KCONFIG_FILE          ${BIST_ROOT_DIR}/src/bist/Kconfig)
set(KCONFIG_BINARY_DIR    ${CMAKE_CURRENT_BINARY_DIR})

if(NOT EXISTS ${KCONFIG_DEFAULT_CONF})
    message(FATAL_ERROR "Default config not found: ${KCONFIG_DEFAULT_CONF}")
endif()

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${KCONFIG_CONFIG_FILE} ${KCONFIG_FILE})

file(MAKE_DIRECTORY ${KCONFIG_BINARY_DIR}/include ${KCONFIG_BINARY_DIR}/include/config)

# ---- Configure time: ensure .config, merge defaults, generate header ----

if(NOT EXISTS ${KCONFIG_CONFIG_FILE})
    file(COPY ${KCONFIG_DEFAULT_CONF} DESTINATION ${KCONFIG_BINARY_DIR})
    get_filename_component(_defname ${KCONFIG_DEFAULT_CONF} NAME)
    file(RENAME ${KCONFIG_BINARY_DIR}/${_defname} ${KCONFIG_CONFIG_FILE})
endif()

set(_kconfig_env
    KCONFIG_CONFIG=${KCONFIG_CONFIG_FILE}
    KCONFIG_AUTOHEADER=${KCONFIG_HEADER}
    KCONFIG_AUTOCONFIG=${KCONFIG_AUTO_CONF}
    KCONFIG_TRISTATE=${KCONFIG_TRISTATE_CONF}
)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env ${_kconfig_env}
        ${KCONFIG_CONF} --olddefconfig ${KCONFIG_FILE}
    WORKING_DIRECTORY ${KCONFIG_BINARY_DIR}
    RESULT_VARIABLE _ret
    OUTPUT_QUIET
    ERROR_VARIABLE _err
)
if(NOT _ret EQUAL 0)
    message(FATAL_ERROR "Kconfig olddefconfig failed (${_ret}).\n${_err}")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env ${_kconfig_env}
        ${KCONFIG_CONF} --silentoldconfig ${KCONFIG_FILE}
    WORKING_DIRECTORY ${KCONFIG_BINARY_DIR}
    RESULT_VARIABLE _ret
    OUTPUT_QUIET
    ERROR_VARIABLE _err
)
if(NOT _ret EQUAL 0)
    message(FATAL_ERROR "Kconfig silentoldconfig failed (${_ret}).\n${_err}")
endif()

# ---- Build-time rules (ninja can recreate when inputs change) ----

add_custom_command(
    OUTPUT ${KCONFIG_CONFIG_FILE}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${KCONFIG_DEFAULT_CONF} ${KCONFIG_CONFIG_FILE}
    COMMAND ${CMAKE_COMMAND} -E env ${_kconfig_env}
        ${KCONFIG_CONF} --olddefconfig ${KCONFIG_FILE}
    WORKING_DIRECTORY ${KCONFIG_BINARY_DIR}
    DEPENDS ${KCONFIG_DEFAULT_CONF} ${KCONFIG_FILE}
    COMMENT "Creating .config from bist.conf and Kconfig defaults"
)

add_custom_command(
    OUTPUT ${KCONFIG_HEADER}
    COMMAND ${CMAKE_COMMAND} -E env ${_kconfig_env}
        ${KCONFIG_CONF} --silentoldconfig ${KCONFIG_FILE}
    WORKING_DIRECTORY ${KCONFIG_BINARY_DIR}
    DEPENDS ${KCONFIG_CONFIG_FILE} ${KCONFIG_FILE}
    COMMENT "Generating bist_conf.h from .config"
)

add_custom_target(bist_conf_gen DEPENDS ${KCONFIG_CONFIG_FILE})
add_custom_target(bist_conf_h DEPENDS ${KCONFIG_HEADER})
add_dependencies(bist_conf_h bist_conf_gen)

add_custom_target(menuconfig
    COMMAND ${KCONFIG_MCONF} ${KCONFIG_FILE}
    WORKING_DIRECTORY ${KCONFIG_BINARY_DIR}
    USES_TERMINAL
    COMMENT "Launching Kconfig menu (re-run cmake after saving)"
)

# ---- Import CONFIG_* from .config into CMake ----
if(EXISTS ${KCONFIG_CONFIG_FILE})
    file(STRINGS ${KCONFIG_CONFIG_FILE} _lines ENCODING UTF-8)
    foreach(_line IN LISTS _lines)
        if("${_line}" MATCHES "^CONFIG_([^=]+)=([ymn]|\".*\"|[^#]*)$")
            set(_name "CONFIG_${CMAKE_MATCH_1}")
            set(_val "${CMAKE_MATCH_2}")
            string(STRIP "${_val}" _val)
            if("${_val}" MATCHES "^\"(.*)\"$")
                set(_val "${CMAKE_MATCH_1}")
            endif()
            set("${_name}" "${_val}" PARENT_SCOPE)
            set("${_name}" "${_val}")
        elseif("${_line}" MATCHES "^# CONFIG_([^ ]+) is not set")
            set(_name "CONFIG_${CMAKE_MATCH_1}")
            unset("${_name}" PARENT_SCOPE)
            unset("${_name}")
        endif()
    endforeach()
endif()

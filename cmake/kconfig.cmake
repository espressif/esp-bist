# Find kconfig-conf and kconfig-mconf (usually part of kconfig-frontends)
find_program(KCONFIG_CONF kconfig-conf)
find_program(KCONFIG_MCONF kconfig-mconf)

if(NOT KCONFIG_CONF)
    message(FATAL_ERROR "kconfig-conf not found. Please install kconfig-frontends.")
endif()
if(NOT KCONFIG_MCONF)
    message(FATAL_ERROR "kconfig-mconf not found. Please install kconfig-frontends.")
endif()

set(KCONFIG_HEADER        ${CMAKE_CURRENT_BINARY_DIR}/include/bist_conf.h)
set(KCONFIG_AUTO_CONF     ${CMAKE_CURRENT_BINARY_DIR}/include/config/auto.conf)
set(KCONFIG_TRISTATE_CONF ${CMAKE_CURRENT_BINARY_DIR}/include/config/tristate.conf)
set(KCONFIG_CONFIG_FILE   ${CMAKE_CURRENT_BINARY_DIR}/.config)
set(KCONFIG_DEFAULT_CONF   ${CMAKE_SOURCE_DIR}/bist.conf)
set(KCONFIG_FILE          ${BIST_ROOT_DIR}/src/bist/Kconfig)   # adjust if needed

if(NOT EXISTS ${KCONFIG_DEFAULT_CONF})
    message(FATAL_ERROR "Default configuration file bist.conf not found in ${CMAKE_SOURCE_DIR}. Please provide this file.")
endif()

# Generate the .config file from bist.conf
add_custom_command(
    OUTPUT ${KCONFIG_CONFIG_FILE}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${KCONFIG_DEFAULT_CONF} ${KCONFIG_CONFIG_FILE}
    DEPENDS ${KCONFIG_DEFAULT_CONF}
    COMMENT "Copying bist.conf to .config as default"
)

add_custom_target(bist_conf_gen DEPENDS ${KCONFIG_CONFIG_FILE})

# Add custom target for menuconfig
add_custom_target(menuconfig
    COMMAND ${KCONFIG_MCONF} ${KCONFIG_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    USES_TERMINAL
    DEPENDS ${KCONFIG_CONFIG_FILE}
    COMMENT "Launching Kconfig menu interface..."
)

add_custom_command(
    OUTPUT  ${KCONFIG_HEADER}
    COMMENT "Generating bist_conf.h from .config"

    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_CURRENT_BINARY_DIR}/include
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_CURRENT_BINARY_DIR}/include/config

    COMMAND ${KCONFIG_CONF} --olddefconfig ${KCONFIG_FILE} > /dev/null

    COMMAND ${CMAKE_COMMAND} -E env
                KCONFIG_CONFIG=${KCONFIG_CONFIG_FILE}
                KCONFIG_AUTOHEADER=${KCONFIG_HEADER}
                KCONFIG_AUTOCONFIG=${KCONFIG_AUTO_CONF}
                KCONFIG_TRISTATE=${KCONFIG_TRISTATE_CONF}
                ${KCONFIG_CONF} --silentoldconfig ${KCONFIG_FILE}

    DEPENDS ${KCONFIG_FILE}
            ${KCONFIG_CONFIG_FILE}
)

# Add custom target for bist_conf
add_custom_target(bist_conf_h DEPENDS ${KCONFIG_HEADER})

add_dependencies(bist_conf_h bist_conf_gen)

# Generate header before building bist
add_dependencies(bist_esp bist_conf_h)

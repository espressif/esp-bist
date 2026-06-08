# CMake script to configure flasher_args.json from a template
cmake_minimum_required(VERSION 3.22)

if(NOT DEFINED INPUT_FILE OR NOT DEFINED OUTPUT_FILE OR NOT DEFINED APP_NAME OR NOT DEFINED SOC_TARGET OR NOT DEFINED APP_ADDR)
    message(FATAL_ERROR "Missing required variables: INPUT_FILE, OUTPUT_FILE, APP_NAME, SOC_TARGET, or APP_ADDR")
endif()

# Read the input template file
file(READ ${INPUT_FILE} TEMPLATE_CONTENT)

# Replace placeholders (use a temporary variable to avoid policy issues)
set(APP_NAME_VAR "${APP_NAME}")
set(SOC_TARGET_VAR "${SOC_TARGET}")
set(APP_ADDR_VAR "${APP_ADDR}")
string(REPLACE "@APP_NAME@" "${APP_NAME_VAR}" TEMPLATE_CONTENT "${TEMPLATE_CONTENT}")
string(REPLACE "@SOC_TARGET@" "${SOC_TARGET_VAR}" TEMPLATE_CONTENT "${TEMPLATE_CONTENT}")
string(REPLACE "@APP_ADDR@" "${APP_ADDR_VAR}" TEMPLATE_CONTENT "${TEMPLATE_CONTENT}")

# Write the configured file
file(WRITE ${OUTPUT_FILE} "${TEMPLATE_CONTENT}")

message(STATUS "Configured ${OUTPUT_FILE} from template")

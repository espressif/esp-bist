/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#define CONFIG_MMU_PAGE_SIZE                      0x10000
#define CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ           400
#define CONFIG_ESP_CONSOLE_UART_NUM               0
#define CONFIG_RTC_CLK_CAL_CYCLES                 3000
#define CONFIG_IDF_TARGET_ESP32P4                 1
#define CONFIG_IDF_TARGET                         "esp32p4"
#define CONFIG_IDF_TARGET_ARCH_RISCV              1
#define CONFIG_XTAL_FREQ                          40
#define CONFIG_LOG_MAXIMUM_LEVEL                  5
#define CONFIG_LOG_DEFAULT_LEVEL                  3
#define CONFIG_LOG_TIMESTAMP_SOURCE_SYSTEM        1
#define CONFIG_ESP_INT_WDT                        1
#define CONFIG_RTC_CLK_SRC_EXT_CRYS               1
#define CONFIG_RTC_FAST_CLK_SRC_RC_FAST           1
#define CONFIG_LOG_VERSION                        1
#define CONFIG_FREERTOS_NUMBER_OF_CORES           2
#define CONFIG_ESP_CONSOLE_ROM_SERIAL_PORT_NUM    0
#define CONFIG_PARTITION_OFFSET                   0x20000
#define CONFIG_CACHE_L2_CACHE_SIZE                0x20000
#define CONFIG_CACHE_L2_CACHE_LINE_SIZE           64
#define CONFIG_ESP32P4_SELECTS_REV_LESS_V3        0
#define CONFIG_ESP32P4_REV_MIN_FULL               300
#define CONFIG_ESP_REV_MIN_FULL                   CONFIG_ESP32P4_REV_MIN_FULL
#define CONFIG_ESP32P4_REV_MAX_FULL               399
#define CONFIG_ESP_REV_MAX_FULL                   CONFIG_ESP32P4_REV_MAX_FULL
#define CONFIG_BOOTLOADER_CPU_CLK_FREQ_MHZ        CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ
#define NON_OS_BUILD                              1

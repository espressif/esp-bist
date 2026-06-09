/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#define CONFIG_MMU_PAGE_SIZE                      0x10000
#define CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ           32
#define CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_32        1
#define CONFIG_ESP_CONSOLE_UART_NUM               0
#define CONFIG_RTC_CLK_CAL_CYCLES                 1024
#define CONFIG_IDF_TARGET_ESP32H4                 1
#define CONFIG_IDF_TARGET                         "esp32h4"
#define CONFIG_IDF_TARGET_ARCH_RISCV              1
#define CONFIG_XTAL_FREQ                          32
#define CONFIG_XTAL_FREQ_32                       1
#define CONFIG_LOG_MAXIMUM_LEVEL                  5
#define CONFIG_LOG_DEFAULT_LEVEL                  3
#define CONFIG_LOG_TIMESTAMP_SOURCE_SYSTEM        1
#define CONFIG_ESP_INT_WDT                        1
#define CONFIG_RTC_CLK_SRC_EXT_CRYS               1
#define CONFIG_LOG_VERSION                        1
#define CONFIG_FREERTOS_NUMBER_OF_CORES           2
#define CONFIG_ESP_CONSOLE_ROM_SERIAL_PORT_NUM    0
#define CONFIG_PARTITION_OFFSET                   0x20000
#define CONFIG_ESP32H4_SELECTS_REV_MP             1

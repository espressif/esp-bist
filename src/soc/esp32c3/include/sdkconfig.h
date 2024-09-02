
/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#define CONFIG_MMU_PAGE_SIZE                      0x10000
#define CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ           160
#define CONFIG_ESP_CONSOLE_UART_NUM               0
#define CONFIG_RTC_CLK_CAL_CYCLES                 1024
#define CONFIG_IDF_TARGET_ESP32C3                 1
#define CONFIG_XTAL_FREQ                          40
#define CONFIG_LOG_MAXIMUM_LEVEL                  5
#define CONFIG_LOG_DEFAULT_LEVEL                  3
#define CONFIG_LOG_TIMESTAMP_SOURCE_SYSTEM        1
#define CONFIG_BIST_FLASH_TEST_CHUNK_SIZE         0x1000
#define CONFIG_FLASH_SIZE                         0x400000
#define CONFIG_SOC_RTC_FAST_MEM_SUPPORTED         1
#define CONFIG_WDT_TIMEOUT_MS                     5000
#define CONFIG_ESP_INT_WDT                        1
#define CONFIG_RTC_CLK_SRC_EXT_CRYS               1
#define CONFIG_BIST_CLOCK_PERCENT_FREQUENCY_DRIFT 1

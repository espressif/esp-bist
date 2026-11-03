/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Host-native logging shim. The real header picks a printk / lp_core_printf
 * backend that does not exist off target.
 */

#pragma once

#include <stdio.h>

/* Same ceiling as src/bist/drivers/include/bist_log.h. Default is info. */
#ifndef CONFIG_ESP_BIST_LOG_LEVEL
#define CONFIG_ESP_BIST_LOG_LEVEL 3
#endif

#if CONFIG_ESP_BIST_LOG_LEVEL >= 1
#define ESP_LOGE(tag, format, ...) printf("[E][%s] " format "\n", tag, ##__VA_ARGS__)
#else
#define ESP_LOGE(tag, format, ...)
#endif

#if CONFIG_ESP_BIST_LOG_LEVEL >= 2
#define ESP_LOGW(tag, format, ...) printf("[W][%s] " format "\n", tag, ##__VA_ARGS__)
#else
#define ESP_LOGW(tag, format, ...)
#endif

#if CONFIG_ESP_BIST_LOG_LEVEL >= 3
#define ESP_LOGI(tag, format, ...) printf("[I][%s] " format "\n", tag, ##__VA_ARGS__)
#else
#define ESP_LOGI(tag, format, ...)
#endif

#if CONFIG_ESP_BIST_LOG_LEVEL >= 4
#define ESP_LOGD(tag, format, ...) printf("[D][%s] " format "\n", tag, ##__VA_ARGS__)
#else
#define ESP_LOGD(tag, format, ...)
#endif

#if CONFIG_ESP_BIST_LOG_LEVEL >= 5
#define ESP_LOGV(tag, format, ...) printf("[V][%s] " format "\n", tag, ##__VA_ARGS__)
#else
#define ESP_LOGV(tag, format, ...)
#endif

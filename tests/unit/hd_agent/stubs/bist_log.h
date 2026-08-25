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

#define ESP_LOGE(tag, format, ...) printf("[E][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...) printf("[W][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGI(tag, format, ...) printf("[I][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGD(tag, format, ...)
#define ESP_LOGV(tag, format, ...)

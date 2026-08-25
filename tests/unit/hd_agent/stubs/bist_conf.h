/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Kconfig values for the host-native agent checkpoint tests. Shadows
 * src/bist/ulp_include/bist_conf.h, which pulls a generated sdkconfig.h.
 */

#pragma once

#define CONFIG_ESP_BIST_HOST_DIAGNOSTICS 1
#define CONFIG_ESP_BIST_HD_AUDIT_CHECKPOINT 1

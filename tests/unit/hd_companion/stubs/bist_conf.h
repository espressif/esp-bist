/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Kconfig values for the host-native companion verdict tests. Shadows
 * src/bist/ulp_include/bist_conf.h, which pulls a generated sdkconfig.h.
 *
 * Only the CPU register test is enabled, so one stubbed STL entry point is
 * enough to drive both a passing and a failing self-BIST.
 */

#pragma once

#define CONFIG_ESP_BIST_HOST_DIAGNOSTICS 1
#define CONFIG_ESP_BIST_HD_AUDIT_QA 1
#define CONFIG_ESP_BIST_CPU_REG_TEST 1
#define CONFIG_ESP_BIST_HD_AGENT_READY_TIMEOUT_US 1000000
#define CONFIG_ESP_BIST_HD_CHALLENGE_WINDOW_US 10000
#define CONFIG_ESP_BIST_WDT_TIMEOUT_US 50000
#define CONFIG_ESP_BIST_RAM_PARTITION_SIZE 64
#define CONFIG_BIST_FLASH_TEST_CHUNK_SIZE 0x1000

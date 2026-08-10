/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * This file is part of Espressif's BIST (Built-In Self Test) Library.
 *
 * BIST library is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * BIST library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with BIST library. If not, see
 * <https://www.gnu.org/licenses/lgpl-3.0.html>.
 */

/**
 * @file bist_log.h
 * @brief BIST logging macros for HP and LP core builds
 *
 * Provides ESP-IDF-compatible `ESP_LOGx` macros for use throughout the BIST
 * library. The backend depends on the build target:
 *
 * - **HP core (standalone / IDF application):** maps to `ESP_EARLY_LOGx` so
 *   logging works before the full ESP-IDF log subsystem is initialized.
 * - **LP core / Zephyr:** maps to `printk()` with a `[level][tag]` prefix.
 *   Debug is compiled out unless `CONFIG_ESP_BIST_LP_DEBUG_LOG` is set.
 * - **LP core / IDF (ULP, `IS_ULP_COCPU`):** maps to `lp_core_printf()`
 *   with a `[level][tag]` prefix. Same debug gating as Zephyr.
 *
 * Include this header instead of `esp_log.h` in BIST sources so the same
 * logging calls work on both cores and frameworks.
 */

#pragma once

#if defined(__ZEPHYR__)
#include <zephyr/sys/printk.h>
#elif defined(IS_ULP_COCPU)
#include "ulp_lp_core_print.h"
#else
#include "esp_log.h"
#endif

#if defined(IS_ULP_COCPU) || defined(__ZEPHYR__)
#include "sdkconfig.h"
#endif

#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#undef ESP_LOGD
#undef ESP_LOGV

/**
 * @defgroup bist_log_macros BIST Logging Macros
 * @brief ESP-IDF-compatible logging macros
 *
 * All macros use the same `(tag, format, ...)` signature as ESP-IDF `ESP_LOGx`.
 * @{
 */

#if defined(__ZEPHYR__)

/** @brief Log an error message on the LP core (Zephyr) */
#define ESP_LOGE(tag, format, ...) printk("[E][%s] " format "\r\n", tag, ##__VA_ARGS__)

/** @brief Log a warning message on the LP core (Zephyr) */
#define ESP_LOGW(tag, format, ...) printk("[W][%s] " format "\r\n", tag, ##__VA_ARGS__)

/** @brief Log an info message on the LP core (Zephyr) */
#define ESP_LOGI(tag, format, ...) printk("[I][%s] " format "\r\n", tag, ##__VA_ARGS__)

#if defined(CONFIG_ESP_BIST_LP_DEBUG_LOG) && CONFIG_ESP_BIST_LP_DEBUG_LOG
/** @brief Log a debug message on the LP core (Zephyr) */
#define ESP_LOGD(tag, format, ...) printk("[D][%s] " format "\r\n", tag, ##__VA_ARGS__)
#else
/** @brief Debug logging disabled on LP core unless CONFIG_ESP_BIST_LP_DEBUG_LOG */
#define ESP_LOGD(tag, format, ...)
#endif

/** @brief Verbose logging disabled on LP core */
#define ESP_LOGV(tag, format, ...)

#elif defined(IS_ULP_COCPU)

/** @brief Log an error message on the LP core */
#define ESP_LOGE(tag, format, ...) lp_core_printf("[E][%s] " format "\r\n", tag, ##__VA_ARGS__)

/** @brief Log a warning message on the LP core */
#define ESP_LOGW(tag, format, ...) lp_core_printf("[W][%s] " format "\r\n", tag, ##__VA_ARGS__)

/** @brief Log an info message on the LP core */
#define ESP_LOGI(tag, format, ...) lp_core_printf("[I][%s] " format "\r\n", tag, ##__VA_ARGS__)

#if defined(CONFIG_ESP_BIST_LP_DEBUG_LOG) && CONFIG_ESP_BIST_LP_DEBUG_LOG
/** @brief Log a debug message on the LP core */
#define ESP_LOGD(tag, format, ...) lp_core_printf("[D][%s] " format "\r\n", tag, ##__VA_ARGS__)
#else
/** @brief Debug logging disabled on LP core unless CONFIG_ESP_BIST_LP_DEBUG_LOG */
#define ESP_LOGD(tag, format, ...)
#endif

/** @brief Verbose logging disabled on LP core */
#define ESP_LOGV(tag, format, ...)

#else

/** @brief Log an error message using ESP-IDF early log */
#define ESP_LOGE(tag, format, ...) ESP_EARLY_LOGE(tag, format, ##__VA_ARGS__)

/** @brief Log a warning message using ESP-IDF early log */
#define ESP_LOGW(tag, format, ...) ESP_EARLY_LOGW(tag, format, ##__VA_ARGS__)

/** @brief Log an info message using ESP-IDF early log */
#define ESP_LOGI(tag, format, ...) ESP_EARLY_LOGI(tag, format, ##__VA_ARGS__)

/** @brief Log a debug message using ESP-IDF early log */
#define ESP_LOGD(tag, format, ...) ESP_EARLY_LOGD(tag, format, ##__VA_ARGS__)

/** @brief Log a verbose message using ESP-IDF early log */
#define ESP_LOGV(tag, format, ...) ESP_EARLY_LOGV(tag, format, ##__VA_ARGS__)

#endif

/** @} */

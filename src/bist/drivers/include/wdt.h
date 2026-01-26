/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * This file is part of Espressif's BIST (Built-In Self Test) Library.
 *
 * BIST library is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * BIST library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with BIST library. If not, see
 * <https://www.gnu.org/licenses/>.
 */

/**
 * @file wdt.h
 * @brief Watchdog timer driver interface
 *
 * Provides low-level watchdog timer control for BIST tests.
 */

#pragma once

#include <stdint.h>

/**
 * @brief Deinitialize watchdog timer
 *
 * Disables and stops the watchdog timer.
 */
void wdt_deinit(void);

/**
 * @brief Initialize watchdog timer
 *
 * Configures and starts the watchdog timer with specified timeout.
 *
 * @param timeout_us Timeout period in microseconds
 */
void wdt_init(uint32_t timeout_us);

/**
 * @brief Feed/refresh watchdog timer
 *
 * Resets the watchdog counter to prevent timeout reset.
 */
void wdt_feed(void);

/**
 * @brief Register watchdog timeout callback
 *
 * Sets a callback function to be invoked when watchdog times out.
 *
 * @param callback Callback function pointer
 * @param arg User argument passed to callback
 */
void wdt_register_callback(void (*callback)(void *), void *arg);

/**
 * @brief Initialize windowed watchdog timer
 *
 * Configures watchdog with underflow window - feeding too early
 * will also trigger reset (ensures minimum execution time).
 *
 * @param underflow_timeout_us Underflow window in microseconds
 */
void wdt_init_windowed(uint32_t underflow_timeout_us);

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
 * @file wdt.h
 * @brief Watchdog timer driver interface
 *
 * Provides low-level watchdog timer control for BIST tests.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

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
 * The minimum accepted value is 500 us (one MWDT tick).
 *
 * @param timeout_us Timeout period in microseconds (>= 500)
 * @return 0 on success, -1 if timeout_us is too small
 */
int wdt_init(uint32_t timeout_us);

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
 * @return 0 on success, -1 if underflow_timeout_us is 0 or initialization fails
 */
int wdt_init_windowed(uint32_t underflow_timeout_us);

/**
 * @brief Deinitialize windowed watchdog timer
 *
 * Stops the underflow timer, resets windowed state flags, and frees
 * timer resources. Call this between tests to ensure clean state.
 */
void wdt_windowed_deinit(void);

/**
 * @brief Check if a windowed underflow was detected
 *
 * Returns true if wdt_feed() was called before the underflow window
 * elapsed, indicating the application completed too quickly.
 *
 * @return true if underflow was detected, false otherwise
 */
bool wdt_is_underflow_detected(void);

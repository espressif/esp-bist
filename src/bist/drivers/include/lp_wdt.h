/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
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
 * @file lp_wdt.h
 * @brief LP Core watchdog timer driver for safety-critical systems
 *
 * Provides direct register-level LP WDT control for use on the LP core.
 * The LP WDT peripheral (at DR_REG_LP_WDT_BASE) is clocked by the LP slow
 * clock (~32 kHz).
 *
 * On timeout, the LP WDT triggers a full system reset
 * (WDT_STAGE_ACTION_RESET_SYSTEM). This ensures that any failure in the
 * BIST execution path forces the entire chip to a known-good state —
 * the appropriate behavior for safety-critical applications.
 */

#pragma once

#include <stdint.h>

/**
 * @brief Initialize and enable the LP WDT
 *
 * Configures the watchdog with a single stage: full system reset on timeout.
 * Must be fed periodically via lp_wdt_feed() to prevent reset.
 *
 * @param timeout_us Timeout in microseconds before system reset
 */
void lp_wdt_init(uint32_t timeout_us);

/**
 * @brief Feed (kick) the LP WDT
 *
 * Resets the watchdog counter. Must be called periodically from the LP core
 * main loop to prevent system reset.
 */
void lp_wdt_feed(void);

/**
 * @brief Disable the LP WDT
 *
 * Turns off the watchdog timer entirely.
 */
void lp_wdt_disable(void);

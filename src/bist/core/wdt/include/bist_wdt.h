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
 * @file bist_wdt.h
 * @brief Watchdog timer operation test
 *
 * Validates watchdog timer (MWDT) initialization, timeout detection,
 * and reset reason reporting. Test intentionally triggers a watchdog
 * timeout and verifies the system resets with correct reset reason.
 */

#pragma once

#include "stdint.h"
#include "bist_esp_types.h"

/**
 * @brief Test watchdog timer operation
 *
 * Two-phase test:
 * 1. First execution: Initializes watchdog with 100 µs timeout,
 *    waits 1000 µs, causing watchdog to expire and trigger reset
 * 2. After reset: Checks reset reason is RESET_REASON_CORE_MWDT0
 *
 * @return BIST_ESP_OK if WDT reset detected (second execution)
 * @return BIST_ESP_WDT_TEST_ERR if watchdog fails to trigger reset
 *
 * @note First execution will cause system reset. Success is detected
 *       on subsequent boot by checking reset reason.
 */
bist_esp_err_t bist_wdt_test(void);

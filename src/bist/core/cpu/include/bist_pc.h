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
 * @file bist_pc.h
 * @brief Program Counter integrity test
 *
 * Verifies Program Counter register bits are functional by calling test
 * functions placed in different memory regions. Each function exercises
 * different PC bits based on its address.
 */

#pragma once

#include "stdint.h"
#include "bist_esp_types.h"

/**
 * @brief Test Program Counter register integrity
 *
 * Calls 4 test functions placed in different memory regions:
 * - pcTestFunction0 @ IRAM 0x4038xxxx (tests PC bits 2-17)
 * - pcTestFunction1 @ IRAM 0x403Bxxxx (tests PC bits 2-17, inverted)
 * - pcTestFunction2 @ Flash 0x4201xxxx (tests PC bits 19-21, 25)
 * - pcTestFunction3 @ RTC 0x5000xxxx (tests PC bit 28)
 *
 * After each function call, captures return address and compares
 * against expected function pointer to verify PC integrity.
 *
 * @return BIST_ESP_OK if all 4 functions return correctly
 * @return BIST_ESP_PC_TEST_ERR if return address mismatch detected
 *
 * @note Covers 24 of 30 addressable PC bits (bits 0-1 always 0 due to alignment)
 */
bist_esp_err_t bist_pc_test(void);

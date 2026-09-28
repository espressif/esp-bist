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
 * @file bist_pc.h
 * @brief Stable API for the Program Counter integrity test
 *
 * The STATIC and RECIPROCAL implementations expose the same bist_pc_test()
 * entry point.
 *
 */

#pragma once

#include "stdint.h"
#include "bist_esp_types.h"
#ifndef __ZEPHYR__
#include "bist_conf.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Test Program Counter register integrity
 *
 * In STATIC mode, calls four test functions placed in different memory
 * regions:
 * - pcTestFunction0 @ IRAM 0x4038xxxx (tests PC bits 2-17)
 * - pcTestFunction1 @ IRAM 0x403Bxxxx (tests PC bits 2-17, inverted)
 * - pcTestFunction2 @ Flash 0x4201xxxx (tests PC bits 19-21, 25)
 * - pcTestFunction3 @ RTC 0x5000xxxx (tests PC bit 28)
 *
 * After each function call, the implementation captures the return address
 * and compares it against the expected function pointer. This covers 24 of
 * the 30 addressable PC bits (bits 0-1 are always zero due to alignment).
 *
 * In RECIPROCAL mode on ESP32-P4, runs the five-region call walk on HP core 0
 * into evidence slot 0 and on HP core 1 into slot 1:
 * - pcTestFunction0 @ HP SRAM 0x4FFxxxxx
 * - pcTestFunction1 @ Flash 0x400xxxxx (start of .flash.text)
 * - pcTestFunction2 @ Flash, 0xFFF8 bytes after pcTestFunction1
 * - pcTestFunction3 @ LP SRAM 0x5010xxxx
 * - pcTestFunction4 @ TCM 0x3010xxxx
 *
 * Each slot records the walk status, a completion bit for every returned
 * region function, and the returned addresses. Each core validates its own
 * slot and the other core's slot against the canonical linked addresses.
 *
 * @return BIST_ESP_OK if the selected mode completes and all validations pass
 * @return BIST_ESP_PC_TEST_ERR on any walk, dispatch, completion, or address
 *         verification failure
 */
bist_esp_err_t bist_pc_test(void);

#ifdef __cplusplus
}
#endif

#if defined(CONFIG_ESP_BIST_PROGRAM_COUNTER_TEST_RECIPROCAL)
#include "bist_pc_reciprocal.h"
#endif

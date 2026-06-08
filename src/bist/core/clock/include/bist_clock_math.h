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

#pragma once

#include <stdint.h>
#include <math.h>

/**
 * @brief Compute absolute percent deviation between actual and expected
 *        crystal frequencies.
 *
 * Uses signed subtraction up front to avoid relying on implementation-defined
 * conversion of an unsigned wrap-around through int. Defined inline so the
 * same code path runs on the device firmware and host-side unit tests.
 *
 * @param actual    Measured crystal frequency in Hz.
 * @param expected  Nominal crystal frequency in Hz (must be non-zero).
 * @return |actual - expected| / expected * 100.0f, or 0 if expected is 0.
 */
static inline float xtal_deviation_percent(uint32_t actual, uint32_t expected)
{
    if (expected == 0U) {
        return 0.0f;
    }
    int32_t diff = (int32_t)actual - (int32_t)expected;
    return fabsf((float)diff) / (float)expected * 100.0f;
}

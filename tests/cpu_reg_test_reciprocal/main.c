/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
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

#include <stdio.h>
#include "rom/ets_sys.h"
#include "bist_esp.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

#if defined(SOC_TARGET_ESP32P4)
void test_BIST_Cpu_Regs(void)
{
    uint8_t ret = bist_cpu_regs_test();
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

void test_BIST_Cpu_Csr_Regs(void)
{
    uint8_t ret = bist_cpu_csr_regs_test();
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}
#endif

int main()
{
    UNITY_BEGIN();
#if defined(SOC_TARGET_ESP32P4)
    RUN_TEST(test_BIST_Cpu_Regs);
    RUN_TEST(test_BIST_Cpu_Csr_Regs);
#endif
    return UNITY_END();
}

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

#include "bist_pc.h"
#include "esp_log.h"
#include "esp_attr.h"

/**
 * Program Counter testing
 *
 * This testing aims to exercise the PC register bits for stuck at conditions.
 * The PC register has 32 bits, but there is some consideration to take in account:
 *
 * Alignment: The PC register is always aligned to 4 bytes, so the lower 2 bits are always 0.
 * Memory mapping:
 * IRAM: 0x40380000 - 0x403BEE00
 * ROM (Executed from ICache):
 * 2MB: 0x42010000 - 0x421FFFFF
 * 4MB: 0x42010000 - 0x423FFFFF
 * 8MB: 0x42010000 - 0x427FFFFF
 * RTC: 0x50000000 - 0x50001FFF
 *
 * pc_test_0 is placed in IRAM, usually at 0x4038xxxx address
 * pc_test_1 is placed in IRAM, but in 0x403Bxxxx with bits 2 - 15 inverted from pc_test_0
 * This way we can test all 2-17 bits of the PC register.
 *
 * pc_test_3 is placed in Flash, usually at address 0x4201xxxx, compared to pc_test_0 and pc_test_1,
 * it will test bits 19/20/21/25
 *
 * pc_test_3 is placed in RTC Memory, usually at address 0x5000xxxx, compared to pc_test_0, pc_test_1 and pc_test_3,
 * it will test bit 28
 *
 * We will have tested bits 2-17, 19-21, 25 and 28 of the PC register.
 *
 */

__attribute__((section(".pc_test_0"))) static void *pcTestFunction0(void);
__attribute__((section(".pc_test_1"))) static void *pcTestFunction1(void);
__attribute__((section(".pc_test_2"))) static void *pcTestFunction2(void);
__attribute__((section(".pc_test_3"))) static void *pcTestFunction3(void);

bist_esp_err_t IRAM_ATTR bist_pc_test(void)
{
    void *returnFunctionAddress;
    void *(*pcTestFunctions[4])(void) = {
        pcTestFunction0,
        pcTestFunction1,
        pcTestFunction2,
        pcTestFunction3,
    };

    BIST_ADD_LABEL("bist_verify_pc_test");
    for (int countPcTest = 0; countPcTest < sizeof(pcTestFunctions) / sizeof(void *); countPcTest++) {
        returnFunctionAddress = (*pcTestFunctions[countPcTest])();
        ESP_EARLY_LOGD("BIST_PC", "returnFunctionAddress %d: %p", countPcTest, returnFunctionAddress);
        if (pcTestFunctions[countPcTest] != returnFunctionAddress) {
            return BIST_ESP_PC_TEST_ERR;
        }
    }

    return BIST_ESP_OK;
}

void *pcTestFunction0(void)
{
    return (pcTestFunction0);
}

void *pcTestFunction1(void)
{
    return (pcTestFunction1);
}

void *pcTestFunction2(void)
{
    return (pcTestFunction2);
}

void *pcTestFunction3(void)
{
    return (pcTestFunction3);
}

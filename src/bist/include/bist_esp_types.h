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

#pragma once

#define ASM(x)                __asm volatile(x)
#define BIST_ADD_LABEL(label) ASM(#label ":")

typedef enum {
    BIST_ESP_OK = 0,
    BIST_ESP_CPU_TEST_ERR = 1,
    BIST_ESP_CPU_CSR_TEST_ERR = 2,
    BIST_ESP_RAM_TEST_ERR = 3,
    BIST_ESP_FLASH_TEST_ERR = 4,
    BIST_ESP_PC_TEST_ERR = 5,
    BIST_ESP_CLOCK_TEST_ERR = 6,
    BIST_ESP_STACK_TEST_ERR = 7,
    BIST_ESP_STACK_TEST_OVERFLOW = 8,
    BIST_ESP_WDT_TEST_ERR = 9,
} bist_esp_err_t;

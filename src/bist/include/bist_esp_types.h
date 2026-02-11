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
 * @file bist_esp_types.h
 * @brief Common types and error codes for ESP-BIST library
 */

#pragma once

/**
 * @brief Assembly instruction macro
 *
 * Inserts inline assembly with volatile qualifier to prevent compiler optimization.
 *
 * @param x Assembly instruction string
 */
#define ASM(x)                __asm volatile(x)

/**
 * @brief Add assembly label for debugging and fault injection
 *
 * Creates a labeled position in assembly code that can be used with GDB breakpoints
 * for test fault injection.
 *
 * @param label Label name
 */
#define BIST_ADD_LABEL(label) ASM(#label ":")

/**
 * @brief BIST test result codes
 *
 * Error codes returned by BIST test functions to indicate success or specific failure types.
 */
typedef enum {
    BIST_ESP_OK = 0,                    /**< Test passed successfully */
    BIST_ESP_CPU_TEST_ERR = 1,          /**< CPU register test failed */
    BIST_ESP_CPU_CSR_TEST_ERR = 2,      /**< CPU CSR (Control and Status Register) test failed */
    BIST_ESP_RAM_TEST_ERR = 3,          /**< RAM integrity test failed (March A or March X) */
    BIST_ESP_FLASH_TEST_ERR = 4,        /**< Flash CRC validation test failed */
    BIST_ESP_PC_TEST_ERR = 5,           /**< Program Counter integrity test failed */
    BIST_ESP_CLOCK_TEST_ERR = 6,        /**< Clock integrity test failed (crystal or frequency) */
    BIST_ESP_STACK_TEST_ERR = 7,        /**< Stack overflow detection test failed */
    BIST_ESP_STACK_TEST_OVERFLOW = 8,   /**< Stack overflow detected */
    BIST_ESP_WDT_TEST_ERR = 9,          /**< Watchdog timer test failed */
    BIST_ESP_IO_TEST_ERR =  10,         /**< GPIO input/output test failed */
} bist_esp_err_t;

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
 * @file bist_esp.h
 * @brief Main include file for ESP-BIST (Built-In Self Test) library
 *
 * This header includes all core BIST test modules for Espressif ESP32-C3/C6 SoCs.
 * Include this single header to access all BIST functionality.
 *
 * The ESP-BIST library provides IEC 60730 Class B compliant self-test functions for:
 * - CPU register and CSR integrity
 * - RAM testing (March A and March X algorithms)
 * - Flash CRC validation
 * - Program Counter integrity
 * - Clock monitoring
 * - Stack overflow detection
 * - Watchdog timer operation
 * - GPIO functionality
 */

#pragma once

#include "bist_esp_types.h"
#include "bist_core.h"

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
 * @file bist_interrupt.h
 * @brief Software and hardware interrupt tests (IEC 60730 Table H.1 ID 2)
 *
 * Validates interrupt handling and execution: software IRQ source mapping
 * through the interrupt matrix, and concurrent hardware interrupt delivery
 * via dual timer-group alarms.
 */

#pragma once

#include "stdint.h"
#include "bist_esp_types.h"

/**
 * @brief Test interrupt-matrix mapping for software IRQ sources
 *
 * Exercises the four SoC software interrupt sources ``CPU_INTR_FROM_CPU_0``
 * through ``CPU_INTR_FROM_CPU_3``. Each source is routed to a free CPU
 * interrupt line, triggered eight times, then unmapped. The test verifies
 * the enable mask and that the ISR count matches the number of triggers.
 *
 * @return BIST_ESP_OK if every software source maps, fires, and unmaps correctly
 * @return BIST_ESP_INTERRUPT_TEST_ERR if enable-mask or ISR-count checks fail
 */
bist_esp_err_t bist_interrupt_source_map_test(void);

/**
 * @brief Test concurrent hardware interrupt delivery via dual timer groups
 *
 * Starts GPTimer alarms on TIMG0 (500 us) and TIMG1 (1000 us), routes them
 * to free CPU interrupt lines, and checks that ISR counts follow a 2:1 period
 * ratio with a tolerance of one count for synchronization skew. Requires two
 * Timer Groups (``TIMG_LL_INST_NUM >= 2``).
 *
 * @return BIST_ESP_OK if both timers deliver interrupts at the expected ratio
 * @return BIST_ESP_INTERRUPT_TEST_ERR if setup fails or the period ratio check fails
 */
bist_esp_err_t bist_hardware_interrupt_test(void);

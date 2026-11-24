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
 * @brief Stable API for the software and hardware interrupt tests
 *
 * The STATIC and RECIPROCAL implementations expose the same software
 * source-map and hardware timer test entry points.
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
 * @brief Test interrupt-matrix mapping for software IRQ sources
 *
 * In STATIC mode, walks ``CPU_INTR_FROM_CPU_0`` through
 * ``CPU_INTR_FROM_CPU_3``. Each source is mapped to CPU interrupt line 9,
 * triggered eight times, checked for the enabled and cleared CPU interrupt
 * masks and the expected ISR count, then unmapped.
 *
 * In RECIPROCAL mode on ESP32-P4, performs the same walk with
 * ``CPU_INTR_FROM_CPU_0`` through ``CPU_INTR_FROM_CPU_2`` on both HP cores.
 * ``CPU_INTR_FROM_CPU_3`` remains reserved for the
 * ``bist_multicore_call()`` Core-1 wake interrupt. Each core publishes a
 * fixed-layout evidence frame containing completion, per-source counts, and
 * active and cleared enable masks. Both frames are validated locally and by
 * the other core.
 *
 * @return BIST_ESP_OK if the selected mode completes and all count,
 *         enable-mask, completion, and reciprocal validations pass
 * @return BIST_ESP_INTERRUPT_TEST_ERR on any walk, dispatch, mapping, count,
 *         enable-mask, completion, or evidence-verification failure
 */
bist_esp_err_t bist_interrupt_source_map_test(void);

/**
 * @brief Test hardware interrupt delivery via dual timer groups
 *
 * Uses a TIMG0 alarm at 500 us and a TIMG1 alarm at 1000 us and checks the
 * expected 2:1 ISR count relationship. The test requires two Timer Groups
 * (``TIMG_LL_INST_NUM >= 2``).
 *
 * In STATIC mode, both timer interrupts run concurrently on the executing
 * core. In RECIPROCAL mode on ESP32-P4, the shared timers run sequentially
 * on both HP cores and publish fixed-layout count evidence. Each core's
 * result is validated locally and cross-validated by the other core.
 *
 * @return BIST_ESP_OK if the selected mode completes and all timer-count,
 *         ratio, and reciprocal validations pass
 * @return BIST_ESP_INTERRUPT_TEST_ERR on any timer setup, walk, dispatch,
 *         ratio, or evidence-verification failure
 */
bist_esp_err_t bist_hardware_interrupt_test(void);

#ifdef __cplusplus
}
#endif

#if defined(CONFIG_ESP_BIST_INTERRUPT_TEST_RECIPROCAL)
#include "bist_interrupt_reciprocal.h"
#endif

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

/**
 * @file bist_soc_multicore.h
 * @brief ESP32-P4 SoC-level multi-core bring-up and worker loop definitions
 */

#pragma once

/**
 * @brief Boot and initialize HP Core 1 from HP Core 0.
 *
 * Enables clocks, releases Core 1 from reset, sets the initial boot
 * entry point to __start_cpu1(), and polls until Core 1 signals readiness.
 * Called by HP Core 0 during startup when CONFIG_ESP_BIST_MULTICORE is enabled.
 */
void bist_cpu_start_core1(void);

/**
 * @brief HP Core 1 worker loop.
 *
 * Configures cross-core IPC interrupt handling, enables global interrupts,
 * and enters an infinite loop waiting for dispatched jobs via WFI
 * (Wait For Interrupt). When a job is posted by Core 0, executes the function
 * pointer and clears the pending flag using atomic synchronization.
 *
 * @note This function never returns.
 */
void bist_cpu_worker(void) __attribute__((noreturn));

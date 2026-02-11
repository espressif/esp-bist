/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
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
 * @file bist_metrics.h
 * @brief Performance metrics collection for BIST tests
 *
 * Provides macro-based API for collecting performance metrics using
 * RISC-V Performance Counter CSRs. Metrics include cycles,
 * instructions, hazards, and other microarchitectural events.
 *
 * Uses Performance Counter CSRs:
 * - 0x7e0: PCER (performance counter event register - enables specific events)
 * - 0x7e1: PCMR (performance counter mode register - active/always counting)
 * - 0x7e2: PCCR (performance counter count register - counter value)
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "bist_conf.h"
#include "riscv/csr.h"
#include "bist_log.h"

#define CSR_PCER 0x7e0 /**< Performance Counter Event Register */
#define CSR_PCMR 0x7e1 /**< Performance Counter Mode Register */
#define CSR_PCCR 0x7e2 /**< Performance Counter Count Register */

/**
 * @brief Performance counter event modes
 *
 * Defines which microarchitectural event to monitor. Only one event
 * can be monitored at a time.
 */
typedef enum {
    BIST_METRICS_MODE_CYCLE = 0,        /**< CPU cycles */
    BIST_METRICS_MODE_INST = 1,         /**< Instructions retired */
    BIST_METRICS_MODE_LD_HAZARDS = 2,   /**< Load use hazards */
    BIST_METRICS_MODE_JMP_HAZARDS = 3,  /**< Jump register hazards */
    BIST_METRICS_MODE_IDLE = 4,         /**< Cycles waiting for memory */
    BIST_METRICS_MODE_LOAD = 5,         /**< Load instructions */
    BIST_METRICS_MODE_STORE = 6,        /**< Store instructions */
    BIST_METRICS_MODE_JMP_UNCOND = 7,   /**< Unconditional jumps */
    BIST_METRICS_MODE_BRANCH = 8,       /**< Branch instructions */
    BIST_METRICS_MODE_BRANCH_TAKEN = 9, /**< Branches taken */
    BIST_METRICS_MODE_INST_COMP = 10,   /**< Compressed instructions */
} bist_metrics_mode_t;

/**
 * @brief Metrics collection data structure
 *
 * Holds start and end counter values for a single measurement interval.
 * The valid flag indicates whether a complete measurement was taken.
 */
typedef struct {
    uint32_t start_value; /**< Counter value at measurement start */
    uint32_t end_value;   /**< Counter value at measurement end */
    bool valid;           /**< True if both start and end were captured */
} bist_metrics_t;

/**
 * @brief Initialize performance counter for specific event mode
 *
 * Configures the PMU to monitor the selected event type and resets
 * the counter to zero. Must be called before BIST_METRICS_BEGIN.
 *
 * @param mode_ Event mode from bist_metrics_mode_t enum
 *
 * Example:
 * @code
 * BIST_METRICS_INIT(BIST_METRICS_MODE_CYCLE);
 * @endcode
 */
#define BIST_METRICS_INIT(mode_)                        \
    do {                                                \
        RV_WRITE_CSR(CSR_PCER, (1u << (mode_)));        \
        RV_WRITE_CSR(CSR_PCMR, 1u); /* count active */  \
        RV_WRITE_CSR(CSR_PCCR, 0u);                     \
    } while (0)

/**
 * @brief Start metrics collection
 *
 * Captures the initial counter value and marks metrics as invalid until
 * BIST_METRICS_END is called. Call after BIST_METRICS_INIT.
 *
 * @param metrics_ bist_metrics_t structure to store measurement
 *
 * Example:
 * @code
 * bist_metrics_t my_metrics;
 * BIST_METRICS_BEGIN(my_metrics);
 * // ... code to measure ...
 * BIST_METRICS_END(my_metrics);
 * @endcode
 */
#define BIST_METRICS_BEGIN(metrics_)                    \
    do {                                                \
        (metrics_).valid = false;                       \
        (metrics_).start_value = RV_READ_CSR(CSR_PCCR); \
    } while (0)

/**
 * @brief Stop metrics collection
 *
 * Captures the final counter value and marks the measurement as valid.
 * The delta between start and end represents the measured event count.
 *
 * @param metrics_ bist_metrics_t structure (must have called BIST_METRICS_BEGIN)
 */
#define BIST_METRICS_END(metrics_)                      \
    do {                                                \
        (metrics_).end_value = RV_READ_CSR(CSR_PCCR);   \
        (metrics_).valid = true;                        \
    } while (0)

/**
 * @brief Print metrics to console
 *
 * Outputs measurement results in CI-parseable format:
 * "METRICS: <name> Mode: <mode> delta=<count>"
 *
 * Prints warning if metrics are invalid (incomplete measurement).
 *
 * @param name_ Test name or label string (can be NULL)
 * @param metrics_ bist_metrics_t structure with completed measurement
 *
 * Example output:
 * @code
 * METRICS: ram_test Mode: 0 delta=12345
 * @endcode
 */
#define BIST_METRICS_PRINT(name_, metrics_)                                                                            \
    do {                                                                                                               \
        if ((metrics_).valid) {                                                                                        \
            uint32_t _delta = (metrics_).end_value - (metrics_).start_value;                                           \
            uint32_t _pcer = RV_READ_CSR(CSR_PCER) ;                                                                   \
            uint32_t mode = _pcer ? __builtin_ctz(_pcer) : 0 ;                                                         \
            ESP_LOGI("bist_metrics", "METRICS: %s Mode: %d delta=%u", (name_) ? (name_) : "test", mode, _delta);       \
        }                                                                                                              \
        else {                                                                                                         \
            ESP_LOGW("bist_metrics", "Invalid metrics: %s", (name_) ? (name_) : "test");                               \
        }                                                                                                              \
    }                                                                                                                  \
    while (0)

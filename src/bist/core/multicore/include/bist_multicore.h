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
 * @file bist_multicore.h
 * @brief Portable multi-core function dispatch
 */

#pragma once

#include <stdint.h>
#include "bist_esp_types.h"

/**
 * @brief Function dispatched to an HP core.
 *
 * @param arg Caller-supplied pointer.
 */
typedef void (*bist_cpu_fn_t)(void *arg);

/**
 * @brief Run a function on an HP core and block until it returns.
 *
 * If core_id is the caller, fn runs inline. Only core 0 may post work to core
 * 1. One job may run at a time and nested calls are unsupported. Do not
 * dispatch RAM-march tests because they wipe the callee's live stack. The
 * function must not use UART or logging concurrently with the other core.
 *
 * @param core_id Target HP core.
 * @param fn Function to run; must not be NULL.
 * @param arg Pointer passed to fn. The pointed-to object must remain valid
 *            until this blocking call returns.
 *
 * @return BIST_ESP_OK on success.
 * @return BIST_ESP_MULTICORE_ERR for an invalid core, NULL fn, or reverse
 *         dispatch.
 */
bist_esp_err_t bist_multicore_call(uint32_t core_id, bist_cpu_fn_t fn, void *arg);

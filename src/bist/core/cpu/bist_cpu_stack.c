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

#include "bist_cpu_stack.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp32c3/rom/ets_sys.h"

extern uint32_t _stack_overflow_protection_start;
extern uint32_t _stack_overflow_protection_end;

#define STACK_PATTERN 0xDEADBEEF

void handle_stack_overflow(void)
{
    /* handle the error (e.g., log it, halt the system) */
}

void bist_cpu_stack_recursive(int count)
{
    /* consume stack space by adding a large local array */
    volatile char buffer[128] = { 0 };
    (void)(buffer);

    buffer[0] = count;
    if (count > 0) {
        bist_cpu_stack_recursive(count - 1);
    }
}

bist_esp_err_t bist_cpu_stack_overflow_init(void)
{
    uint32_t *protection_start = (uint32_t *)&_stack_overflow_protection_start;
    *protection_start = STACK_PATTERN;

    return BIST_ESP_OK;
}

bist_esp_err_t bist_cpu_stack_overflow_check(void)
{
    uint32_t *protection_start = (uint32_t *)&_stack_overflow_protection_start;

    if (*protection_start != STACK_PATTERN) {
        /* stack overflow detected */
        handle_stack_overflow();

        return BIST_ESP_STACK_TEST_OVERFLOW;
    }

    return BIST_ESP_OK;
}

bist_esp_err_t bist_cpu_stack_overflow_test(void)
{
    int count_max = 20000;

    bist_cpu_stack_overflow_init();

    BIST_ADD_LABEL("bist_cpu_stack_overflow_data");
    for (int k = 0; k < count_max; k++) {
        /* intentionally cause a stack overflow */
        bist_cpu_stack_recursive(k);

        if (bist_cpu_stack_overflow_check() == BIST_ESP_STACK_TEST_OVERFLOW) {
            return BIST_ESP_OK;
        }
    }

    return BIST_ESP_STACK_TEST_ERR;
}

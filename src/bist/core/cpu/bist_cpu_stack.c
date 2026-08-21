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

extern uint32_t _stack_top;
extern uint32_t _stack_overflow_protection_start;
extern uint32_t _stack_overflow_protection_end;

#define STACK_PROTECTION_PATTERN    0xDEADBEEF
#define STACK_FILL_PATTERN          0xBADC0FFE

void __attribute__((weak)) handle_stack_overflow(void)
{
    /* handle the error (e.g., log it, halt the system) */
}

void __attribute__((optimize("O0"))) bist_cpu_stack_recursive(int count)
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
    /*
     * Fill the entire protection region with the sentinel pattern.
     * A single-word sentinel can fall in an unwritten gap between
     * stack frames (saved regs + padding occupy ~32 bytes per 160-byte
     * frame), making single-point detection unreliable. Filling the
     * full region guarantees at least one word lands in a zeroed buffer.
     */
    uint32_t *p   = (uint32_t *)&_stack_overflow_protection_end;
    uint32_t *end = (uint32_t *)&_stack_overflow_protection_start;
    for (; p < end; p++) {
        *p = STACK_PROTECTION_PATTERN;
    }

    return BIST_ESP_OK;
}

bist_esp_err_t bist_cpu_stack_overflow_check(void)
{
    uint32_t *p   = (uint32_t *)&_stack_overflow_protection_end;
    uint32_t *end = (uint32_t *)&_stack_overflow_protection_start;
    for (; p < end; p++) {
        if (*p != STACK_PROTECTION_PATTERN) {
            handle_stack_overflow();
            return BIST_ESP_STACK_TEST_OVERFLOW;
        }
    }

    return BIST_ESP_OK;
}

bist_esp_err_t bist_cpu_stack_overflow_test(void)
{
    volatile int count_max = 20000;

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

uint32_t bist_get_stack_high_watermark(void)
{
    uint32_t *stack_top = (uint32_t *)&_stack_top;
    uint32_t *stack_bottom = (uint32_t *)&_stack_overflow_protection_start;
    uint32_t high_watermark = 0;

    /* We should check after the overflow protection region */
    stack_bottom++;

    for (uint32_t *p = stack_bottom; p < stack_top; p++) {
        if (*p == STACK_FILL_PATTERN) {
            high_watermark++;
        } else {
            break;
        }
    }

    return high_watermark * sizeof(uint32_t);
}

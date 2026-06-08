/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Host-side unit test for xtal_deviation_percent().
 *
 * Build:
 *   cc -std=c99 -Wall -Wextra -I../../src/bist/core/clock/include \
 *      test_xtal_deviation.c -lm -o test_xtal_deviation
 * Run:
 *   ./test_xtal_deviation
 *
 * This test exists because the device-only clock_test cannot run on boards
 * without a 32 kHz crystal (the helper is short-circuited by rtc_clk_cal
 * returning 0 before the math executes). A host-side check protects the
 * arithmetic from regressions independently of the calibration hardware.
 *
 * NOTE on float ABI: the host build uses the platform's native float ABI
 * (typically hard-float x86 SSE), while the device build uses soft-float
 * for targets without an F extension (esp32c3/c6/h2). This test verifies
 * algorithmic correctness, not bit-exact float semantics; the chosen
 * tolerances are loose enough to absorb the ABI difference for the input
 * range used by bist_main_crystal_test().
 */

#include "bist_clock_math.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int failures = 0;

static void check_close(const char *name, float got, float expected, float tol)
{
    float diff = fabsf(got - expected);
    if (diff > tol) {
        fprintf(stderr, "FAIL %s: got %f, expected %f (diff %f > tol %f)\n",
                name, got, expected, diff, tol);
        failures++;
    } else {
        printf("PASS %s: got %f (expected %f)\n", name, got, expected);
    }
}

int main(void)
{
    /* Exact match: zero deviation. */
    check_close("exact_match",
                xtal_deviation_percent(40000000U, 40000000U), 0.0f, 1e-4f);

    /* +1% deviation (actual > expected). */
    check_close("plus_one_percent",
                xtal_deviation_percent(40400000U, 40000000U), 1.0f, 1e-4f);

    /* -1% deviation (actual < expected): this is the path that exercised
     * implementation-defined behavior in the old uint32->int cast. */
    check_close("minus_one_percent",
                xtal_deviation_percent(39600000U, 40000000U), 1.0f, 1e-4f);

    /* -10% deviation. The old (int)(uint32_t-uint32_t) path produced
     * 0xD9_99_99_99 unsigned, which as (int) is implementation-defined. */
    check_close("minus_ten_percent",
                xtal_deviation_percent(36000000U, 40000000U), 10.0f, 1e-3f);

    /* +10% deviation. */
    check_close("plus_ten_percent",
                xtal_deviation_percent(44000000U, 40000000U), 10.0f, 1e-3f);

    /* Tiny drift at the low end: 1 ppm = 0.0001%. */
    check_close("tiny_negative_drift",
                xtal_deviation_percent(40000000U - 40U, 40000000U),
                0.0001f, 1e-6f);

    /* Guard against div-by-zero: expected=0 must return 0, not NaN. */
    check_close("expected_zero",
                xtal_deviation_percent(40000000U, 0U), 0.0f, 1e-6f);

    if (failures) {
        fprintf(stderr, "\n%d test(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    printf("\nAll xtal_deviation_percent tests passed\n");
    return EXIT_SUCCESS;
}

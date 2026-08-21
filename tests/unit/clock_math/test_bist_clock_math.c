/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Host-native unit tests for xtal_deviation_percent().
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

static int g_failures;

static void expect_true(int cond, const char *msg)
{
    if (!cond) {
        printf("FAIL: %s\n", msg);
        g_failures++;
    }
}

static void expect_close(float got, float expected, float tol, const char *msg)
{
    float diff = fabsf(got - expected);
    if (diff > tol) {
        printf("FAIL: %s: got %f, expected %f (diff %f > tol %f)\n",
               msg, (double)got, (double)expected, (double)diff, (double)tol);
        g_failures++;
    }
}

static void test_exact_match(void)
{
    expect_close(xtal_deviation_percent(40000000U, 40000000U),
                 0.0f, 1e-4f, "exact match: zero deviation");
}

static void test_plus_one_percent(void)
{
    expect_close(xtal_deviation_percent(40400000U, 40000000U),
                 1.0f, 1e-4f, "+1% deviation (actual > expected)");
}

/* This is the path that exercised implementation-defined behavior in the
 * old uint32->int cast. */
static void test_minus_one_percent(void)
{
    expect_close(xtal_deviation_percent(39600000U, 40000000U),
                 1.0f, 1e-4f, "-1% deviation (actual < expected)");
}

/* The old (int)(uint32_t-uint32_t) path produced 0xD9_99_99_99 unsigned,
 * which as (int) is implementation-defined. */
static void test_minus_ten_percent(void)
{
    expect_close(xtal_deviation_percent(36000000U, 40000000U),
                 10.0f, 1e-3f, "-10% deviation");
}

static void test_plus_ten_percent(void)
{
    expect_close(xtal_deviation_percent(44000000U, 40000000U),
                 10.0f, 1e-3f, "+10% deviation");
}

static void test_tiny_negative_drift(void)
{
    expect_close(xtal_deviation_percent(40000000U - 40U, 40000000U),
                 0.0001f, 1e-6f, "1 ppm = 0.0001% drift");
}

static void test_expected_zero(void)
{
    expect_true(xtal_deviation_percent(40000000U, 0U) == 0.0f,
                "expected=0 returns 0, not NaN");
}

int main(void)
{
    test_exact_match();
    test_plus_one_percent();
    test_minus_one_percent();
    test_minus_ten_percent();
    test_plus_ten_percent();
    test_tiny_negative_drift();
    test_expected_zero();

    if (g_failures != 0) {
        printf("%d failure(s)\n", g_failures);
        return 1;
    }
    printf("All bist_clock_math tests passed\n");
    return 0;
}

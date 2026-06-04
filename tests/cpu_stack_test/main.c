#include <stdio.h>
#include "rom/ets_sys.h"
#include "bist_esp.h"
#include "unity.h"

extern uint32_t _stack_overflow_protection_start;
extern uint32_t _stack_overflow_protection_end;

#define STACK_PROTECTION_PATTERN    0xDEADBEEF
#define PROBE_PATTERN               0xCAFEBABE

void setUp(void) {}
void tearDown(void) {}

void test_BIST_Cpu_Stack_Overflow(void)
{
    uint8_t ret = bist_cpu_stack_overflow_test();
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

/*
 * Discriminating test for the init/check loop bounds.
 *
 * The protection region is [_end, _start) per the linker script. The init
 * loop must iterate strictly less than _start, so the word at *_start
 * (which lives in stack-bottom territory, not in the protection region)
 * must remain untouched.
 *
 * If the loop uses p <= _start (off-by-one), init writes the sentinel
 * pattern over the probe word and this test fails. With p < _start, the
 * probe word is preserved.
 *
 * The probe sits at the lowest live-stack address. This test is reliable
 * only as long as the current SP is well above the probe so that
 * bist_cpu_stack_overflow_init's own frame does not reach that word.
 * We assert that margin explicitly below.
 */
#define BOUNDS_TEST_MIN_SP_MARGIN  256U
void test_BIST_Cpu_Stack_Overflow_Bounds(void)
{
    volatile uint32_t *probe = (volatile uint32_t *)&_stack_overflow_protection_start;
    register uintptr_t sp asm("sp");

    /* If SP is too close to the probe, init's own frame could overwrite
     * the probe regardless of the loop bound; the test would be unreliable. */
    TEST_ASSERT_GREATER_THAN_UINT32((uintptr_t)probe + BOUNDS_TEST_MIN_SP_MARGIN, sp);

    *probe = PROBE_PATTERN;
    (void)bist_cpu_stack_overflow_init();

    TEST_ASSERT_EQUAL_HEX32(PROBE_PATTERN, *probe);
    TEST_ASSERT_NOT_EQUAL_HEX32(STACK_PROTECTION_PATTERN, *probe);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_BIST_Cpu_Stack_Overflow_Bounds);
    RUN_TEST(test_BIST_Cpu_Stack_Overflow);
    return UNITY_END();
}

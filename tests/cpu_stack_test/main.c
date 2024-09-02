#include <stdio.h>
#include "rom/ets_sys.h"
#include "bist_esp.h"
#include "unity.h"

void test_BIST_Cpu_Stack_Overflow(void)
{
    uint8_t ret = bist_cpu_stack_overflow_test();
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_BIST_Cpu_Stack_Overflow);
    return UNITY_END();
}

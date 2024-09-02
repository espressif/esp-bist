#include <stdio.h>
#include "bist_esp.h"
#include "unity.h"

void test_BIST_PC(void)
{
    uint8_t ret = bist_pc_test();
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_BIST_PC);
    return UNITY_END();
}

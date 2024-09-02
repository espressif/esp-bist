#include <stdio.h>
#include "bist_esp.h"
#include "unity.h"

void test_BIST_WDT(void)
{
    uint8_t ret = bist_wdt_test();
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_BIST_WDT);
    return UNITY_END();
}

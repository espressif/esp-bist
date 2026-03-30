#include <stdio.h>
#include "bist_esp.h"
#include "unity.h"
#include "wdt.h"

void test_BIST_WDT(void)
{
    uint8_t ret = bist_wdt_test();
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

void test_BIST_WDT_INIT_SUB_TICK_TIMEOUT(void)
{
    /* 1 us is always below one RTC tick (e.g. ~31 us at 32768 Hz) */
    TEST_ASSERT_EQUAL_INT(-1, wdt_init(1));
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_BIST_WDT);
    RUN_TEST(test_BIST_WDT_INIT_SUB_TICK_TIMEOUT);
    return UNITY_END();
}

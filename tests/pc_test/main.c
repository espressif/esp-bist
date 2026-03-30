#include <stdio.h>
#include "bist_esp.h"
#include "unity.h"
#include "wdt.h"

void test_BIST_PC(void)
{
    TEST_ASSERT_EQUAL(0, wdt_init(10000));
    bist_esp_err_t ret = bist_pc_test();
    wdt_deinit();
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_BIST_PC);
    return UNITY_END();
}

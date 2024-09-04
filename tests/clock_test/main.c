#include <stdio.h>
#include "rom/ets_sys.h"
#include "bist_esp.h"
#include "unity.h"

void test_BIST_ext_crystal_fail(void)
{
    uint8_t ret = bist_ext_crystal_fail_test();
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

void test_BIST_main_crystal(void)
{
    uint8_t ret = bist_main_crystal_test();
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_BIST_ext_crystal_fail);
    RUN_TEST(test_BIST_main_crystal);
    return UNITY_END();
}

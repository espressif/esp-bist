#include <stdio.h>
#include "rom/ets_sys.h"
#include "bist_esp.h"
#include "unity.h"

void test_BIST_ram_march_a(void)
{
    bist_esp_err_t ret = bist_ram_test_march_a();
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

void test_BIST_ram_march_x(void)
{
    bist_esp_err_t ret = bist_ram_test_march_x();
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_BIST_ram_march_a);
    RUN_TEST(test_BIST_ram_march_x);
    return UNITY_END();
}

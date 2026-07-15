#include <stdio.h>
#include "rom/ets_sys.h"
#include "bist_esp.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

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

void test_BIST_ram_abraham(void)
{
    bist_ram_test_abraham_reset();
    bist_esp_err_t ret = bist_ram_test_abraham();
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

void test_BIST_ram_abraham_full(void)
{
    bist_esp_err_t ret = bist_ram_test_abraham_full();
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_BIST_ram_march_a);
    RUN_TEST(test_BIST_ram_march_x);
    RUN_TEST(test_BIST_ram_abraham);
    RUN_TEST(test_BIST_ram_abraham_full);
    return UNITY_END();
}

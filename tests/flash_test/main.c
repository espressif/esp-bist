#include <stdio.h>
#include "rom/ets_sys.h"
#include "bist_esp.h"
#include "unity.h"

void test_BIST_flash(void)
{
    bist_esp_err_t ret = bist_flash_test();
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_BIST_flash);
    return UNITY_END();
}

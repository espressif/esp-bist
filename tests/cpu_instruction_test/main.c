#include <stdio.h>
#include "rom/ets_sys.h"
#include "bist_esp.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

void test_BIST_Cpu_Instructions(void)
{
    bist_esp_err_t ret = bist_cpu_instruction_test();
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0, bist_cpu_instruction_last_failure_id());
    TEST_ASSERT_EQUAL(BIST_INSN_GROUP_NONE, bist_cpu_instruction_last_failure_group());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_BIST_Cpu_Instructions);
    return UNITY_END();
}

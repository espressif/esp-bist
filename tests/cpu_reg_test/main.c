#include <stdio.h>
#include "rom/ets_sys.h"
#include "bist_esp.h"
#include "unity.h"

void test_BIST_Cpu_Regs(void)
{
    uint8_t ret = bist_cpu_regs_test();
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

void test_BIST_Cpu_Csr_Regs(void)
{
    uint8_t ret = bist_cpu_csr_regs_test();
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_BIST_Cpu_Regs);
    RUN_TEST(test_BIST_Cpu_Csr_Regs);
    return UNITY_END();
}

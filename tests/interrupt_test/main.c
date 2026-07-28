#include "bist_esp.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

void test_BIST_Interrupt_Source_Map(void)
{
    uint8_t ret = bist_interrupt_source_map_test();
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

void test_BIST_Hardware_Interrupt(void)
{
    uint8_t ret = bist_hardware_interrupt_test();
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_BIST_Interrupt_Source_Map);
    RUN_TEST(test_BIST_Hardware_Interrupt);
    return UNITY_END();
}

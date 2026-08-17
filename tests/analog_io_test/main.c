#include <stdio.h>
#include "bist_esp.h"
#include "unity.h"
#include "hal/adc_types.h"
#include "adc_hw_calibration.h"

#define BIST_TEST_ADC_UNIT                      ADC_UNIT_1
#define BIST_TEST_ADC_CHANNEL                   ADC_CHANNEL_2

#if defined(SOC_TARGET_ESP32H4)
#warning "ADC is not supported for ESP32-H4!"

int main()
{
    return 0;
}
#else

void setUp(void)
{
    adc_hw_calibration();
}

void tearDown(void) {}

void test_BIST_ANALOG_IO_INVALID_ADC(void)
{
    bist_esp_err_t ret = bist_adc_low_level_test(SOC_ADC_PERIPH_NUM, ADC_CHANNEL_0);
    TEST_ASSERT_EQUAL(BIST_ESP_ADC_TEST_ERR, ret);
    ret = bist_adc_low_level_test(BIST_TEST_ADC_UNIT, SOC_ADC_CHANNEL_NUM(BIST_TEST_ADC_UNIT));
    TEST_ASSERT_EQUAL(BIST_ESP_ADC_TEST_ERR, ret);
    ret = bist_adc_high_level_test(SOC_ADC_PERIPH_NUM, ADC_CHANNEL_0);
    TEST_ASSERT_EQUAL(BIST_ESP_ADC_TEST_ERR, ret);
    ret = bist_adc_high_level_test(BIST_TEST_ADC_UNIT, SOC_ADC_CHANNEL_NUM(BIST_TEST_ADC_UNIT));
    TEST_ASSERT_EQUAL(BIST_ESP_ADC_TEST_ERR, ret);
#if defined(SOC_TARGET_ESP32C3)
    ret = bist_adc_reference_test(SOC_ADC_PERIPH_NUM, ADC_CHANNEL_0);
    TEST_ASSERT_EQUAL(BIST_ESP_ADC_TEST_ERR, ret);
    ret = bist_adc_reference_test(BIST_TEST_ADC_UNIT, SOC_ADC_CHANNEL_NUM(BIST_TEST_ADC_UNIT));
    TEST_ASSERT_EQUAL(BIST_ESP_ADC_TEST_ERR, ret);
#endif
}

void test_BIST_ANALOG_IO_LOW_LEVEL(void)
{
    bist_esp_err_t ret = bist_adc_low_level_test(BIST_TEST_ADC_UNIT, BIST_TEST_ADC_CHANNEL);
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

void test_BIST_ANALOG_IO_HIGH_LEVEL(void)
{
    bist_esp_err_t ret = bist_adc_high_level_test(BIST_TEST_ADC_UNIT, BIST_TEST_ADC_CHANNEL);
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

#if defined(SOC_TARGET_ESP32C3)
void test_BIST_ANALOG_IO_REFERENCE(void)
{
    bist_esp_err_t ret = bist_adc_reference_test(BIST_TEST_ADC_UNIT, BIST_TEST_ADC_CHANNEL);
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}
#endif

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_BIST_ANALOG_IO_INVALID_ADC);
    RUN_TEST(test_BIST_ANALOG_IO_LOW_LEVEL);
    RUN_TEST(test_BIST_ANALOG_IO_HIGH_LEVEL);
#if defined(SOC_TARGET_ESP32C3)
    RUN_TEST(test_BIST_ANALOG_IO_REFERENCE);
#endif
    return UNITY_END();
}
#endif

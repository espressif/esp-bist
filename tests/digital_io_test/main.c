#include <stdio.h>
#include "bist_esp.h"
#include "unity.h"
#include "hal/gpio_ll.h"

#define BIST_TEST_GPIO_EXT_OUT_IO               (2)
#define BIST_TEST_GPIO_EXT_IN_IO                (3)
#define BIST_TEST_GPIO_SIGNAL_IDX               (SIG_IN_FUNC97_IDX)
#ifdef SOC_TARGET_ESP32C6
#define BIST_TEST_GPIO_INPUT_IO                 (8)
#elif SOC_TARGET_ESP32C3
#define BIST_TEST_GPIO_INPUT_IO                 (9)
#elif SOC_TARGET_ESP32H2
#define BIST_TEST_GPIO_INPUT_IO                 (9)
#endif
#define BIST_TEST_GPIO_INPUT_LEVEL              (1)

void setUp(void) {}
void tearDown(void) {}

void test_BIST_IO_INVALID_GPIO(void)
{
    bist_esp_err_t ret = bist_gpio_output_test(SOC_GPIO_PIN_COUNT);
    TEST_ASSERT_EQUAL(BIST_ESP_IO_TEST_ERR, ret);
    ret = bist_gpio_input_test(SOC_GPIO_PIN_COUNT, 0);
    TEST_ASSERT_EQUAL(BIST_ESP_IO_TEST_ERR, ret);
    ret = bist_gpio_output_test(-1);
    TEST_ASSERT_EQUAL(BIST_ESP_IO_TEST_ERR, ret);
    ret = bist_gpio_input_test(-1, 0);
    TEST_ASSERT_EQUAL(BIST_ESP_IO_TEST_ERR, ret);
}

void test_BIST_IO_OUTPUT_GPIO(void)
{
    bist_esp_err_t ret = bist_gpio_output_test(BIST_TEST_GPIO_EXT_OUT_IO);
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

void test_BIST_IO_INPUT_GPIO(void)
{
    bist_esp_err_t ret = bist_gpio_input_test(BIST_TEST_GPIO_INPUT_IO, BIST_TEST_GPIO_INPUT_LEVEL);
    TEST_ASSERT_EQUAL(BIST_ESP_OK, ret);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_BIST_IO_INVALID_GPIO);
    RUN_TEST(test_BIST_IO_OUTPUT_GPIO);
    RUN_TEST(test_BIST_IO_INPUT_GPIO);
    return UNITY_END();
}

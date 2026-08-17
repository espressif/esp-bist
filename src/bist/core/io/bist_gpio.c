#include "bist_gpio.h"
#include "gpio.h"
#include "bist_esp_types.h"
#include "bist_log.h"

static const char *TAG = "BIST_IO";

static bist_esp_err_t bist_gpio_set_and_check_level(gpio_num_t gpio_num, int set_level)
{
    if (gpio_set_level(gpio_num, set_level) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set GPIO %d to %d", gpio_num, set_level);
        return BIST_ESP_IO_TEST_ERR;
    }

    int level = gpio_get_level(gpio_num);

    if (level != set_level) {
        ESP_LOGE(TAG, "GPIO %d output test failed, expected level: %d, got: %d", gpio_num, set_level, level);
        return BIST_ESP_IO_TEST_ERR;
    }

    return BIST_ESP_OK;
}

bist_esp_err_t bist_gpio_output_test(gpio_num_t gpio_num)
{
    if (!GPIO_IS_VALID_GPIO(gpio_num)) {
        ESP_LOGE(TAG, "GPIO number error: %d", gpio_num);
        return BIST_ESP_IO_TEST_ERR;
    }

    if (gpio_reset_pin(gpio_num) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reset GPIO %d", gpio_num);
        return BIST_ESP_IO_TEST_ERR;
    }

    if (gpio_set_direction(gpio_num, GPIO_MODE_INPUT_OUTPUT) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set GPIO %d as output", gpio_num);
        return BIST_ESP_IO_TEST_ERR;
    }

    if (bist_gpio_set_and_check_level(gpio_num, 0) != BIST_ESP_OK) {
        return BIST_ESP_IO_TEST_ERR;
    }

    if (bist_gpio_set_and_check_level(gpio_num, 1) != BIST_ESP_OK) {
        return BIST_ESP_IO_TEST_ERR;
    }

    if (gpio_reset_pin(gpio_num) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reset GPIO %d", gpio_num);
        return BIST_ESP_IO_TEST_ERR;
    }

    return BIST_ESP_OK;
}

bist_esp_err_t bist_gpio_input_test(gpio_num_t gpio_num, bool expected_level)
{
    if (!GPIO_IS_VALID_GPIO(gpio_num)) {
        ESP_LOGE(TAG, "GPIO number error: %d", gpio_num);
        return BIST_ESP_IO_TEST_ERR;
    }

    if (gpio_reset_pin(gpio_num) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reset GPIO %d", gpio_num);
        return BIST_ESP_IO_TEST_ERR;
    }

    if (gpio_set_direction(gpio_num, GPIO_MODE_INPUT) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set GPIO %d as input", gpio_num);
        return BIST_ESP_IO_TEST_ERR;
    }

    if (gpio_get_level(gpio_num) != expected_level) {
        ESP_LOGE(TAG, "GPIO input test failed for GPIO %d, expected level: %d", gpio_num, expected_level);
        return BIST_ESP_IO_TEST_ERR;
    }

    if (gpio_reset_pin(gpio_num) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reset GPIO %d", gpio_num);
        return BIST_ESP_IO_TEST_ERR;
    }

    return BIST_ESP_OK;
}

#include <stdio.h>
#include "bist_esp.h"
#include "bist_log.h"
#include "wdt.h"
#include "bist_gpio.h"
#include "rom/ets_sys.h"

#define LED_GPIO 7
#define BTN_GPIO 9

static const char *TAG = "sample";

void mwdt_callback(void *args)
{
    ESP_LOGE(TAG, "WDT timeout");
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Configure LED at GPIO %d", LED_GPIO);
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
}

static void configure_button(void)
{
    ESP_LOGI(TAG, "Configure Button at GPIO %d", BTN_GPIO);
    gpio_set_direction(BTN_GPIO, GPIO_MODE_INPUT);
}

static void set_led(bool state)
{
    gpio_set_level(LED_GPIO, state);
}

static bool get_button(void)
{
    return gpio_get_level(BTN_GPIO);
}

static void fail_safe_exit(void)
{
    ESP_LOGE(TAG, "Fail safe exit");
    while (1)
        ;
}

static void runtime_tests(void)
{
    bist_esp_err_t test_err = BIST_ESP_OK;

    ESP_LOGI(TAG, "Running BIST CPU tests");
    test_err = bist_cpu_regs_test();
    if (test_err == BIST_ESP_CPU_TEST_ERR) {
        ESP_LOGE(TAG, "CPU register test failed");
        fail_safe_exit();
    }

    ESP_LOGI(TAG, "Running BIST CPU CSR tests");
    test_err = bist_cpu_csr_regs_test();
    if (test_err == BIST_ESP_CPU_CSR_TEST_ERR) {
        ESP_LOGE(TAG, "CPU CSR register test failed");
        fail_safe_exit();
    }

    ESP_LOGI(TAG, "Running BIST Stack tests");
    test_err = bist_cpu_stack_overflow_test();
    if (test_err == BIST_ESP_STACK_TEST_ERR) {
        ESP_LOGE(TAG, "Stack test failed");
        fail_safe_exit();
    }

    ESP_LOGI(TAG, "Running BIST PC tests");
    test_err = bist_pc_test();
    if (test_err == BIST_ESP_PC_TEST_ERR) {
        ESP_LOGE(TAG, "PC test failed");
        fail_safe_exit();
    }
}

static void post_boot_tests(void)
{
    bist_esp_err_t test_err = BIST_ESP_OK;

    ESP_LOGI(TAG, "Running BIST external clock tests");
    test_err = bist_ext_crystal_fail_test();
    if (test_err == BIST_ESP_CLOCK_TEST_ERR) {
        ESP_LOGE(TAG, "External crystal fail test failed");
        fail_safe_exit();
    }

    ESP_LOGI(TAG, "Running BIST main crystal tests");
    test_err = bist_main_crystal_test();
    if (test_err == BIST_ESP_CLOCK_TEST_ERR) {
        ESP_LOGE(TAG, "Main crystal test failed");
        fail_safe_exit();
    }

    ESP_LOGI(TAG, "Running BIST RAM tests");
    test_err = bist_ram_test_march_a();
    if (test_err == BIST_ESP_RAM_TEST_ERR) {
        ESP_LOGE(TAG, "RAM test failed");
        fail_safe_exit();
    }

    ESP_LOGI(TAG, "Running BIST Flash tests");
    test_err = bist_flash_test();
    if (test_err == BIST_ESP_FLASH_TEST_ERR) {
        ESP_LOGE(TAG, "Flash test failed");
        fail_safe_exit();
    }
}

int main()
{
    ESP_LOGI(TAG, "Input Sample Application!");
    post_boot_tests();
    runtime_tests();

    ESP_LOGI(TAG, "All tests passed!");

    configure_led();
    configure_button();

    while (1) {
        wdt_feed();
        set_led(get_button());
        runtime_tests();
    }

    return 0;
}

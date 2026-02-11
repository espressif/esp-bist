#include <stdio.h>
#include "bist_esp.h"
#include "bist_log.h"
#include "wdt.h"
#include "esp_xt_wdt.h"
#include "gpio.h"
#include "rom/ets_sys.h"
#include "esp_attr.h"

#define LED_GPIO 7
#define BTN_GPIO 9

static const char *TAG = "sample";

static void fail_safe_exit(void)
{
    ESP_LOGE(TAG, "Fail safe exit");
    while (1)
        ;
}

void IRAM_ATTR wdt_callback(void *args)
{
    ESP_LOGE(TAG, "User WDT callback triggered");
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

static void runtime_tests(void)
{
    bist_esp_err_t test_err = BIST_ESP_OK;

    test_err = bist_cpu_regs_test();
    if (test_err == BIST_ESP_CPU_TEST_ERR) {
        ESP_LOGE(TAG, "CPU register test failed");
        fail_safe_exit();
    }

    test_err = bist_cpu_csr_regs_test();
    if (test_err == BIST_ESP_CPU_CSR_TEST_ERR) {
        ESP_LOGE(TAG, "CPU CSR register test failed");
        fail_safe_exit();
    }

    test_err = bist_cpu_stack_overflow_check();
    if (test_err == BIST_ESP_STACK_TEST_OVERFLOW) {
        ESP_LOGE(TAG, "Stack test failed");
        fail_safe_exit();
    }

    test_err = bist_pc_test();
    if (test_err == BIST_ESP_PC_TEST_ERR) {
        ESP_LOGE(TAG, "PC test failed");
        fail_safe_exit();
    }
}

static void post_boot_tests(void)
{
    bist_esp_err_t test_err = BIST_ESP_OK;


    test_err = bist_ext_crystal_fail_test();
    if (test_err == BIST_ESP_CLOCK_TEST_ERR) {
        ESP_LOGE(TAG, "External crystal fail test failed");
        fail_safe_exit();
    }

    test_err = bist_main_crystal_test();
    if (test_err == BIST_ESP_CLOCK_TEST_ERR) {
        ESP_LOGE(TAG, "Main crystal test failed");
        fail_safe_exit();
    }

    test_err = bist_ram_test_march_a();
    if (test_err == BIST_ESP_RAM_TEST_ERR) {
        ESP_LOGE(TAG, "RAM test failed");
        fail_safe_exit();
    }

    test_err = bist_flash_test();
    if (test_err == BIST_ESP_FLASH_TEST_ERR) {
        ESP_LOGE(TAG, "Flash test failed");
        fail_safe_exit();
    }

    test_err = bist_cpu_stack_overflow_test();
    if (test_err == BIST_ESP_STACK_TEST_ERR) {
        ESP_LOGE(TAG, "Stack test failed");
        fail_safe_exit();
    }

    test_err = bist_gpio_output_test(LED_GPIO);
    if (test_err == BIST_ESP_IO_TEST_ERR) {
        ESP_LOGE(TAG, "GPIO output test failed for LED GPIO");
        fail_safe_exit();
    }

    test_err = bist_gpio_input_test(BTN_GPIO, 1);
    if (test_err == BIST_ESP_IO_TEST_ERR) {
        ESP_LOGE(TAG, "GPIO input test failed for Button GPIO");
        fail_safe_exit();
    }

    ESP_LOGI(TAG, "All post boot tests passed! Executed tests: External Crystal, Main Crystal, RAM, Flash, Stack, IO");
}

int test_malloc(void)
{
    int *ptr = malloc(4);
    if (ptr == NULL) {
        ESP_LOGE(TAG, "Memory allocation failed");
        return -1;
    }
    ESP_LOGI(TAG, "Memory allocated at %p", ptr);
    free(ptr);
    return 0;
}

int main()
{
    ESP_LOGI(TAG, "BIST Input Sample Application!");

    post_boot_tests();

    bist_cpu_stack_overflow_init();

    // Register Crystal WDT and Master WDT callbacks
    esp_xt_wdt_register_callback((esp_xt_callback_t)fail_safe_exit, NULL);
    wdt_register_callback(wdt_callback, NULL);

    runtime_tests();

    configure_led();
    configure_button();

    test_malloc();

    ESP_LOGI(TAG, "Initializing WDT");
    wdt_init(CONFIG_ESP_BIST_WDT_TIMEOUT_US);
    wdt_init_windowed(CONFIG_ESP_BIST_WDT_WINDOWED_UNDERFLOW_TIMEOUT_US);

    while (1) {
        set_led(get_button());
        runtime_tests();
        wdt_feed();
    }

    return 0;
}

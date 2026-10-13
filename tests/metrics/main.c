/*
 * Characterization application for the standalone Module API tables.
 * This is not a qualification test. It does not call bist_wdt_test():
 * that function arms the MWDT and resets the chip.
 */

#include <stdio.h>

#include "adc_hw_calibration.h"
#include "bist_esp.h"
#include "bist_log.h"
#include "bist_metrics.h"
#include "hal/adc_types.h"
#include "sdkconfig.h"
#include "wdt.h"

#define GPIO_OUTPUT_PIN 2
#define GPIO_INPUT_LEVEL 1

#if defined(SOC_TARGET_ESP32C3) || defined(SOC_TARGET_ESP32H2)
#define GPIO_INPUT_PIN 9
#elif defined(SOC_TARGET_ESP32H4)
#define GPIO_INPUT_PIN 34
#else
#define GPIO_INPUT_PIN 8
#endif

#define ADC_UNIT ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_2

#if defined(SOC_TARGET_ESP32C3)
#define METRICS_SOC "esp32c3"
#elif defined(SOC_TARGET_ESP32C5)
#define METRICS_SOC "esp32c5"
#elif defined(SOC_TARGET_ESP32C6)
#define METRICS_SOC "esp32c6"
#elif defined(SOC_TARGET_ESP32C61)
#define METRICS_SOC "esp32c61"
#elif defined(SOC_TARGET_ESP32H2)
#define METRICS_SOC "esp32h2"
#elif defined(SOC_TARGET_ESP32H4)
#define METRICS_SOC "esp32h4"
#elif defined(SOC_TARGET_ESP32P4)
#define METRICS_SOC "esp32p4"
#else
#define METRICS_SOC "unknown"
#endif

static const char *TAG = "metrics test";

#define MEASURE_CHECKED(name, setup, expr, check)                     \
    do {                                                               \
        bist_metrics_t metrics;                                        \
        int ret;                                                       \
        ESP_LOGI(TAG, "BEGIN %s", name);                               \
        setup;                                                         \
        BIST_METRICS_INIT(BIST_METRICS_MODE_CYCLE);                    \
        BIST_METRICS_BEGIN(metrics);                                   \
        ret = (int)(expr);                                             \
        BIST_METRICS_END(metrics);                                     \
        BIST_METRICS_PRINT(name, metrics);                             \
        ESP_LOGI(TAG, "RESULT %s mode=0 ret=%d", name, ret);           \
        setup;                                                         \
        BIST_METRICS_INIT(BIST_METRICS_MODE_INST);                     \
        BIST_METRICS_BEGIN(metrics);                                   \
        ret = (int)(expr);                                             \
        BIST_METRICS_END(metrics);                                     \
        BIST_METRICS_PRINT(name, metrics);                             \
        ESP_LOGI(TAG, "RESULT %s mode=1 ret=%d", name, ret);           \
    } while (0)

#define MEASURE(name, setup, expr) MEASURE_CHECKED(name, setup, expr, 1)

int main(void)
{
    ESP_LOGI(TAG, "METRICS_BANNER soc=%s cpu_mhz=%d ram_partition_words=%d",
             METRICS_SOC,
             CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
             CONFIG_ESP_BIST_RAM_PARTITION_SIZE);
    ESP_LOGI(TAG, "METRICS_PINS gpio_out=%d gpio_in=%d gpio_in_level=%d",
             GPIO_OUTPUT_PIN, GPIO_INPUT_PIN, GPIO_INPUT_LEVEL);
#if !defined(SOC_TARGET_ESP32H4)
    ESP_LOGI(TAG, "METRICS_PINS adc_unit=%d adc_channel=%d", (int)ADC_UNIT, (int)ADC_CHANNEL);
#else
    ESP_LOGI(TAG, "METRICS_PINS adc=not_in_image");
#endif
    ESP_LOGI(TAG, "WDT_PROCEDURE bist_wdt_test timeout_us=10000 wait_us=50000 two_boot=1 not_measured=1");

    MEASURE("bist_cpu_regs_test", (void)0, bist_cpu_regs_test());
    MEASURE("bist_cpu_csr_regs_test", (void)0, bist_cpu_csr_regs_test());
    MEASURE("bist_cpu_stack_overflow_init", (void)0, bist_cpu_stack_overflow_init());
    MEASURE("bist_cpu_stack_overflow_check", (void)0, bist_cpu_stack_overflow_check());
    MEASURE("bist_cpu_stack_overflow_test", (void)0, bist_cpu_stack_overflow_test());
    MEASURE_CHECKED("bist_get_stack_high_watermark", (void)0, bist_get_stack_high_watermark(), 0);
    MEASURE("bist_pc_test", (void)0, bist_pc_test());

    MEASURE("bist_ram_test_march_a", (void)0, bist_ram_test_march_a());
    MEASURE("bist_ram_test_march_x", (void)0, bist_ram_test_march_x());
    MEASURE("bist_ram_test_abraham", bist_ram_test_abraham_reset(), bist_ram_test_abraham());
    MEASURE("bist_flash_test", (void)0, bist_flash_test());

    MEASURE("bist_interrupt_source_map_test", (void)0, bist_interrupt_source_map_test());
    MEASURE("bist_hardware_interrupt_test", (void)0, bist_hardware_interrupt_test());

    MEASURE("bist_ext_crystal_fail_test", (void)0, bist_ext_crystal_fail_test());
    MEASURE("bist_main_crystal_test", (void)0, bist_main_crystal_test());

    MEASURE("bist_gpio_output_test", (void)0, bist_gpio_output_test(GPIO_OUTPUT_PIN));
    MEASURE("bist_gpio_input_test", (void)0, bist_gpio_input_test(GPIO_INPUT_PIN, GPIO_INPUT_LEVEL));

#if !defined(SOC_TARGET_ESP32H4)
    adc_hw_calibration();
    MEASURE("bist_adc_low_level_test", (void)0, bist_adc_low_level_test(ADC_UNIT, ADC_CHANNEL));
    MEASURE("bist_adc_high_level_test", (void)0, bist_adc_high_level_test(ADC_UNIT, ADC_CHANNEL));
#endif
#if defined(SOC_TARGET_ESP32C3)
    MEASURE("bist_adc_reference_test", (void)0, bist_adc_reference_test(ADC_UNIT, ADC_CHANNEL));
#endif

    MEASURE("wdt_init_windowed", wdt_windowed_deinit(),
            wdt_init_windowed(CONFIG_ESP_BIST_WDT_WINDOWED_UNDERFLOW_TIMEOUT_US));
    wdt_windowed_deinit();

    ESP_LOGI(TAG, "NOTE abraham_full is quadratic in the partition count");
    MEASURE("bist_ram_test_abraham_full", bist_ram_test_abraham_reset(), bist_ram_test_abraham_full());

    ESP_LOGI(TAG, "METRICS_DONE soc=%s", METRICS_SOC);
    return 0;
}

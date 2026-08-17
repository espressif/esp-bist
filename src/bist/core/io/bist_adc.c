#include "bist_adc.h"
#include "adc_oneshot.h"
#include "gpio.h"
#include "hal/adc_types.h"
#include "soc/soc_caps.h"
#include "hal/adc_ll.h"
#include "bist_conf.h"
#include "bist_esp_types.h"
#include "bist_log.h"
#include "rom/ets_sys.h"
#include "math.h"

static const char *TAG = "BIST_ADC";

#define BIST_ADC_DEPTH           SOC_ADC_RTC_MAX_BITWIDTH
#define BIST_ADC_MAX_RAW         ((1 << BIST_ADC_DEPTH) - 1)
#define BIST_ADC_TOLERANCE       BIST_ADC_MAX_RAW * CONFIG_ESP_BIST_ADC_PERCENT_DEVIATION / 100

#if defined(SOC_TARGET_ESP32C3)
#define BIST_ADC_HIGH_VAL        BIST_ADC_MAX_RAW
#define BIST_ADC_REFERENCE       1500
#elif defined(SOC_TARGET_ESP32C6)
#define BIST_ADC_HIGH_VAL        3350
#elif defined(SOC_TARGET_ESP32H2)
#define BIST_ADC_HIGH_VAL        3390
#elif defined(SOC_TARGET_ESP32C5)
#define BIST_ADC_HIGH_VAL        3375
#elif defined(SOC_TARGET_ESP32C61)
#define BIST_ADC_HIGH_VAL        3329
#elif defined(SOC_TARGET_ESP32P4)
#define BIST_ADC_HIGH_VAL        3360
#else
#define BIST_ADC_HIGH_VAL        BIST_ADC_MAX_RAW
#endif

static bist_esp_err_t bist_adc_set_pin_pull(adc_unit_t unit, adc_channel_t channel,
                                            gpio_pull_mode_t pull, int *io_num)
{
    if (adc_oneshot_channel_to_io(unit, channel, io_num) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to map ADC%d channel %d to GPIO", unit + 1, channel);
        return BIST_ESP_ADC_TEST_ERR;
    }

    if (gpio_set_pull_mode(*io_num, pull) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set GPIO %d pull mode", *io_num);
        return BIST_ESP_ADC_TEST_ERR;
    }

    return BIST_ESP_OK;
}

bist_esp_err_t bist_adc_low_level_test(adc_unit_t unit, adc_channel_t channel)
{
    adc_oneshot_unit_handle_t adc_handle = NULL;
    int io_num = 0;
    int raw = 0;
    bist_esp_err_t ret = BIST_ESP_ADC_TEST_ERR;

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = unit,
    };
    if (adc_oneshot_new_unit(&init_config, &adc_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ADC%d unit", unit + 1);
        return BIST_ESP_ADC_TEST_ERR;
    }

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_oneshot_config_channel(adc_handle, channel, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC%d channel %d", unit + 1, channel);
        adc_oneshot_del_unit(adc_handle);
        return BIST_ESP_ADC_TEST_ERR;
    }

    if (bist_adc_set_pin_pull(unit, channel, GPIO_PULLDOWN_ONLY, &io_num) != BIST_ESP_OK) {
        adc_oneshot_del_unit(adc_handle);
        return BIST_ESP_ADC_TEST_ERR;
    }

    ets_delay_us(10000);

    if (adc_oneshot_read(adc_handle, channel, &raw) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read ADC%d channel %d", unit + 1, channel);
        goto cleanup;
    }

    ESP_LOGI(TAG, "ADC%d channel %d low raw: %d", unit + 1, channel, raw);

    if (raw > BIST_ADC_TOLERANCE) {
        ESP_LOGE(TAG, "ADC%d channel %d low test failed, raw: %d, expected: 0",
                 unit + 1, channel, raw);
        goto cleanup;
    }

    ret = BIST_ESP_OK;

cleanup:
    gpio_reset_pin(io_num);
    adc_oneshot_del_unit(adc_handle);
    return ret;
}

bist_esp_err_t bist_adc_high_level_test(adc_unit_t unit, adc_channel_t channel)
{
    adc_oneshot_unit_handle_t adc_handle = NULL;
    int io_num = 0;
    int raw = 0;
    bist_esp_err_t ret = BIST_ESP_ADC_TEST_ERR;

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = unit,
    };
    if (adc_oneshot_new_unit(&init_config, &adc_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ADC%d unit", unit + 1);
        return BIST_ESP_ADC_TEST_ERR;
    }

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_oneshot_config_channel(adc_handle, channel, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC%d channel %d", unit + 1, channel);
        adc_oneshot_del_unit(adc_handle);
        return BIST_ESP_ADC_TEST_ERR;
    }

    if (bist_adc_set_pin_pull(unit, channel, GPIO_PULLUP_ONLY, &io_num) != BIST_ESP_OK) {
        adc_oneshot_del_unit(adc_handle);
        return BIST_ESP_ADC_TEST_ERR;
    }

    ets_delay_us(10000);

    if (adc_oneshot_read(adc_handle, channel, &raw) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read ADC%d channel %d", unit + 1, channel);
        goto cleanup;
    }

    ESP_LOGI(TAG, "ADC%d channel %d high raw: %d", unit + 1, channel, raw);

    if (raw < BIST_ADC_HIGH_VAL - BIST_ADC_TOLERANCE) {
        ESP_LOGE(TAG, "ADC%d channel %d high test failed, raw: %d, expected: %d",
                 unit + 1, channel, raw, BIST_ADC_HIGH_VAL);
        goto cleanup;
    }

    ret = BIST_ESP_OK;

cleanup:
    gpio_reset_pin(io_num);
    adc_oneshot_del_unit(adc_handle);

    return ret;
}

#if defined(SOC_TARGET_ESP32C3)
bist_esp_err_t bist_adc_reference_test(adc_unit_t unit, adc_channel_t channel)
{
    if (unit >= SOC_ADC_PERIPH_NUM) {
        ESP_LOGE(TAG, "ADC unit error: %d", unit);
        return BIST_ESP_ADC_TEST_ERR;
    }

    if (channel >= SOC_ADC_CHANNEL_NUM(unit)) {
        ESP_LOGE(TAG, "ADC channel error: %d", channel);
        return BIST_ESP_ADC_TEST_ERR;
    }

    adc_oneshot_unit_handle_t adc_handle = NULL;
    int io_num = 0;
    int raw = 0;
    bist_esp_err_t ret = BIST_ESP_ADC_TEST_ERR;

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = unit,
    };
    if (adc_oneshot_new_unit(&init_config, &adc_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ADC%d unit", unit + 1);
        return BIST_ESP_ADC_TEST_ERR;
    }

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_oneshot_config_channel(adc_handle, channel, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC%d channel %d", unit + 1, channel);
        adc_oneshot_del_unit(adc_handle);
        return BIST_ESP_ADC_TEST_ERR;
    }

    adc_ll_vref_output(unit, channel, true);

    ets_delay_us(10000);

    if (adc_oneshot_read(adc_handle, channel, &raw) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read ADC%d channel %d", unit + 1, channel);
        goto cleanup;
    }

    ESP_LOGD(TAG, "ADC%d channel %d reference raw: %d", unit + 1, channel, raw);

    if (raw < (BIST_ADC_REFERENCE - BIST_ADC_TOLERANCE) || raw > (BIST_ADC_REFERENCE + BIST_ADC_TOLERANCE)) {
        ESP_LOGE(TAG, "ADC%d channel %d reference test failed, raw stuck low: %d, expected: %d",
                 unit + 1, channel, raw, BIST_ADC_REFERENCE);
        goto cleanup;
    }

    ret = BIST_ESP_OK;

cleanup:
    gpio_reset_pin(io_num);
    adc_oneshot_del_unit(adc_handle);

    return ret;
}
#endif

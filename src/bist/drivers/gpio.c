/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * This file is part of Espressif's BIST (Built-In Self Test) Library.
 *
 * BIST library is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * BIST library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with BIST library. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <esp_types.h>
#include "esp_err.h"
#include "bist_gpio.h"
#include "soc/soc.h"
#include "soc/periph_defs.h"

#include "soc/soc_caps.h"
#include "soc/gpio_periph.h"
#include "bist_log.h"
#include "esp_check.h"
#include "hal/gpio_hal.h"
#include "esp_rom_gpio.h"
#include "esp_private/esp_gpio_reserve.h"

static const char *GPIO_TAG = "GPIO";

#define GPIO_CHECK(a, str, ret_val) ESP_RETURN_ON_FALSE(a, ret_val, GPIO_TAG, "%s", str)

#define GPIO_ISR_CORE_ID_UNINIT (3)

typedef struct
{
    gpio_isr_t fn; /*!< isr function */
    void *args;    /*!< isr function args */
} gpio_isr_func_t;

// Used by the IPC call to register the interrupt service routine.
typedef struct
{
    int source;           /*!< ISR source */
    int intr_alloc_flags; /*!< ISR alloc flag */
    void (*fn)(void *);   /*!< ISR function */
    void *arg;            /*!< ISR function args*/
    void *handle;         /*!< ISR handle */
    esp_err_t ret;
} gpio_isr_alloc_t;

typedef struct
{
    gpio_hal_context_t *gpio_hal;
    uint32_t isr_core_id;
    uint64_t isr_clr_on_entry_mask; // for edge-triggered interrupts, interrupt status bits should be cleared before
                                    // entering per-pin handlers
} gpio_context_t;

static gpio_hal_context_t _gpio_hal = { .dev = GPIO_HAL_GET_HW(GPIO_PORT_0) };

static gpio_context_t gpio_context = {
    .gpio_hal = &_gpio_hal,
    .isr_core_id = GPIO_ISR_CORE_ID_UNINIT,
    .isr_clr_on_entry_mask = 0,
};

esp_err_t gpio_pullup_en(gpio_num_t gpio_num)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    gpio_hal_pullup_en(gpio_context.gpio_hal, gpio_num);
    return ESP_OK;
}

esp_err_t gpio_pullup_dis(gpio_num_t gpio_num)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    gpio_hal_pullup_dis(gpio_context.gpio_hal, gpio_num);
    return ESP_OK;
}

esp_err_t gpio_pulldown_en(gpio_num_t gpio_num)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    gpio_hal_pulldown_en(gpio_context.gpio_hal, gpio_num);
    return ESP_OK;
}

esp_err_t gpio_pulldown_dis(gpio_num_t gpio_num)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    gpio_hal_pulldown_dis(gpio_context.gpio_hal, gpio_num);
    return ESP_OK;
}

static esp_err_t gpio_input_disable(gpio_num_t gpio_num)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    gpio_hal_input_disable(gpio_context.gpio_hal, gpio_num);
    return ESP_OK;
}

static esp_err_t gpio_input_enable(gpio_num_t gpio_num)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    gpio_hal_input_enable(gpio_context.gpio_hal, gpio_num);
    return ESP_OK;
}

static esp_err_t gpio_output_disable(gpio_num_t gpio_num)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    gpio_hal_output_disable(gpio_context.gpio_hal, gpio_num);
    return ESP_OK;
}

static esp_err_t gpio_output_enable(gpio_num_t gpio_num)
{
    GPIO_CHECK(GPIO_IS_VALID_OUTPUT_GPIO(gpio_num), "GPIO output gpio_num error", ESP_ERR_INVALID_ARG);
    gpio_hal_output_enable(gpio_context.gpio_hal, gpio_num);
    esp_rom_gpio_connect_out_signal(gpio_num, SIG_GPIO_OUT_IDX, false, false);
    return ESP_OK;
}

static esp_err_t gpio_od_disable(gpio_num_t gpio_num)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    gpio_hal_od_disable(gpio_context.gpio_hal, gpio_num);
    return ESP_OK;
}

static esp_err_t gpio_od_enable(gpio_num_t gpio_num)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    gpio_hal_od_enable(gpio_context.gpio_hal, gpio_num);
    return ESP_OK;
}

esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level)
{
    GPIO_CHECK(GPIO_IS_VALID_OUTPUT_GPIO(gpio_num), "GPIO output gpio_num error", ESP_ERR_INVALID_ARG);
    gpio_hal_set_level(gpio_context.gpio_hal, gpio_num, level);
    return ESP_OK;
}

int gpio_get_level(gpio_num_t gpio_num)
{
    return gpio_hal_get_level(gpio_context.gpio_hal, gpio_num);
}

esp_err_t gpio_set_pull_mode(gpio_num_t gpio_num, gpio_pull_mode_t pull)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    GPIO_CHECK(pull <= GPIO_FLOATING, "GPIO pull mode error", ESP_ERR_INVALID_ARG);
    esp_err_t ret = ESP_OK;

    switch (pull) {
        case GPIO_PULLUP_ONLY:
            gpio_pulldown_dis(gpio_num);
            gpio_pullup_en(gpio_num);
            break;

        case GPIO_PULLDOWN_ONLY:
            gpio_pulldown_en(gpio_num);
            gpio_pullup_dis(gpio_num);
            break;

        case GPIO_PULLUP_PULLDOWN:
            gpio_pulldown_en(gpio_num);
            gpio_pullup_en(gpio_num);
            break;

        case GPIO_FLOATING:
            gpio_pulldown_dis(gpio_num);
            gpio_pullup_dis(gpio_num);
            break;

        default:
            ESP_LOGE(GPIO_TAG, "Unknown pull up/down mode,gpio_num=%u,pull=%u", gpio_num, pull);
            ret = ESP_ERR_INVALID_ARG;
            break;
    }

    return ret;
}

esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);

    if ((GPIO_IS_VALID_OUTPUT_GPIO(gpio_num) != true) && (mode & GPIO_MODE_DEF_OUTPUT)) {
        ESP_LOGE(GPIO_TAG, "io_num=%d can only be input", gpio_num);
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ESP_OK;

    if (mode & GPIO_MODE_DEF_INPUT) {
        gpio_input_enable(gpio_num);
    }
    else {
        gpio_input_disable(gpio_num);
    }

    if (mode & GPIO_MODE_DEF_OUTPUT) {
        gpio_output_enable(gpio_num);
    }
    else {
        gpio_output_disable(gpio_num);
    }

    if (mode & GPIO_MODE_DEF_OD) {
        gpio_od_enable(gpio_num);
    }
    else {
        gpio_od_disable(gpio_num);
    }

    return ret;
}

esp_err_t gpio_config(const gpio_config_t *pGPIOConfig)
{
    uint64_t gpio_pin_mask = (pGPIOConfig->pin_bit_mask);
    uint32_t io_reg = 0;
    uint32_t io_num = 0;
    uint8_t input_en = 0;
    uint8_t output_en = 0;
    uint8_t od_en = 0;
    uint8_t pu_en = 0;
    uint8_t pd_en = 0;

    if (pGPIOConfig->pin_bit_mask == 0 || pGPIOConfig->pin_bit_mask & ~SOC_GPIO_VALID_GPIO_MASK) {
        ESP_LOGE(GPIO_TAG, "GPIO_PIN mask error ");
        return ESP_ERR_INVALID_ARG;
    }

    if (pGPIOConfig->mode & GPIO_MODE_DEF_OUTPUT && pGPIOConfig->pin_bit_mask & ~SOC_GPIO_VALID_OUTPUT_GPIO_MASK) {
        ESP_LOGE(GPIO_TAG, "GPIO can only be used as input mode");
        return ESP_ERR_INVALID_ARG;
    }

    do {
        io_reg = GPIO_PIN_MUX_REG[io_num];

        if (((gpio_pin_mask >> io_num) & BIT(0))) {
            assert(io_reg != (intptr_t)NULL);

            if ((pGPIOConfig->mode) & GPIO_MODE_DEF_INPUT) {
                input_en = 1;
                gpio_input_enable(io_num);
            }
            else {
                gpio_input_disable(io_num);
            }

            if ((pGPIOConfig->mode) & GPIO_MODE_DEF_OD) {
                od_en = 1;
                gpio_od_enable(io_num);
            }
            else {
                gpio_od_disable(io_num);
            }

            if ((pGPIOConfig->mode) & GPIO_MODE_DEF_OUTPUT) {
                output_en = 1;
                gpio_output_enable(io_num);
            }
            else {
                gpio_output_disable(io_num);
            }

            if (pGPIOConfig->pull_up_en) {
                pu_en = 1;
                gpio_pullup_en(io_num);
            }
            else {
                gpio_pullup_dis(io_num);
            }

            if (pGPIOConfig->pull_down_en) {
                pd_en = 1;
                gpio_pulldown_en(io_num);
            }
            else {
                gpio_pulldown_dis(io_num);
            }

            ESP_LOGI(GPIO_TAG,
                "GPIO[%" PRIu32 "]| InputEn: %d| OutputEn: %d| OpenDrain: %d| Pullup: %d| Pulldown: %d",
                io_num, input_en, output_en, od_en, pu_en, pd_en);

            /* By default, all the pins have to be configured as GPIO pins. */
            gpio_hal_iomux_func_sel(io_reg, PIN_FUNC_GPIO);
        }

        io_num++;
    }
    while (io_num < GPIO_PIN_COUNT);

    return ESP_OK;
}

esp_err_t gpio_reset_pin(gpio_num_t gpio_num)
{
    assert(GPIO_IS_VALID_GPIO(gpio_num));
    gpio_config_t cfg = {
        .pin_bit_mask = BIT64(gpio_num),
        .mode = GPIO_MODE_DISABLE,
        // for powersave reasons, the GPIO should not be floating, select pullup
        .pull_up_en = true,
        .pull_down_en = false,
    };
    gpio_config(&cfg);
    return ESP_OK;
}

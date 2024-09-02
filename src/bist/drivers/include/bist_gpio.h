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

#pragma once

#include <stdio.h>
#include <stdbool.h>
#include "sdkconfig.h"
#include "esp_err.h"
// #include "esp_intr_alloc.h"
#include "soc/soc_caps.h"
#include "hal/gpio_types.h"
#include "esp_rom_gpio.h"
// #include "driver/gpio_etm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GPIO_PIN_COUNT (SOC_GPIO_PIN_COUNT)
/// Check whether it is a valid GPIO number
#define GPIO_IS_VALID_GPIO(gpio_num) ((gpio_num >= 0) && (((1ULL << (gpio_num)) & SOC_GPIO_VALID_GPIO_MASK) != 0))
/// Check whether it can be a valid GPIO number of output mode
#define GPIO_IS_VALID_OUTPUT_GPIO(gpio_num)                                                                            \
    ((gpio_num >= 0) && (((1ULL << (gpio_num)) & SOC_GPIO_VALID_OUTPUT_GPIO_MASK) != 0))
/// Check whether it can be a valid digital I/O pad
#define GPIO_IS_VALID_DIGITAL_IO_PAD(gpio_num)                                                                         \
    ((gpio_num >= 0) && (((1ULL << (gpio_num)) & SOC_GPIO_VALID_DIGITAL_IO_PAD_MASK) != 0))

// typedef intr_handle_t gpio_isr_handle_t;

/**
 * @brief GPIO interrupt handler
 *
 * @param arg User registered data
 */
typedef void (*gpio_isr_t)(void *arg);

/**
 * @brief Configuration parameters of GPIO pad for gpio_config function
 */
typedef struct
{
    uint64_t pin_bit_mask;        /*!< GPIO pin: set with bit mask, each bit maps to a GPIO */
    gpio_mode_t mode;             /*!< GPIO mode: set input/output mode                     */
    gpio_pullup_t pull_up_en;     /*!< GPIO pull-up                                         */
    gpio_pulldown_t pull_down_en; /*!< GPIO pull-down                                       */
#if SOC_GPIO_SUPPORT_PIN_HYS_FILTER
    gpio_hys_ctrl_mode_t hys_ctrl_mode; /*!< GPIO hysteresis: hysteresis filter on slope input    */
#endif
} gpio_config_t;

/**
 * @brief GPIO common configuration
 *
 *        Configure GPIO's Mode,pull-up,PullDown,IntrType
 *
 * @param  pGPIOConfig Pointer to GPIO configure struct
 *
 * @return
 *     - ESP_OK success
 *     - ESP_ERR_INVALID_ARG Parameter error
 *
 */
esp_err_t gpio_config(const gpio_config_t *pGPIOConfig);

/**
 * @brief Reset an gpio to default state (select gpio function, enable pullup and disable input and output).
 *
 * @param gpio_num GPIO number.
 *
 * @note This function also configures the IOMUX for this pin to the GPIO
 *       function, and disconnects any other peripheral output configured via GPIO
 *       Matrix.
 *
 * @return Always return ESP_OK.
 */
esp_err_t gpio_reset_pin(gpio_num_t gpio_num);

/**
 * @brief  GPIO set output level
 *
 * @note This function is allowed to be executed when Cache is disabled within ISR context, by enabling
 * `CONFIG_GPIO_CTRL_FUNC_IN_IRAM`
 *
 * @param  gpio_num GPIO number. If you want to set the output level of e.g. GPIO16, gpio_num should be GPIO_NUM_16
 * (16);
 * @param  level Output level. 0: low ; 1: high
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG GPIO number error
 *
 */
esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level);

/**
 * @brief  GPIO get input level
 *
 * @warning If the pad is not configured for input (or input and output) the returned value is always 0.
 *
 * @param  gpio_num GPIO number. If you want to get the logic level of e.g. pin GPIO16, gpio_num should be GPIO_NUM_16
 * (16);
 *
 * @return
 *     - 0 the GPIO input level is 0
 *     - 1 the GPIO input level is 1
 *
 */
int gpio_get_level(gpio_num_t gpio_num);

/**
 * @brief    GPIO set direction
 *
 * Configure GPIO direction,such as output_only,input_only,output_and_input
 *
 * @param  gpio_num  Configure GPIO pins number, it should be GPIO number. If you want to set direction of e.g. GPIO16,
 * gpio_num should be GPIO_NUM_16 (16);
 * @param  mode GPIO direction
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG GPIO error
 *
 */
esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode);

/**
 * @brief  Configure GPIO pull-up/pull-down resistors
 *
 * @note ESP32: Only pins that support both input & output have integrated pull-up and pull-down resistors. Input-only
 * GPIOs 34-39 do not.
 *
 * @param  gpio_num GPIO number. If you want to set pull up or down mode for e.g. GPIO16, gpio_num should be GPIO_NUM_16
 * (16);
 * @param  pull GPIO pull up/down mode.
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG : Parameter error
 *
 */
esp_err_t gpio_set_pull_mode(gpio_num_t gpio_num, gpio_pull_mode_t pull);

/**
 * @brief Enable pull-up on GPIO.
 *
 * @param gpio_num GPIO number
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t gpio_pullup_en(gpio_num_t gpio_num);

/**
 * @brief Disable pull-up on GPIO.
 *
 * @param gpio_num GPIO number
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t gpio_pullup_dis(gpio_num_t gpio_num);

/**
 * @brief Enable pull-down on GPIO.
 *
 * @param gpio_num GPIO number
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t gpio_pulldown_en(gpio_num_t gpio_num);

/**
 * @brief Disable pull-down on GPIO.
 *
 * @param gpio_num GPIO number
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
esp_err_t gpio_pulldown_dis(gpio_num_t gpio_num);

#ifdef __cplusplus
}
#endif

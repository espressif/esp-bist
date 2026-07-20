/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * This file is part of Espressif's BIST (Built-In Self Test) Library.
 *
 * BIST library is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * BIST library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with BIST library. If not, see
 * <https://www.gnu.org/licenses/lgpl-3.0.html>.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "hal/adc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Type of ADC unit handle for oneshot mode
 */
typedef struct adc_oneshot_unit_ctx_t *adc_oneshot_unit_handle_t;

/**
 * @brief ADC oneshot driver initial configurations
 */
typedef struct {
    adc_unit_t unit_id;             ///< ADC unit
    adc_oneshot_clk_src_t clk_src;  ///< Clock source
    adc_ulp_mode_t ulp_mode;        ///< ADC controlled by ULP, see `adc_ulp_mode_t`
} adc_oneshot_unit_init_cfg_t;

/**
 * @brief ADC channel configurations
 */
typedef struct {
    adc_atten_t atten;              ///< ADC attenuation
    adc_bitwidth_t bitwidth;        ///< ADC conversion result bits
} adc_oneshot_chan_cfg_t;

/**
 * @brief Create a handle to a specific ADC unit
 *
 * @note This API is thread-safe. For more details, see ADC programming guide
 *
 * @param[in]  init_config    Driver initial configurations
 * @param[out] ret_unit       ADC unit handle
 *
 * @return
 *        - ESP_OK:              On success
 *        - ESP_ERR_INVALID_ARG: Invalid arguments
 *        - ESP_ERR_NO_MEM:      No memory
 *        - ESP_ERR_NOT_FOUND:   The ADC peripheral to be claimed is already in use
 *        - ESP_FAIL:            Clock source isn't initialised correctly
 */
esp_err_t adc_oneshot_new_unit(const adc_oneshot_unit_init_cfg_t *init_config, adc_oneshot_unit_handle_t *ret_unit);

/**
 * @brief Set ADC oneshot mode required configurations
 *
 * @note This API is thread-safe. For more details, see ADC programming guide
 *
 * @param[in] handle    ADC handle
 * @param[in] channel   ADC channel to be configured
 * @param[in] config    ADC configurations
 *
 * @return
 *        - ESP_OK:              On success
 *        - ESP_ERR_INVALID_ARG: Invalid arguments
 */
esp_err_t adc_oneshot_config_channel(adc_oneshot_unit_handle_t handle, adc_channel_t channel, const adc_oneshot_chan_cfg_t *config);

/**
 * @brief Get one ADC conversion raw result
 *
 * @note This API is thread-safe. For more details, see ADC programming guide
 * @note This API should NOT be called in an ISR context
 *
 * @param[in] handle    ADC handle
 * @param[in] chan      ADC channel
 * @param[out] out_raw  ADC conversion raw result
 *
 * @return
 *        - ESP_OK:                On success
 *        - ESP_ERR_INVALID_ARG:   Invalid arguments
 *        - ESP_ERR_TIMEOUT:       Timeout, the ADC result is invalid
 */
esp_err_t adc_oneshot_read(adc_oneshot_unit_handle_t handle, adc_channel_t chan, int *out_raw);

/**
 * @brief Delete the ADC unit handle
 *
 * @note This API is thread-safe. For more details, see ADC programming guide
 *
 * @param[in] handle    ADC handle
 *
 * @return
 *        - ESP_OK:              On success
 *        - ESP_ERR_INVALID_ARG: Invalid arguments
 *        - ESP_ERR_NOT_FOUND:   The ADC peripheral to be disclaimed isn't in use
 */
esp_err_t adc_oneshot_del_unit(adc_oneshot_unit_handle_t handle);

/**
 * @brief Get ADC channel from the given GPIO number
 *
 * @param[in]  io_num     GPIO number
 * @param[out] unit_id    ADC unit
 * @param[out] channel    ADC channel
 *
 * @return
 *        - ESP_OK:              On success
 *        - ESP_ERR_INVALID_ARG: Invalid argument
 *        - ESP_ERR_NOT_FOUND:   The IO is not a valid ADC pad
 */
esp_err_t adc_oneshot_io_to_channel(int io_num, adc_unit_t * const unit_id, adc_channel_t * const channel);

/**
 * @brief Get GPIO number from the given ADC channel
 *
 * @param[in]  unit_id    ADC unit
 * @param[in]  channel    ADC channel
 * @param[out] io_num     GPIO number
 *
 * @param
 *       - ESP_OK:              On success
 *       - ESP_ERR_INVALID_ARG: Invalid argument
 */
esp_err_t adc_oneshot_channel_to_io(adc_unit_t unit_id, adc_channel_t channel, int * const io_num);

#ifdef __cplusplus
}
#endif

/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "soc/soc_caps.h"

#ifdef __cplusplus
extern "C" {
#endif

#if SOC_ADC_CALIBRATION_V1_SUPPORTED
/**
 * @brief Run ADC hardware calibration (BIST bare-metal override).
 *
 * IDF registers this as a GCC constructor; BIST exposes it explicitly so
 * callers can invoke it when needed (e.g. from adc_oneshot_new_unit()).
 */
void adc_hw_calibration(void);
#endif

#ifdef __cplusplus
}
#endif

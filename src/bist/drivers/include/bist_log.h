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

#include "esp_log.h"

#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#undef ESP_LOGD
#undef ESP_LOGV

#define ESP_LOGE(tag, format, ...) ESP_EARLY_LOGE(tag, format, ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...) ESP_EARLY_LOGW(tag, format, ##__VA_ARGS__)
#define ESP_LOGI(tag, format, ...) ESP_EARLY_LOGI(tag, format, ##__VA_ARGS__)
#define ESP_LOGD(tag, format, ...) ESP_EARLY_LOGD(tag, format, ##__VA_ARGS__)
#define ESP_LOGV(tag, format, ...) ESP_EARLY_LOGV(tag, format, ##__VA_ARGS__)

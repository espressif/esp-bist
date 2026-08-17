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

#include <stdbool.h>
#include "bist_flash.h"
#include "bist_conf.h"
#include "bist_hd_crc.h"
#include "bist_log.h"

// Pointer to CRC32 value
extern uint32_t _crc_section_text_start, _crc_section_data_start;

// Flash sections
extern uint32_t _flash_rodata_start, _flash_rodata_end;
extern uint32_t _flash_text_start, _flash_text_end;

static const char *TAG = "bist.flash";

uint32_t calculate_crc32(uint8_t *data, size_t length)
{
    uint32_t calc_crc = 0;
    size_t processed = 0;

    while (processed < length) {
        size_t chunk_size = length - processed;
        if (chunk_size > CONFIG_BIST_FLASH_TEST_CHUNK_SIZE) {
            chunk_size = CONFIG_BIST_FLASH_TEST_CHUNK_SIZE;
        }

        calc_crc = bist_hd_crc32_update(data + processed, chunk_size, calc_crc);
        processed += chunk_size;
    }

    return calc_crc & 0xFFFFFFFF;
}

/**
 * @brief Run flash test
 */

bist_esp_err_t bist_flash_test(void)
{
    uint32_t crc_data, crc_text = 0;
    volatile size_t crc_section_len = 0;

    ESP_LOGD(TAG, "Flash text CRC addr: %p", &_crc_section_text_start);
    ESP_LOGD(TAG, "flash.text CRC: 0x%lx", _crc_section_text_start);
    ESP_LOGD(TAG, "Flash data CRC addr: %p", &_crc_section_data_start);
    ESP_LOGD(TAG, "flash.rodata CRC: 0x%lx", _crc_section_data_start);

    ESP_LOGD(TAG, "Calculating CRC for flash.rodata section from %p to %p (%d B)", &_flash_rodata_start,
             &_flash_rodata_end, (size_t)(&_flash_rodata_end - &_flash_rodata_start) * 4);

    crc_section_len = (size_t)(&_flash_rodata_end - &_flash_rodata_start) * 4;
    BIST_ADD_LABEL("bist_flash_test_data");
    crc_data = calculate_crc32((uint8_t *)&_flash_rodata_start, crc_section_len);

    ESP_LOGD(TAG, "Calculated CRC for flash.rodata section: 0x%lx", crc_data);
    if (crc_data != _crc_section_data_start) {
        ESP_LOGE(TAG, "CRC for flash.rodata section does not match the expected value");
        return BIST_ESP_FLASH_TEST_ERR;
    }

    ESP_LOGD(TAG, "Calculating CRC for flash.text section from %p to %p (%d B)", &_flash_text_start,
             &_flash_text_end, (size_t)(&_flash_text_end - &_flash_text_start) * 4);

    crc_section_len = (size_t)(&_flash_text_end - &_flash_text_start) * 4;
    BIST_ADD_LABEL("bist_flash_test_text");
    crc_text = calculate_crc32((uint8_t *)&_flash_text_start, crc_section_len);
    ESP_LOGD(TAG, "Calculated CRC for flash.text section: 0x%lx", crc_text);
    if (crc_text != _crc_section_text_start) {
        ESP_LOGE(TAG, "CRC for flash.text section does not match the expected value");
        return BIST_ESP_FLASH_TEST_ERR;
    }

    return BIST_ESP_OK;
}

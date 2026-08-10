/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Shared CRC-32 (poly 0x04C11DB7 / reflected) for Host Diagnostics.
 * Matches scripts/calculate_crc32.py and bist_flash internal_crc32.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Incremental CRC-32 update.
 *
 * @param data Input bytes
 * @param len Length in bytes
 * @param crc Previous CRC (0 to start a new digest)
 * @return Updated CRC-32
 */
uint32_t bist_hd_crc32_update(const uint8_t *data, size_t len, uint32_t crc);

/**
 * @brief CRC-32 of a contiguous buffer (initial crc = 0).
 */
uint32_t bist_hd_crc32(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Host Diagnostics Q&A challenge response.
 *
 * f(challenge, seq) = CRC32({challenge, seq, key}). The key comes from a
 * build-level BIST_HD_CHALLENGE_KEY, else CONFIG_ESP_BIST_HD_CHALLENGE_KEY,
 * else the compile-time default below.
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BIST_HD_CHALLENGE_KEY_DEFAULT
#define BIST_HD_CHALLENGE_KEY_DEFAULT 0xA5A5A5A5u
#endif

/**
 * @brief Compute the expected challenge answer.
 *
 * @param challenge Companion-issued challenge word
 * @param seq Protocol sequence number mixed into the digest
 * @return f(challenge, seq)
 */
uint32_t bist_hd_challenge_answer(uint32_t challenge, uint16_t seq);

#ifdef __cplusplus
}
#endif

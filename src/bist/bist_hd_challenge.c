/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "bist_hd_challenge.h"

#include "bist_hd_crc.h"

/*
 * Unlike the rest of Host Diagnostics, this file is also compiled host-native
 * by tests/unit, where neither bist_conf.h nor sdkconfig.h exists.
 */
#if defined(__has_include)
#if __has_include("sdkconfig.h")
#include "sdkconfig.h"
#endif
#endif

/*
 * A build-level BIST_HD_CHALLENGE_KEY wins over Kconfig, which wins over the
 * compile-time default. The override exists because the two ends of the wire do
 * not always share one Kconfig namespace: ESP-IDF hands the ULP sub-project the
 * application's sdkconfig, so per-image keys (a provisioned product key, or a
 * validation image deliberately built with a key the companion does not share)
 * can only be expressed as a compile definition.
 */
#ifndef BIST_HD_CHALLENGE_KEY
#ifdef CONFIG_ESP_BIST_HD_CHALLENGE_KEY
#define BIST_HD_CHALLENGE_KEY ((uint32_t)CONFIG_ESP_BIST_HD_CHALLENGE_KEY)
#else
#define BIST_HD_CHALLENGE_KEY BIST_HD_CHALLENGE_KEY_DEFAULT
#endif
#endif

uint32_t bist_hd_challenge_answer(uint32_t challenge, uint16_t seq)
{
    uint8_t buf[10];
    uint32_t key = BIST_HD_CHALLENGE_KEY;

    buf[0] = (uint8_t)(challenge);
    buf[1] = (uint8_t)(challenge >> 8);
    buf[2] = (uint8_t)(challenge >> 16);
    buf[3] = (uint8_t)(challenge >> 24);
    buf[4] = (uint8_t)(seq);
    buf[5] = (uint8_t)(seq >> 8);
    buf[6] = (uint8_t)(key);
    buf[7] = (uint8_t)(key >> 8);
    buf[8] = (uint8_t)(key >> 16);
    buf[9] = (uint8_t)(key >> 24);

    return bist_hd_crc32(buf, sizeof(buf));
}

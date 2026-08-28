/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * NuttX ULP companion port: byte-granular lp_core_mailbox + LP cycle counter.
 *
 * NuttX's /dev/lp_mailbox HP driver transfers one byte per
 * lp_core_mailbox_send/receive call, so this port serializes each 32-bit
 * protocol word as 4 little-endian bytes (matching the HP transport).
 */

#include "bist_hd_comp_port.h"

#include "ulp_lp_core_mailbox.h"
#include "ulp_lp_core_utils.h"

#include <stddef.h>

/* LP core runs at 40 MHz on the supported targets. */
#define HD_LP_MHZ 40u

#define MAILBOX_SEND_WAIT (-1)

static lp_mailbox_t s_mailbox;

static int32_t us_to_cycles(int32_t timeout_us)
{
    if (timeout_us < 0) {
        return -1;
    }
    return (int32_t)((uint32_t)timeout_us * HD_LP_MHZ);
}

static int send_word_as_bytes(uint32_t word)
{
    uint8_t b[4] = {
        (uint8_t)(word) & 0xFF,
        (uint8_t)(word >> 8) & 0xFF,
        (uint8_t)(word >> 16) & 0xFF,
        (uint8_t)(word >> 24) & 0xFF,
    };
    size_t i;

    for (i = 0; i < 4; i++) {
        if (lp_core_mailbox_send(s_mailbox, (lp_message_t)b[i],
                                 MAILBOX_SEND_WAIT) != ESP_OK) {
            return -1;
        }
    }
    return 0;
}

static int recv_word_from_bytes(uint32_t *word, int32_t timeout_cycles)
{
    uint8_t b[4];
    size_t i;
    lp_message_t raw;

    for (i = 0; i < 4; i++) {
        if (lp_core_mailbox_receive(s_mailbox, &raw,
                                    timeout_cycles) != ESP_OK) {
            return -1;
        }
        b[i] = (uint8_t)raw;
    }
    *word = ((uint32_t)b[0]) |
            ((uint32_t)b[1] << 8) |
            ((uint32_t)b[2] << 16) |
            ((uint32_t)b[3] << 24);
    return 0;
}

int bist_hd_comp_port_init(void)
{
    return (lp_core_mailbox_init(&s_mailbox, NULL) == ESP_OK) ? 0 : -1;
}

int bist_hd_comp_port_send(const bist_hd_msg_t *msg)
{
    uint32_t words[BIST_HD_WIRE_WORDS_MAX];
    size_t nwords = 0;
    size_t i;

    if (bist_hd_msg_encode(msg, words, BIST_HD_WIRE_WORDS_MAX, &nwords) != 0) {
        return -1;
    }
    for (i = 0; i < nwords; i++) {
        if (send_word_as_bytes(words[i]) != 0) {
            return -1;
        }
    }
    return 0;
}

int bist_hd_comp_port_recv(bist_hd_msg_t *msg, int32_t timeout_us)
{
    uint32_t words[BIST_HD_WIRE_WORDS_MAX];
    int32_t timeout_cycles = us_to_cycles(timeout_us);
    size_t nwords;
    size_t i;

    if (msg == NULL) {
        return -1;
    }

    if (recv_word_from_bytes(&words[0], timeout_cycles) != 0) {
        return -1;
    }

    nwords = bist_hd_frame_nwords(words[0]);
    if (nwords == 0u) {
        return -1;
    }

    for (i = 1; i < nwords; i++) {
        if (recv_word_from_bytes(&words[i], timeout_cycles) != 0) {
            return -1;
        }
    }
    return bist_hd_msg_decode(words, nwords, msg);
}

void bist_hd_comp_port_delay_us(uint32_t us)
{
    ulp_lp_core_delay_us(us);
}

uint32_t bist_hd_comp_port_tick(void)
{
    return ulp_lp_core_get_cpu_cycles();
}

uint32_t bist_hd_comp_port_elapsed_us(uint32_t start_tick)
{
    return (ulp_lp_core_get_cpu_cycles() - start_tick) / HD_LP_MHZ;
}

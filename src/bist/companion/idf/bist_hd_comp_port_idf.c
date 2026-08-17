/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * ESP-IDF ULP companion port: lp_core_mailbox word FIFO + LP cycle counter.
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
        if (lp_core_mailbox_send(s_mailbox, (lp_message_t)words[i], MAILBOX_SEND_WAIT) != ESP_OK) {
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
    lp_message_t raw;

    if (msg == NULL) {
        return -1;
    }

    if (lp_core_mailbox_receive(s_mailbox, &raw, timeout_cycles) != ESP_OK) {
        return -1;
    }
    words[0] = (uint32_t)raw;

    nwords = bist_hd_frame_nwords(words[0]);
    if (nwords == 0u) {
        return -1;
    }

    for (i = 1; i < nwords; i++) {
        if (lp_core_mailbox_receive(s_mailbox, &raw, timeout_cycles) != ESP_OK) {
            return -1;
        }
        words[i] = (uint32_t)raw;
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

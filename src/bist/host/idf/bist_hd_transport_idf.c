/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * ESP-IDF Host Diagnostics transport (lp_core_mailbox).
 */

#include "bist_hd_transport.h"

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lp_core_mailbox.h"

/* Twice the deepest frame; the mailbox can hold at most one frame pending. */
#define LP_MAILBOX_FLUSH_MAX (2u * BIST_HD_WIRE_WORDS_MAX)

static lp_mailbox_t s_mailbox;
static int s_inited;

static TickType_t timeout_to_ticks(int32_t timeout_ms)
{
    if (timeout_ms < 0) {
        return portMAX_DELAY;
    }
    return pdMS_TO_TICKS((uint32_t)timeout_ms);
}

int bist_hd_transport_init(void)
{
    esp_err_t err;

    if (s_inited) {
        return 0;
    }

    err = lp_core_mailbox_init(&s_mailbox, NULL);
    if (err != ESP_OK) {
        return -1;
    }
    s_inited = 1;
    return 0;
}

void bist_hd_transport_flush(void)
{
    lp_message_t raw;
    unsigned int i;

    if (!s_inited) {
        return;
    }

    for (i = 0; i < LP_MAILBOX_FLUSH_MAX; i++) {
        if (lp_core_mailbox_receive(s_mailbox, &raw, 0) != ESP_OK) {
            return;
        }
    }
}

int bist_hd_transport_send(const bist_hd_msg_t *msg, int32_t timeout_ms)
{
    uint32_t words[BIST_HD_WIRE_WORDS_MAX];
    size_t nwords = 0;
    size_t i;
    TickType_t ticks = timeout_to_ticks(timeout_ms);

    if (!s_inited || msg == NULL) {
        return -1;
    }
    if (bist_hd_msg_encode(msg, words, BIST_HD_WIRE_WORDS_MAX, &nwords) != 0) {
        return -1;
    }

    for (i = 0; i < nwords; i++) {
        esp_err_t err = lp_core_mailbox_send(s_mailbox, (lp_message_t)words[i], ticks);
        if (err != ESP_OK) {
            return -1;
        }
    }
    return 0;
}

int bist_hd_transport_recv(bist_hd_msg_t *msg, int32_t timeout_ms)
{
    uint32_t words[BIST_HD_WIRE_WORDS_MAX];
    size_t nwords;
    size_t i;
    TickType_t ticks = timeout_to_ticks(timeout_ms);
    lp_message_t raw;
    esp_err_t err;

    if (!s_inited || msg == NULL) {
        return -1;
    }

    err = lp_core_mailbox_receive(s_mailbox, &raw, ticks);
    if (err != ESP_OK) {
        return -1;
    }
    words[0] = (uint32_t)raw;

    nwords = bist_hd_frame_nwords(words[0]);
    if (nwords == 0u) {
        return -1;
    }

    for (i = 1; i < nwords; i++) {
        err = lp_core_mailbox_receive(s_mailbox, &raw, ticks);
        if (err != ESP_OK) {
            return -1;
        }
        words[i] = (uint32_t)raw;
    }

    return bist_hd_msg_decode(words, nwords, msg);
}

/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Host Diagnostics protocol encode/decode and helpers (pure C, OS-neutral).
 */

#include "bist_hd_protocol.h"

#include <string.h>

static uint32_t pack_hdr(uint8_t type, uint8_t audit_id, uint8_t nwords)
{
    return ((uint32_t)BIST_HD_WIRE_TAG << BIST_HD_HDR_TAG_SHIFT) |
           ((uint32_t)type << BIST_HD_HDR_TYPE_SHIFT) |
           ((uint32_t)audit_id << BIST_HD_HDR_AUDIT_SHIFT) |
           ((uint32_t)nwords << BIST_HD_HDR_NWORDS_SHIFT);
}

int bist_hd_msg_encode(const bist_hd_msg_t *msg, uint32_t *words, size_t max_words,
                       size_t *out_nwords)
{
    size_t nwords;

    if (msg == NULL || words == NULL || out_nwords == NULL) {
        return -1;
    }

    if (msg->type == BIST_HD_MSG_AGENT_READY) {
        if (max_words < 1u) {
            return -1;
        }
        words[0] = BIST_HD_AGENT_READY_MAGIC;
        *out_nwords = 1u;
        return 0;
    }

    nwords = BIST_HD_WIRE_TYPED_WORDS;
    if (max_words < nwords) {
        return -1;
    }

    words[0] = pack_hdr(msg->type, msg->audit_id, (uint8_t)nwords);
    words[1] = (uint32_t)msg->seq;
    words[2] = msg->payload;
    words[3] = msg->deadline_ticks;
    *out_nwords = nwords;
    return 0;
}

int bist_hd_msg_decode(const uint32_t *words, size_t nwords, bist_hd_msg_t *msg)
{
    uint8_t tag;
    uint8_t type;
    uint8_t audit_id;
    uint8_t hdr_nwords;

    if (words == NULL || msg == NULL || nwords == 0u) {
        return -1;
    }

    memset(msg, 0, sizeof(*msg));

    if (nwords == 1u && words[0] == BIST_HD_AGENT_READY_MAGIC) {
        msg->type = BIST_HD_MSG_AGENT_READY;
        return 0;
    }

    tag = BIST_HD_HDR_GET_TAG(words[0]);
    type = BIST_HD_HDR_GET_TYPE(words[0]);
    audit_id = BIST_HD_HDR_GET_AUDIT(words[0]);
    hdr_nwords = BIST_HD_HDR_GET_NWORDS(words[0]);

    if (tag != BIST_HD_WIRE_TAG || hdr_nwords < BIST_HD_WIRE_TYPED_WORDS ||
            hdr_nwords > BIST_HD_WIRE_WORDS_MAX || nwords < hdr_nwords || type == 0u) {
        return -1;
    }

    msg->type = type;
    msg->audit_id = audit_id;
    msg->seq = (uint16_t)(words[1] & BIST_HD_SEQ_MASK);
    msg->payload = words[2];
    msg->deadline_ticks = words[3];
    return 0;
}

size_t bist_hd_frame_nwords(uint32_t word0)
{
    uint8_t hdr_nwords;

    if (word0 == BIST_HD_AGENT_READY_MAGIC) {
        return 1u;
    }

    hdr_nwords = BIST_HD_HDR_GET_NWORDS(word0);
    if (BIST_HD_HDR_GET_TAG(word0) != BIST_HD_WIRE_TAG ||
            hdr_nwords < BIST_HD_WIRE_TYPED_WORDS || hdr_nwords > BIST_HD_WIRE_WORDS_MAX) {
        return 0u;
    }
    return hdr_nwords;
}

bool bist_hd_seq_check(uint16_t expected, uint16_t actual)
{
    return expected == actual;
}

bool bist_hd_seq_is_stale(uint16_t last_accepted, uint16_t incoming)
{
    uint16_t delta = (uint16_t)(incoming - last_accepted);

    /* Equal => duplicate; delta in high half of u16 space => behind / wrap-stale. */
    if (delta == 0u) {
        return true;
    }
    return delta > BIST_HD_SEQ_HALF_RANGE;
}

bool bist_hd_timeout_expired(uint32_t now_ticks, uint32_t deadline_ticks)
{
    return (int32_t)(now_ticks - deadline_ticks) >= 0;
}

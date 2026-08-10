/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Host-native unit tests for bist_hd_protocol encode/decode and seq helpers.
 */

#include "bist_hd_challenge.h"
#include "bist_hd_crc.h"
#include "bist_hd_protocol.h"

#include <stdio.h>
#include <string.h>

static int g_failures;

static void expect_true(int cond, const char *msg)
{
    if (!cond) {
        printf("FAIL: %s\n", msg);
        g_failures++;
    }
}

static void test_agent_ready_roundtrip(void)
{
    bist_hd_msg_t in = {0};
    bist_hd_msg_t out = {0};
    uint32_t words[BIST_HD_WIRE_WORDS_MAX];
    size_t nwords = 0;

    in.type = BIST_HD_MSG_AGENT_READY;
    expect_true(bist_hd_msg_encode(&in, words, BIST_HD_WIRE_WORDS_MAX, &nwords) == 0,
                "encode AGENT_READY");
    expect_true(nwords == 1u, "AGENT_READY is 1 word");
    expect_true(words[0] == BIST_HD_AGENT_READY_MAGIC, "AGENT_READY magic");
    expect_true(bist_hd_msg_decode(words, nwords, &out) == 0, "decode AGENT_READY");
    expect_true(out.type == BIST_HD_MSG_AGENT_READY, "decoded type AGENT_READY");
}

static void test_lp_status_roundtrip(void)
{
    bist_hd_msg_t in = {0};
    bist_hd_msg_t out = {0};
    uint32_t words[BIST_HD_WIRE_WORDS_MAX];
    size_t nwords = 0;
    uint32_t status = BIST_HD_POSTBOOT_ALL_PASS | BIST_HD_BIT_POSTBOOT;

    in.type = BIST_HD_MSG_LP_STATUS;
    in.payload = status;
    expect_true(bist_hd_msg_encode(&in, words, BIST_HD_WIRE_WORDS_MAX, &nwords) == 0,
                "encode LP_STATUS");
    expect_true(nwords == 4u, "LP_STATUS is a 4-word tagged frame");
    expect_true(words[2] == status, "status carried in the payload word");
    expect_true(bist_hd_msg_decode(words, nwords, &out) == 0, "decode LP_STATUS");
    expect_true(out.type == BIST_HD_MSG_LP_STATUS, "decoded LP_STATUS");
    expect_true(out.payload == status, "payload matches");
}

static void test_challenge_roundtrip(void)
{
    bist_hd_msg_t in = {0};
    bist_hd_msg_t out = {0};
    uint32_t words[BIST_HD_WIRE_WORDS_MAX];
    size_t nwords = 0;

    in.type = BIST_HD_MSG_CHALLENGE;
    in.audit_id = BIST_HD_AUDIT_QA;
    in.seq = 0x1234;
    in.payload = 0xA5A5A5A5u;
    in.deadline_ticks = 1000u;

    expect_true(bist_hd_msg_encode(&in, words, BIST_HD_WIRE_WORDS_MAX, &nwords) == 0,
                "encode CHALLENGE");
    expect_true(nwords == 4u, "CHALLENGE is 4 words");
    expect_true(bist_hd_msg_decode(words, nwords, &out) == 0, "decode CHALLENGE");
    expect_true(out.type == BIST_HD_MSG_CHALLENGE, "type");
    expect_true(out.audit_id == BIST_HD_AUDIT_QA, "audit_id");
    expect_true(out.seq == 0x1234, "seq");
    expect_true(out.payload == 0xA5A5A5A5u, "payload");
    expect_true(out.deadline_ticks == 1000u, "deadline");
}

static void test_diag_roundtrip(void)
{
    bist_hd_msg_t req = {0};
    bist_hd_msg_t rsp = {0};
    bist_hd_msg_t out = {0};
    uint32_t words[BIST_HD_WIRE_WORDS_MAX];
    size_t nwords = 0;

    req.type = BIST_HD_MSG_DIAG_REQ;
    req.audit_id = BIST_HD_AUDIT_FLASH;
    req.seq = 7;
    req.deadline_ticks = 500;

    expect_true(bist_hd_msg_encode(&req, words, BIST_HD_WIRE_WORDS_MAX, &nwords) == 0,
                "encode DIAG_REQ");
    expect_true(bist_hd_msg_decode(words, nwords, &out) == 0, "decode DIAG_REQ");
    expect_true(out.type == BIST_HD_MSG_DIAG_REQ && out.audit_id == BIST_HD_AUDIT_FLASH,
                "DIAG_REQ fields");

    rsp.type = BIST_HD_MSG_DIAG_RSP;
    rsp.audit_id = BIST_HD_AUDIT_FLASH;
    rsp.seq = 7;
    rsp.payload = 0xDEADBEEFu; /* CRC value */

    expect_true(bist_hd_msg_encode(&rsp, words, BIST_HD_WIRE_WORDS_MAX, &nwords) == 0,
                "encode DIAG_RSP");
    expect_true(bist_hd_msg_decode(words, nwords, &out) == 0, "decode DIAG_RSP");
    expect_true(out.payload == 0xDEADBEEFu && bist_hd_seq_check(rsp.seq, out.seq),
                "DIAG_RSP payload/seq");
}

static void test_seq_rejection(void)
{
    expect_true(bist_hd_seq_check(10, 10), "seq match");
    expect_true(!bist_hd_seq_check(10, 11), "seq mismatch");

    expect_true(bist_hd_seq_is_stale(10, 10), "duplicate is stale");
    expect_true(!bist_hd_seq_is_stale(10, 11), "next seq accepted");
    expect_true(bist_hd_seq_is_stale(10, 9), "older seq stale");
    /* 0xFFF0 -> 0x0001 is forward (delta = 0x11), not stale */
    expect_true(!bist_hd_seq_is_stale(0xFFF0, 0x0001), "wrap-forward not stale");
    expect_true(bist_hd_seq_is_stale(0x0001, 0xFFF0), "wrap-backward is stale");
}

static void test_timeout_compare(void)
{
    expect_true(!bist_hd_timeout_expired(100, 200), "before deadline");
    expect_true(bist_hd_timeout_expired(200, 200), "at deadline");
    expect_true(bist_hd_timeout_expired(201, 200), "after deadline");
    expect_true(bist_hd_timeout_expired(10, 0xFFFFFFF0u), "tick wrap expired");
}

static void test_decode_rejects_garbage(void)
{
    bist_hd_msg_t out = {0};
    uint32_t words[4] = {0x11111111u, 0, 0, 0};

    expect_true(bist_hd_msg_decode(words, 1, &out) != 0, "reject unknown single word");
    expect_true(bist_hd_msg_decode(NULL, 1, &out) != 0, "reject null words");
    expect_true(bist_hd_msg_decode(words, 0, &out) != 0, "reject zero nwords");
}

static void test_frame_nwords(void)
{
    bist_hd_msg_t in = {0};
    uint32_t words[BIST_HD_WIRE_WORDS_MAX];
    size_t nwords = 0;

    expect_true(bist_hd_frame_nwords(BIST_HD_AGENT_READY_MAGIC) == 1u, "AGENT_READY is 1 word");
    expect_true(bist_hd_frame_nwords(BIST_HD_BIT_POSTBOOT | BIST_HD_BIT_CPU_REG) == 0u,
                "a bare status bitmask starts no frame");
    expect_true(bist_hd_frame_nwords(0x11111111u) == 0u, "garbage has no frame length");

    in.type = BIST_HD_MSG_CHALLENGE;
    in.audit_id = BIST_HD_AUDIT_QA;
    in.seq = 7;
    expect_true(bist_hd_msg_encode(&in, words, BIST_HD_WIRE_WORDS_MAX, &nwords) == 0,
                "encode CHALLENGE");
    expect_true(bist_hd_frame_nwords(words[0]) == nwords, "typed frame length from header");
}

/*
 * A desynced transport can present a CHALLENGE payload as the first word of a
 * frame. While LP_STATUS was untagged, any word with bit 30 or 31 set decoded
 * as a passing BIST result; such a word must now be rejected outright.
 */
static void test_stray_payload_is_not_lp_status(void)
{
    bist_hd_msg_t out = {0};
    uint32_t stray = 0xC0DE1234u;

    expect_true(bist_hd_frame_nwords(stray) == 0u, "stray payload starts no frame");
    expect_true(bist_hd_msg_decode(&stray, 1u, &out) != 0, "stray payload does not decode");
    expect_true(out.type != BIST_HD_MSG_LP_STATUS, "stray payload is not LP_STATUS");
}

static void test_challenge_answer_deterministic(void)
{
    uint32_t a = bist_hd_challenge_answer(0x12345678u, 0x0001);
    uint32_t b = bist_hd_challenge_answer(0x12345678u, 0x0001);
    uint32_t c = bist_hd_challenge_answer(0x12345678u, 0x0002);

    expect_true(a == b, "challenge answer stable");
    expect_true(a != c, "seq changes answer");
    expect_true(a != 0u, "answer non-zero for this input");
}

static void test_safe_state_notify_roundtrip(void)
{
    bist_hd_msg_t in = {0};
    bist_hd_msg_t out = {0};
    uint32_t words[BIST_HD_WIRE_WORDS_MAX];
    size_t nwords = 0;

    in.type = BIST_HD_MSG_SAFE_STATE_NOTIFY;
    in.payload = BIST_HD_STATUS_FAIL;

    expect_true(bist_hd_msg_encode(&in, words, BIST_HD_WIRE_WORDS_MAX, &nwords) == 0,
                "encode SAFE_STATE_NOTIFY");
    expect_true(nwords == 4u, "SAFE_STATE_NOTIFY is 4 words");
    expect_true(bist_hd_msg_decode(words, nwords, &out) == 0, "decode SAFE_STATE_NOTIFY");
    expect_true(out.type == BIST_HD_MSG_SAFE_STATE_NOTIFY, "type SAFE_STATE_NOTIFY");
    expect_true(out.payload == BIST_HD_STATUS_FAIL, "payload FAIL");
}

static void test_wrong_answer_mismatch(void)
{
    uint32_t good = bist_hd_challenge_answer(0xA5A5A5A5u, 3);
    uint32_t bad = good ^ 1u;

    expect_true(good != bad, "injected bad answer differs");
    expect_true(!bist_hd_seq_check(3, 4), "wrong seq rejected");
}

static void test_crc32_known_vector(void)
{
    /* Empty buffer → 0 */
    expect_true(bist_hd_crc32(NULL, 0) == 0u, "crc empty");
    /* "123456789" IEEE CRC-32 = 0xCBF43926 */
    {
        const uint8_t data[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
        expect_true(bist_hd_crc32(data, sizeof(data)) == 0xCBF43926u, "crc vector 123456789");
    }
}

int main(void)
{
    test_agent_ready_roundtrip();
    test_lp_status_roundtrip();
    test_challenge_roundtrip();
    test_diag_roundtrip();
    test_seq_rejection();
    test_timeout_compare();
    test_decode_rejects_garbage();
    test_frame_nwords();
    test_stray_payload_is_not_lp_status();
    test_challenge_answer_deterministic();
    test_safe_state_notify_roundtrip();
    test_wrong_answer_mismatch();
    test_crc32_known_vector();

    if (g_failures != 0) {
        printf("%d failure(s)\n", g_failures);
        return 1;
    }
    printf("All bist_hd_protocol tests passed\n");
    return 0;
}

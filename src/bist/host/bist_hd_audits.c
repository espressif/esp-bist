/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Host-side Q&A challenge handler. Other catalog DIAG audits are deferred.
 */

#include "bist_hd_audits.h"

#include "bist_hd_challenge.h"
#include "bist_hd_transport.h"

#include "bist_conf.h"

int bist_hd_audit_handle_challenge(const bist_hd_msg_t *challenge)
{
    if (challenge == NULL) {
        return -1;
    }

#ifdef CONFIG_ESP_BIST_HD_AUDIT_QA
    bist_hd_msg_t answer = {0};

    answer.type = BIST_HD_MSG_ANSWER;
    answer.audit_id = BIST_HD_AUDIT_QA;
    answer.seq = challenge->seq;
    answer.payload = bist_hd_challenge_answer(challenge->payload, challenge->seq);
    answer.deadline_ticks = challenge->deadline_ticks;

    return bist_hd_transport_send(&answer, 10000);
#else
    (void)challenge;
    return -1;
#endif
}

/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * OS-neutral Host Diagnostic Agent state machine.
 * Transport and OS primitives come from platform adapters (IDF today).
 *
 * Currently handles LP_STATUS and Q&A CHALLENGE. DIAG_REQ catalog audits
 * and checkpoints are deferred. SAFE_STATE_NOTIFY may arrive on the wire;
 * the companion owns safe state (e.g. stops feeding LP WDT) regardless.
 */

#include "bist_hd_agent.h"

#include "bist_hd_audits.h"
#include "bist_hd_platform.h"
#include "bist_hd_protocol.h"
#include "bist_hd_transport.h"

#include "bist_conf.h"

static int s_started;
static int s_have_seq;
static uint16_t s_last_seq;

static int accept_seq(uint16_t seq)
{
    if (s_have_seq && bist_hd_seq_is_stale(s_last_seq, seq)) {
        return 0;
    }
    s_last_seq = seq;
    s_have_seq = 1;
    return 1;
}

static void agent_worker(void *arg)
{
    (void)arg;

    for (;;) {
        bist_hd_msg_t msg;
        int err = bist_hd_transport_recv(&msg, -1);
        if (err != 0) {
            /* Errors can return without blocking (malformed frame, transport
             * down), so yield to keep this high-priority task off a spin.
             */
            bist_hd_platform_sleep_ms(1);
            continue;
        }

        if (msg.type == BIST_HD_MSG_LP_STATUS) {
            (void)bist_hd_platform_lp_status_push(msg.payload);
            continue;
        }

        if (msg.type == BIST_HD_MSG_CHALLENGE) {
            if (!accept_seq(msg.seq)) {
                continue;
            }
            (void)bist_hd_audit_handle_challenge(&msg);
            continue;
        }
    }
}

int bist_hd_agent_start(void)
{
    bist_hd_msg_t ready = {0};

    if (s_started) {
        return 0;
    }

    if (bist_hd_platform_init() != 0) {
        return -1;
    }
    if (bist_hd_transport_init() != 0) {
        return -1;
    }

    bist_hd_transport_flush();

    ready.type = BIST_HD_MSG_AGENT_READY;
    if (bist_hd_transport_send(&ready, 10000) != 0) {
        return -1;
    }

    if (bist_hd_platform_start_worker(agent_worker, NULL) != 0) {
        return -1;
    }

    s_started = 1;
    return 0;
}

int bist_hd_agent_wait_lp_status(uint32_t *status_out, uint32_t expect_flag, int32_t timeout_ms)
{
    uint32_t start_ms;

    if (status_out == NULL) {
        return -1;
    }

    start_ms = bist_hd_platform_time_ms();

    for (;;) {
        uint32_t status = 0;
        int32_t wait_ms = timeout_ms;
        int err;

        if (timeout_ms >= 0) {
            uint32_t elapsed = bist_hd_platform_time_ms() - start_ms;
            if (elapsed >= (uint32_t)timeout_ms) {
                return -1;
            }
            wait_ms = (int32_t)((uint32_t)timeout_ms - elapsed);
        }

        err = bist_hd_platform_lp_status_pop(&status, wait_ms);
        if (err != 0) {
            return err;
        }
        if ((status & expect_flag) != 0u) {
            *status_out = status;
            return 0;
        }
    }
}

int bist_hd_checkpoint_reached(uint32_t checkpoint_id)
{
    /* Checkpoint audit deferred. */
    (void)checkpoint_id;
    return -1;
}

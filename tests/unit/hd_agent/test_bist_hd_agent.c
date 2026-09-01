/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Host-native unit tests for the HP agent's deferred checkpoint send.
 *
 * The agent is compiled from src/bist exactly as it is on target; only the
 * transport, platform and audit adapters are replaced by stubs. The companion
 * is not involved — it has its own off-target suite in tests/unit/hd_companion.
 *
 * Checkpoint sending is deferred: bist_hd_checkpoint_reached() only marks a
 * pending sequence number. The agent worker sends it after accepting a
 * CHALLENGE. These tests start that worker on a host thread and inject
 * mailbox messages through the transport stubs.
 *
 * One scenario per process: the agent has file-scope static state (s_started,
 * s_ckpt_seq) that cannot be reset without a fresh process.
 */

#include "bist_hd_agent.h"
#include "bist_hd_platform.h"
#include "bist_hd_protocol.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define SENT_MAX 16

static bist_hd_msg_t g_sent[SENT_MAX];
static int g_sent_count;
static int g_send_result;

static int g_failures;

static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_cv = PTHREAD_COND_INITIALIZER;
static pthread_t g_worker_tid;
static int g_worker_in_recv;
static bist_hd_msg_t g_inbox;
static int g_inbox_ready;
static uint16_t g_challenge_seq;

/* ----- test helpers ---------------------------------------------------- */

static void expect_true(int cond, const char *msg)
{
    if (!cond) {
        printf("FAIL: %s\n", msg);
        g_failures++;
    }
}

static void expect_eq_int(int got, int want, const char *msg)
{
    if (got != want) {
        printf("FAIL: %s (got %d, want %d)\n", msg, got, want);
        g_failures++;
    }
}

static void expect_eq_u32(uint32_t got, uint32_t want, const char *msg)
{
    if (got != want) {
        printf("FAIL: %s (got 0x%08x, want 0x%08x)\n", msg, got, want);
        g_failures++;
    }
}

/* ----- transport stubs ------------------------------------------------- */

int bist_hd_transport_init(void)
{
    return 0;
}

void bist_hd_transport_flush(void)
{
}

int bist_hd_transport_send(const bist_hd_msg_t *msg, int32_t timeout_ms)
{
    (void)timeout_ms;

    if (g_send_result != 0) {
        return g_send_result;
    }
    if (msg != NULL && g_sent_count < SENT_MAX) {
        g_sent[g_sent_count++] = *msg;
    }
    return 0;
}

int bist_hd_transport_recv(bist_hd_msg_t *msg, int32_t timeout_ms)
{
    (void)timeout_ms;

    pthread_mutex_lock(&g_mu);
    g_worker_in_recv = 1;
    pthread_cond_broadcast(&g_cv);
    while (!g_inbox_ready) {
        pthread_cond_wait(&g_cv, &g_mu);
    }
    *msg = g_inbox;
    g_inbox_ready = 0;
    g_worker_in_recv = 0;
    pthread_cond_broadcast(&g_cv);
    pthread_mutex_unlock(&g_mu);
    return 0;
}

/* ----- platform stubs -------------------------------------------------- */

int bist_hd_platform_init(void)
{
    return 0;
}

static void *worker_entry(void *arg)
{
    bist_hd_worker_fn_t fn = (bist_hd_worker_fn_t)arg;

    fn(NULL);
    return NULL;
}

int bist_hd_platform_start_worker(bist_hd_worker_fn_t fn, void *arg)
{
    (void)arg;

    if (pthread_create(&g_worker_tid, NULL, worker_entry, (void *)fn) != 0) {
        return -1;
    }
    return 0;
}

int bist_hd_platform_lp_status_push(uint32_t status)
{
    (void)status;
    return 0;
}

int bist_hd_platform_lp_status_pop(uint32_t *status_out, int32_t timeout_ms)
{
    (void)status_out;
    (void)timeout_ms;
    return -1;
}

uint32_t bist_hd_platform_time_ms(void)
{
    return 0;
}

void bist_hd_platform_sleep_ms(uint32_t ms)
{
    (void)ms;
}

/* ----- audit stub ------------------------------------------------------ */

int bist_hd_audit_handle_challenge(const bist_hd_msg_t *challenge)
{
    (void)challenge;
    return 0;
}

/* ----- worker mailbox helpers ------------------------------------------ */

static void wait_worker_idle(void)
{
    pthread_mutex_lock(&g_mu);
    while (!g_worker_in_recv || g_inbox_ready) {
        pthread_cond_wait(&g_cv, &g_mu);
    }
    pthread_mutex_unlock(&g_mu);
}

static void deliver(const bist_hd_msg_t *msg)
{
    pthread_mutex_lock(&g_mu);
    g_inbox = *msg;
    g_inbox_ready = 1;
    pthread_cond_broadcast(&g_cv);
    while (g_inbox_ready || !g_worker_in_recv) {
        pthread_cond_wait(&g_cv, &g_mu);
    }
    pthread_mutex_unlock(&g_mu);
}

static void deliver_challenge(void)
{
    bist_hd_msg_t chal = {0};

    chal.type = BIST_HD_MSG_CHALLENGE;
    chal.audit_id = BIST_HD_AUDIT_QA;
    chal.seq = ++g_challenge_seq;
    chal.payload = 0x12345678u;
    deliver(&chal);
}

static void deliver_lp_status(void)
{
    bist_hd_msg_t status = {0};

    status.type = BIST_HD_MSG_LP_STATUS;
    status.payload = BIST_HD_BIT_RUNTIME;
    deliver(&status);
}

static void agent_start(void)
{
    expect_eq_int(bist_hd_agent_start(), 0, "agent_start succeeds");
    wait_worker_idle();
}

/*
 * Return the index of the first CHECKPOINT message in g_sent[], starting at
 * 'from'. Returns -1 when not found.
 */
static int find_checkpoint(int from)
{
    for (int i = from; i < g_sent_count; i++) {
        if (g_sent[i].type == BIST_HD_MSG_CHECKPOINT) {
            return i;
        }
    }
    return -1;
}

/* ----- scenarios ------------------------------------------------------- */

static void scenario_checkpoint_first_id(void)
{
    int idx;

    agent_start();
    expect_eq_int(bist_hd_checkpoint_reached(), 0, "first checkpoint call succeeds");
    expect_true(find_checkpoint(0) < 0, "checkpoint is not sent before a CHALLENGE");

    deliver_lp_status();
    expect_true(find_checkpoint(0) < 0, "LP_STATUS does not flush the pending checkpoint");

    deliver_challenge();

    idx = find_checkpoint(0);
    expect_true(idx >= 0, "a CHECKPOINT message was sent after CHALLENGE");
    if (idx >= 0) {
        expect_eq_u32(g_sent[idx].payload, 1u, "first checkpoint payload is 1");
        expect_eq_u32(g_sent[idx].audit_id, BIST_HD_AUDIT_CHECKPOINT,
                      "audit_id is CHECKPOINT");
    }
}

static void scenario_checkpoint_step_is_one(void)
{
    const int n = 5;
    uint32_t payloads[5];
    int count = 0;

    agent_start();

    for (int i = 0; i < n; i++) {
        expect_eq_int(bist_hd_checkpoint_reached(), 0, "checkpoint call succeeds");
        deliver_challenge();
    }

    for (int i = 0; i < g_sent_count && count < n; i++) {
        if (g_sent[i].type == BIST_HD_MSG_CHECKPOINT) {
            expect_eq_u32(g_sent[i].audit_id, BIST_HD_AUDIT_CHECKPOINT,
                          "audit_id is CHECKPOINT");
            payloads[count++] = g_sent[i].payload;
        }
    }

    expect_eq_int(count, n, "five CHECKPOINT messages captured");

    for (int i = 1; i < count; i++) {
        uint32_t step = payloads[i] - payloads[i - 1];
        expect_eq_u32(step, 1u, "step between consecutive payloads is 1");
    }
}

static void scenario_checkpoint_send_failure_consumes_id(void)
{
    uint32_t before_fail, after_fail;
    int idx;

    agent_start();

    expect_eq_int(bist_hd_checkpoint_reached(), 0, "first call succeeds");
    deliver_challenge();

    idx = find_checkpoint(0);
    expect_true(idx >= 0, "first CHECKPOINT captured");
    before_fail = (idx >= 0) ? g_sent[idx].payload : 0;

    /* Mark pending, then fail the worker's send on the next CHALLENGE. */
    expect_eq_int(bist_hd_checkpoint_reached(), 0, "second call succeeds (deferred)");
    g_send_result = -1;
    deliver_challenge();

    g_send_result = 0;
    expect_eq_int(bist_hd_checkpoint_reached(), 0, "third call succeeds");
    deliver_challenge();

    idx = find_checkpoint(idx + 1);
    expect_true(idx >= 0, "third CHECKPOINT captured");
    after_fail = (idx >= 0) ? g_sent[idx].payload : 0;

    /*
     * send_checkpoint() clears the pending ID before the transport call, so a
     * failed send still consumes it. The gap is 2:
     * before_fail → (before_fail+1 consumed by failure) → after_fail.
     */
    expect_eq_u32(after_fail - before_fail, 2u,
                  "failed send consumes an ID (gap of 2)");
}

int main(int argc, char **argv)
{
    const char *scenario;

    if (argc != 2) {
        printf("usage: %s <scenario>\n", argv[0]);
        return 2;
    }
    scenario = argv[1];

    g_sent_count = 0;
    g_send_result = 0;
    g_challenge_seq = 0;

    if (strcmp(scenario, "checkpoint_first_id") == 0) {
        scenario_checkpoint_first_id();
    } else if (strcmp(scenario, "checkpoint_step_is_one") == 0) {
        scenario_checkpoint_step_is_one();
    } else if (strcmp(scenario, "checkpoint_send_failure_consumes_id") == 0) {
        scenario_checkpoint_send_failure_consumes_id();
    } else {
        printf("unknown scenario '%s'\n", scenario);
        return 2;
    }

    if (g_failures != 0) {
        printf("%s: %d failure(s)\n", scenario, g_failures);
        return 1;
    }
    printf("%s: ok\n", scenario);
    return 0;
}

/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Host-native verdict tests for the LP companion state machine.
 *
 * The companion is compiled from src/bist exactly as it is on target; only the
 * OS port, the LP watchdog and one STL entry point are replaced. A scripted
 * fake agent then plays every way a host can be wrong — bad value, replayed
 * seq, wrong frame type, silence, an answer that lands after the window — so
 * the fail-closed judgment is covered without a device and without any test
 * hook inside the library. Safe state is observed the way the wire shows it:
 * the companion emits SAFE_STATE_NOTIFY.
 *
 * One scenario per process: the companion latches safe state and offers no
 * reset, so each scenario runs as its own ctest case.
 */

#include "bist_cpu_regs.h"
#include "bist_hd_challenge.h"
#include "bist_hd_companion.h"
#include "bist_hd_comp_port.h"
#include "bist_hd_protocol.h"
#include "lp_wdt.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define ROUNDS_MAX 8
#define INBOX_SIZE 8

typedef enum {
    AGENT_CORRECT,
    AGENT_WRONG_VALUE,
    AGENT_WRONG_SEQ,
    AGENT_WRONG_TYPE,
    AGENT_SILENT,
    AGENT_LATE,
    AGENT_OVER_BUDGET,
} agent_mode_t;

static agent_mode_t g_mode;
static bist_esp_err_t g_cpu_regs_result;

static bist_hd_msg_t g_inbox_queue[INBOX_SIZE];
static int g_inbox_head;
static int g_inbox_count;

static uint32_t g_tick;
static uint32_t g_elapsed_us;

static int g_safe_state_notifies;
static int g_lp_status_sent;
static uint32_t g_last_lp_status;
static int g_challenges;
static uint16_t g_challenge_seq[ROUNDS_MAX];
static uint32_t g_challenge_payload[ROUNDS_MAX];
static int g_wdt_inits;
static int g_wdt_feeds;

static int g_failures;

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

static void inbox_push(const bist_hd_msg_t *msg)
{
    if (g_inbox_count >= INBOX_SIZE) {
        printf("FAIL: inbox overflow\n");
        g_failures++;
        return;
    }
    int tail = (g_inbox_head + g_inbox_count) % INBOX_SIZE;
    g_inbox_queue[tail] = *msg;
    g_inbox_count++;
}

static int inbox_pop(bist_hd_msg_t *msg)
{
    if (g_inbox_count == 0) {
        return -1;
    }
    *msg = g_inbox_queue[g_inbox_head];
    g_inbox_head = (g_inbox_head + 1) % INBOX_SIZE;
    g_inbox_count--;
    return 0;
}

static void answer_challenge(const bist_hd_msg_t *req)
{
    bist_hd_msg_t answer = {0};

    if (g_challenges < ROUNDS_MAX) {
        g_challenge_seq[g_challenges] = req->seq;
        g_challenge_payload[g_challenges] = req->payload;
    }
    g_challenges++;

    if (g_mode == AGENT_SILENT) {
        /* Nothing queued, so the companion's receive must time out. */
        return;
    }

    answer.type = (g_mode == AGENT_WRONG_TYPE) ? BIST_HD_MSG_DIAG_RSP : BIST_HD_MSG_ANSWER;
    answer.audit_id = BIST_HD_AUDIT_QA;
    answer.seq = (g_mode == AGENT_WRONG_SEQ) ? (uint16_t)(req->seq - 1u) : req->seq;
    answer.payload = bist_hd_challenge_answer(req->payload, req->seq);
    if (g_mode == AGENT_WRONG_VALUE) {
        answer.payload ^= 1u;
    }
    answer.deadline_ticks = req->deadline_ticks;

    if (g_mode == AGENT_LATE) {
        g_elapsed_us = req->deadline_ticks + 1u;
    } else if (g_mode == AGENT_OVER_BUDGET) {
        g_elapsed_us = (uint32_t)CONFIG_ESP_BIST_HD_IRQ_LATENCY_BUDGET_US + 1u;
    } else {
        g_elapsed_us = 100u;
    }

    inbox_push(&answer);
}

/* --- companion OS port --- */

int bist_hd_comp_port_init(void)
{
    return 0;
}

int bist_hd_comp_port_send(const bist_hd_msg_t *msg)
{
    if (msg == NULL) {
        return -1;
    }

    switch (msg->type) {
    case BIST_HD_MSG_LP_STATUS:
        g_lp_status_sent++;
        g_last_lp_status = msg->payload;
        break;
    case BIST_HD_MSG_SAFE_STATE_NOTIFY:
        g_safe_state_notifies++;
        break;
    case BIST_HD_MSG_CHALLENGE:
        answer_challenge(msg);
        break;
    default:
        break;
    }
    return 0;
}

int bist_hd_comp_port_recv(bist_hd_msg_t *msg, int32_t timeout_us)
{
    (void)timeout_us;

    if (msg == NULL) {
        return -1;
    }
    return inbox_pop(msg);
}

uint32_t bist_hd_comp_port_tick(void)
{
    return ++g_tick;
}

uint32_t bist_hd_comp_port_elapsed_us(uint32_t start_tick)
{
    (void)start_tick;
    return g_elapsed_us;
}

void bist_hd_comp_port_delay_us(uint32_t us)
{
    (void)us;
}

/* --- stubbed target dependencies --- */

void lp_wdt_init(uint32_t timeout_us)
{
    (void)timeout_us;
    g_wdt_inits++;
}

void lp_wdt_feed(void)
{
    g_wdt_feeds++;
}

void lp_wdt_disable(void)
{
}

bist_esp_err_t bist_cpu_regs_test(void)
{
    return g_cpu_regs_result;
}

/* --- scenarios --- */

static void queue_agent_ready(void)
{
    bist_hd_msg_t ready = {0};

    ready.type = BIST_HD_MSG_AGENT_READY;
    inbox_push(&ready);
}

static void queue_checkpoint(uint32_t id)
{
    bist_hd_msg_t cp = {0};

    cp.type = BIST_HD_MSG_CHECKPOINT;
    cp.audit_id = BIST_HD_AUDIT_CHECKPOINT;
    cp.payload = id;
    inbox_push(&cp);
}

static void init_ok(void)
{
    queue_agent_ready();
    expect_eq_int(bist_hd_companion_init(), 0, "companion init");
    expect_eq_int(g_safe_state_notifies, 0, "no safe state after a clean init");
}

static void scenario_ready(void)
{
    queue_agent_ready();

    expect_eq_int(bist_hd_companion_init(), 0, "init accepts AGENT_READY");
    expect_eq_int(g_lp_status_sent, 1, "one post-boot status reported");
    expect_true((g_last_lp_status & BIST_HD_BIT_POSTBOOT) != 0u, "status carries POSTBOOT");
    expect_true((g_last_lp_status & BIST_HD_BIT_CPU_REG) != 0u, "post-boot CPU test passed");
    expect_eq_int(g_wdt_inits, 1, "LP watchdog armed after post-boot");
    expect_eq_int(g_safe_state_notifies, 0, "no safe state");
}

static void scenario_no_ready(void)
{
    /* Nothing queued: AGENT_READY never arrives. */
    expect_eq_int(bist_hd_companion_init(), -1, "init fails without AGENT_READY");
    expect_eq_int(g_safe_state_notifies, 1, "missing AGENT_READY is safe state");
    expect_eq_int(g_wdt_inits, 0, "watchdog not armed on a failed init");
    expect_eq_int(g_lp_status_sent, 0, "no status before the host is known ready");
}

static void scenario_ready_wrong_type(void)
{
    bist_hd_msg_t junk = {0};

    junk.type = BIST_HD_MSG_LP_STATUS;
    junk.payload = BIST_HD_BIT_POSTBOOT;
    inbox_push(&junk);

    expect_eq_int(bist_hd_companion_init(), -1, "init rejects a non-READY first frame");
    expect_eq_int(g_safe_state_notifies, 1, "wrong first frame is safe state");
    expect_eq_int(g_wdt_inits, 0, "watchdog not armed");
}

static void scenario_postboot_fail(void)
{
    g_cpu_regs_result = BIST_ESP_CPU_TEST_ERR;
    queue_agent_ready();

    expect_eq_int(bist_hd_companion_init(), -1, "init fails when post-boot BIST fails");
    expect_eq_int(g_lp_status_sent, 1, "the failing status is still reported");
    expect_true((g_last_lp_status & BIST_HD_BIT_CPU_REG) == 0u, "CPU bit clear in the report");
    expect_eq_int(g_safe_state_notifies, 1, "post-boot failure is safe state");
    expect_eq_int(g_wdt_inits, 0, "watchdog not armed after a failed self-BIST");
}

static void scenario_qa_pass(void)
{
    const int rounds = 3;

    g_mode = AGENT_CORRECT;
    init_ok();

    for (int i = 0; i < rounds; i++) {
        queue_checkpoint((uint32_t)(i + 1));
        expect_eq_int(bist_hd_companion_loop(), 0, "runtime round accepted");
    }

    expect_eq_int(g_challenges, rounds, "one challenge per round");
    expect_eq_int(g_safe_state_notifies, 0, "correct answers keep the companion running");
    expect_eq_int(g_lp_status_sent, 1 + rounds, "one status per round after post-boot");
    expect_true((g_last_lp_status & BIST_HD_BIT_RUNTIME) != 0u, "status carries RUNTIME");
    expect_true(g_wdt_feeds > 0, "watchdog fed while healthy");

    for (int i = 1; i < rounds; i++) {
        expect_true(g_challenge_seq[i] != g_challenge_seq[i - 1], "seq advances per round");
        expect_true(g_challenge_payload[i] != g_challenge_payload[i - 1],
                    "challenge value changes per round");
    }
}

static void scenario_bad_answer(agent_mode_t mode, const char *what)
{
    g_mode = mode;
    init_ok();

    queue_checkpoint(1);
    expect_eq_int(bist_hd_companion_loop(), -1, what);
    expect_eq_int(g_challenges, 1, "the round issued one challenge");
    expect_eq_int(g_safe_state_notifies, 1, "the verdict is safe state");
}

static void scenario_safe_state_latched(void)
{
    int feeds_at_failure;
    int status_at_failure;

    g_mode = AGENT_WRONG_VALUE;
    init_ok();

    queue_checkpoint(1);
    expect_eq_int(bist_hd_companion_loop(), -1, "first round fails");
    feeds_at_failure = g_wdt_feeds;
    status_at_failure = g_lp_status_sent;

    /* A product loop keeps calling in; the companion must stay shut down. */
    expect_eq_int(bist_hd_companion_loop(), 0, "later rounds are a no-op");
    expect_eq_int(g_wdt_feeds, feeds_at_failure, "watchdog no longer fed");
    expect_eq_int(g_lp_status_sent, status_at_failure, "no further status reported");
    expect_eq_int(g_challenges, 1, "no further challenges");
    expect_eq_int(g_safe_state_notifies, 1, "safe state announced once");
}

static void scenario_runtime_fail(void)
{
    g_mode = AGENT_CORRECT;
    init_ok();

    g_cpu_regs_result = BIST_ESP_CPU_TEST_ERR;
    expect_eq_int(bist_hd_companion_loop(), -1, "runtime self-BIST failure fails the round");
    expect_eq_int(g_challenges, 0, "no challenge issued once the companion itself failed");
    expect_eq_int(g_safe_state_notifies, 1, "runtime self-BIST failure is safe state");
}

/* --- Checkpoint scenarios --- */

static void scenario_checkpoint_pass(void)
{
    const int rounds = 3;

    g_mode = AGENT_CORRECT;
    init_ok();

    for (int i = 0; i < rounds; i++) {
        queue_checkpoint((uint32_t)(i + 1));
        expect_eq_int(bist_hd_companion_loop(), 0, "round with checkpoint passes");
    }

    expect_eq_int(g_safe_state_notifies, 0, "checkpoints present, no safe state");
    expect_eq_int(g_challenges, rounds, "all QA challenges completed");
}

static void scenario_checkpoint_missing(void)
{
    g_mode = AGENT_CORRECT;
    init_ok();

    queue_checkpoint(1);
    expect_eq_int(bist_hd_companion_loop(), 0, "first loop with a checkpoint arms tracking");

    /* Armed, no checkpoint → miss_count = 1. With PERIOD_LOOPS=1 the first
     * miss is still within budget. */
    expect_eq_int(bist_hd_companion_loop(), 0, "first miss tolerated (period=1)");
    expect_eq_int(g_safe_state_notifies, 0, "one miss within budget");

    expect_eq_int(bist_hd_companion_loop(), -1, "second consecutive miss triggers safe state");
    expect_eq_int(g_safe_state_notifies, 1, "missing checkpoint is safe state");
}

static void scenario_checkpoint_never(void)
{
    g_mode = AGENT_CORRECT;
    init_ok();

    /* PERIOD_LOOPS=1: one empty loop is tolerated, the next fails. A host
     * that never calls bist_hd_checkpoint_reached() must still fail closed. */
    expect_eq_int(bist_hd_companion_loop(), 0, "first empty loop within budget");
    expect_eq_int(g_safe_state_notifies, 0, "grace loop does not trip safe state");

    expect_eq_int(bist_hd_companion_loop(), -1, "never-received checkpoint triggers safe state");
    expect_eq_int(g_safe_state_notifies, 1, "silent host is safe state");
}

static void scenario_checkpoint_out_of_order(void)
{
    g_mode = AGENT_CORRECT;
    init_ok();

    /* Loop 1: checkpoint id=5 (arms checkpoint tracking). */
    queue_checkpoint(5);
    expect_eq_int(bist_hd_companion_loop(), 0, "first loop with id=5 passes");

    /* Loop 2: checkpoint id=3 < 5 → out of order → safe state. */
    queue_checkpoint(3);
    expect_eq_int(bist_hd_companion_loop(), -1, "out-of-order checkpoint triggers safe state");
    expect_eq_int(g_safe_state_notifies, 1, "out-of-order checkpoint is safe state");
}

static void scenario_checkpoint_wrap(void)
{
    g_mode = AGENT_CORRECT;
    init_ok();

    /* Loop 1: checkpoint near uint32 max (arms tracking). */
    queue_checkpoint(0xFFFFFFFEu);
    expect_eq_int(bist_hd_companion_loop(), 0, "checkpoint near max passes");

    /* Loop 2: checkpoint wraps past 0 → still forward in the number space. */
    queue_checkpoint(0x00000001u);
    expect_eq_int(bist_hd_companion_loop(), 0, "wrapped checkpoint accepted");
    expect_eq_int(g_safe_state_notifies, 0, "wrap-around does not trigger safe state");
}

/* --- IRQ latency scenario --- */

static void scenario_irq_latency_over_budget(void)
{
    g_mode = AGENT_OVER_BUDGET;
    init_ok();

    /*
     * The fake agent sends a correct answer but comp_port_elapsed_us will
     * return budget + 1 (within the window but over the IRQ budget).
     */
    queue_checkpoint(1);

    expect_eq_int(bist_hd_companion_loop(), -1, "over-budget answer triggers safe state");
    expect_eq_int(g_safe_state_notifies, 1, "IRQ latency budget exceeded is safe state");
    expect_eq_int(g_challenges, 1, "challenge was issued");
}

int main(int argc, char **argv)
{
    const char *scenario;

    if (argc != 2) {
        printf("usage: %s <scenario>\n", argv[0]);
        return 2;
    }
    scenario = argv[1];

    g_cpu_regs_result = BIST_ESP_OK;
    g_mode = AGENT_CORRECT;
    g_elapsed_us = 100u;
    g_inbox_head = 0;
    g_inbox_count = 0;

    if (strcmp(scenario, "ready") == 0) {
        scenario_ready();
    } else if (strcmp(scenario, "no_ready") == 0) {
        scenario_no_ready();
    } else if (strcmp(scenario, "ready_wrong_type") == 0) {
        scenario_ready_wrong_type();
    } else if (strcmp(scenario, "postboot_fail") == 0) {
        scenario_postboot_fail();
    } else if (strcmp(scenario, "qa_pass") == 0) {
        scenario_qa_pass();
    } else if (strcmp(scenario, "qa_wrong_value") == 0) {
        scenario_bad_answer(AGENT_WRONG_VALUE, "wrong answer value rejected");
    } else if (strcmp(scenario, "qa_wrong_seq") == 0) {
        scenario_bad_answer(AGENT_WRONG_SEQ, "replayed seq rejected");
    } else if (strcmp(scenario, "qa_wrong_type") == 0) {
        scenario_bad_answer(AGENT_WRONG_TYPE, "wrong frame type rejected");
    } else if (strcmp(scenario, "qa_silent") == 0) {
        scenario_bad_answer(AGENT_SILENT, "silence rejected");
    } else if (strcmp(scenario, "qa_late") == 0) {
        scenario_bad_answer(AGENT_LATE, "answer after the window rejected");
    } else if (strcmp(scenario, "safe_state_latched") == 0) {
        scenario_safe_state_latched();
    } else if (strcmp(scenario, "runtime_fail") == 0) {
        scenario_runtime_fail();
    } else if (strcmp(scenario, "checkpoint_pass") == 0) {
        scenario_checkpoint_pass();
    } else if (strcmp(scenario, "checkpoint_missing") == 0) {
        scenario_checkpoint_missing();
    } else if (strcmp(scenario, "checkpoint_never") == 0) {
        scenario_checkpoint_never();
    } else if (strcmp(scenario, "checkpoint_out_of_order") == 0) {
        scenario_checkpoint_out_of_order();
    } else if (strcmp(scenario, "checkpoint_wrap") == 0) {
        scenario_checkpoint_wrap();
    } else if (strcmp(scenario, "irq_latency_over_budget") == 0) {
        scenario_irq_latency_over_budget();
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

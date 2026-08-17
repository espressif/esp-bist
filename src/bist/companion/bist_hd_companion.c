/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Host Diagnostics companion (LP): self-BIST gate, Q&A challenge schedule,
 * safe-state ownership.
 *
 * OS-neutral: transport and time come from bist_hd_comp_port (IDF ULP or
 * Zephyr lpcore). Catalog DIAG audits (flash/CPU/…) are deferred; only the
 * QA challenge is wired. Logging is kept minimal: LP SRAM is tight (~16 KiB).
 */

#include "bist_hd_companion.h"

#include "bist_cpu_csr_regs.h"
#include "bist_cpu_regs.h"
#include "bist_cpu_stack.h"
#include "bist_esp_types.h"
#include "bist_flash.h"
#include "bist_hd_challenge.h"
#include "bist_hd_comp_port.h"
#include "bist_hd_protocol.h"
#include "bist_log.h"
#include "bist_ram.h"
#include "lp_wdt.h"

#include <stddef.h>
#include <stdint.h>

#include "bist_conf.h"

#ifndef CONFIG_ESP_BIST_HD_AGENT_READY_TIMEOUT_US
#define CONFIG_ESP_BIST_HD_AGENT_READY_TIMEOUT_US 1000000
#endif
#ifndef CONFIG_ESP_BIST_HD_CHALLENGE_WINDOW_US
#define CONFIG_ESP_BIST_HD_CHALLENGE_WINDOW_US 10000
#endif
#ifndef CONFIG_ESP_BIST_WDT_TIMEOUT_US
#define CONFIG_ESP_BIST_WDT_TIMEOUT_US 50000
#endif

static const char *TAG = "hd_comp";

static int s_in_safe_state;
#ifdef CONFIG_ESP_BIST_HD_AUDIT_QA
static uint16_t s_seq;
static uint32_t s_challenge_prng;
#endif

void __attribute__((weak)) bist_hd_safe_state(void)
{
    bist_hd_msg_t notify = {0};

    if (s_in_safe_state) {
        return;
    }
    s_in_safe_state = 1;
    ESP_LOGE(TAG, "safe_state");
    /* Best-effort notify so HP/pytest can observe companion judgment. */
    notify.type = BIST_HD_MSG_SAFE_STATE_NOTIFY;
    notify.payload = BIST_HD_STATUS_FAIL;
    (void)bist_hd_comp_port_send(&notify);
}

static int send_lp_status(uint32_t mask)
{
    bist_hd_msg_t msg = {0};

    msg.type = BIST_HD_MSG_LP_STATUS;
    msg.payload = mask;
    return bist_hd_comp_port_send(&msg);
}

#ifdef CONFIG_ESP_BIST_HD_AUDIT_QA
static uint16_t next_seq(void)
{
    s_seq++;
    if (s_seq == 0u) {
        s_seq = 1u;
    }
    return s_seq;
}

static uint32_t next_challenge(void)
{
    /* xorshift32; seed from the free-running tick counter once. */
    if (s_challenge_prng == 0u) {
        s_challenge_prng = bist_hd_comp_port_tick() | 1u;
    }
    s_challenge_prng ^= s_challenge_prng << 13;
    s_challenge_prng ^= s_challenge_prng >> 17;
    s_challenge_prng ^= s_challenge_prng << 5;
    return s_challenge_prng;
}

static int run_challenge_audit(void)
{
    bist_hd_msg_t req = {0};
    bist_hd_msg_t rsp;
    uint32_t challenge = next_challenge();
    uint16_t seq = next_seq();
    uint32_t t0;
    uint32_t elapsed_us;
    uint32_t window_us = (uint32_t)CONFIG_ESP_BIST_HD_CHALLENGE_WINDOW_US;
    uint32_t expected;

    req.type = BIST_HD_MSG_CHALLENGE;
    req.audit_id = BIST_HD_AUDIT_QA;
    req.seq = seq;
    req.payload = challenge;
    req.deadline_ticks = window_us;

    t0 = bist_hd_comp_port_tick();
    if (bist_hd_comp_port_send(&req) != 0) {
        return -1;
    }
    if (bist_hd_comp_port_recv(&rsp, (int32_t)window_us) != 0) {
        ESP_LOGE(TAG, "chal timeout");
        return -1;
    }
    elapsed_us = bist_hd_comp_port_elapsed_us(t0);

    if (rsp.type != BIST_HD_MSG_ANSWER || !bist_hd_seq_check(seq, rsp.seq)) {
        ESP_LOGE(TAG, "chal bad rsp");
        return -1;
    }
    expected = bist_hd_challenge_answer(challenge, seq);
    if (rsp.payload != expected) {
        ESP_LOGE(TAG, "chal bad and");
        return -1;
    }
    if (elapsed_us > window_us) {
        ESP_LOGE(TAG, "chal late");
        return -1;
    }
    return 0;
}
#endif /* CONFIG_ESP_BIST_HD_AUDIT_QA */

static uint32_t run_postboot_tests(void)
{
    uint32_t mask = 0;

    ESP_LOGD(TAG, "postboot: start");
#ifdef CONFIG_ESP_BIST_CPU_REG_TEST
    ESP_LOGD(TAG, "postboot: cpu_reg");
    if (bist_cpu_regs_test() == BIST_ESP_OK) {
        mask |= BIST_HD_BIT_CPU_REG;
    }
#endif
#ifdef CONFIG_ESP_BIST_CPU_CSR_REG_TEST
    ESP_LOGD(TAG, "postboot: cpu_csr");
    if (bist_cpu_csr_regs_test() == BIST_ESP_OK) {
        mask |= BIST_HD_BIT_CPU_CSR;
    }
#endif
#ifdef CONFIG_ESP_BIST_MEMORY_RAM_TEST
    ESP_LOGD(TAG, "postboot: ram_march_x");
    if (bist_ram_test_march_x() == BIST_ESP_OK) {
        mask |= BIST_HD_BIT_RAM_X;
    }
    ESP_LOGD(TAG, "postboot: ram_abraham_full");
    if (bist_ram_test_abraham_full() == BIST_ESP_OK) {
        mask |= BIST_HD_BIT_ABRAHAM;
    }
#endif
#ifdef CONFIG_ESP_BIST_MEMORY_FLASH_TEST
    ESP_LOGD(TAG, "postboot: flash");
    if (bist_flash_test() == BIST_ESP_OK) {
        mask |= BIST_HD_BIT_FLASH;
    }
#endif
    ESP_LOGD(TAG, "postboot: done mask 0x%08lx", (unsigned long)mask);
    return mask;
}

static uint32_t run_runtime_tests(void)
{
    uint32_t mask = 0;

    ESP_LOGD(TAG, "runtime tests: start");
#ifdef CONFIG_ESP_BIST_CPU_REG_TEST
    ESP_LOGD(TAG, "runtime: cpu_reg");
    if (bist_cpu_regs_test() == BIST_ESP_OK) {
        mask |= BIST_HD_BIT_CPU_REG;
    }
    lp_wdt_feed();
#endif
#ifdef CONFIG_ESP_BIST_CPU_CSR_REG_TEST
    ESP_LOGD(TAG, "runtime: cpu_csr");
    if (bist_cpu_csr_regs_test() == BIST_ESP_OK) {
        mask |= BIST_HD_BIT_CPU_CSR;
    }
    lp_wdt_feed();
#endif
#ifdef CONFIG_ESP_BIST_MEMORY_RAM_TEST
    ESP_LOGD(TAG, "runtime: ram_march_a");
    if (bist_ram_test_march_a() == BIST_ESP_OK) {
        mask |= BIST_HD_BIT_RAM_A;
    }
    lp_wdt_feed();
    ESP_LOGD(TAG, "runtime: ram_abraham");
    if (bist_ram_test_abraham() == BIST_ESP_OK) {
        mask |= BIST_HD_BIT_ABRAHAM;
    }
    lp_wdt_feed();
#endif
#ifdef CONFIG_ESP_BIST_STACK_TEST
    ESP_LOGD(TAG, "runtime: stack");
    if (bist_cpu_stack_overflow_check() == BIST_ESP_OK) {
        mask |= BIST_HD_BIT_STACK;
    }
#endif
    ESP_LOGD(TAG, "runtime tests: done mask 0x%08lx", (unsigned long)mask);
    return mask;
}

static uint32_t expected_postboot_mask(void)
{
    uint32_t m = 0;

#ifdef CONFIG_ESP_BIST_CPU_REG_TEST
    m |= BIST_HD_BIT_CPU_REG;
#endif
#ifdef CONFIG_ESP_BIST_CPU_CSR_REG_TEST
    m |= BIST_HD_BIT_CPU_CSR;
#endif
#ifdef CONFIG_ESP_BIST_MEMORY_RAM_TEST
    m |= BIST_HD_BIT_RAM_X | BIST_HD_BIT_ABRAHAM;
#endif
#ifdef CONFIG_ESP_BIST_MEMORY_FLASH_TEST
    m |= BIST_HD_BIT_FLASH;
#endif
    return m;
}

static uint32_t expected_runtime_mask(void)
{
    uint32_t m = 0;

#ifdef CONFIG_ESP_BIST_CPU_REG_TEST
    m |= BIST_HD_BIT_CPU_REG;
#endif
#ifdef CONFIG_ESP_BIST_CPU_CSR_REG_TEST
    m |= BIST_HD_BIT_CPU_CSR;
#endif
#ifdef CONFIG_ESP_BIST_MEMORY_RAM_TEST
    m |= BIST_HD_BIT_RAM_A | BIST_HD_BIT_ABRAHAM;
#endif
#ifdef CONFIG_ESP_BIST_STACK_TEST
    m |= BIST_HD_BIT_STACK;
#endif
    return m;
}

int bist_hd_companion_init(void)
{
    bist_hd_msg_t msg;
    uint32_t result;
    int32_t ready_timeout_us = (int32_t)CONFIG_ESP_BIST_HD_AGENT_READY_TIMEOUT_US;

    ESP_LOGD(TAG, "init: mailbox");
    if (bist_hd_comp_port_init() != 0) {
        return -1;
    }

#ifdef CONFIG_ESP_BIST_STACK_TEST
    ESP_LOGD(TAG, "init: stack canary");
    bist_cpu_stack_overflow_init();
#endif

    ESP_LOGD(TAG, "init: wait AGENT_READY");
    if (bist_hd_comp_port_recv(&msg, ready_timeout_us) != 0 ||
            msg.type != BIST_HD_MSG_AGENT_READY) {
        ESP_LOGE(TAG, "no AGENT_READY");
        bist_hd_safe_state();
        return -1;
    }
    ESP_LOGD(TAG, "init: AGENT_READY");

    result = run_postboot_tests() | BIST_HD_BIT_POSTBOOT;
    ESP_LOGD(TAG, "init: send postboot status 0x%08lx", (unsigned long)result);
    if (send_lp_status(result) != 0) {
        bist_hd_safe_state();
        return -1;
    }
    if ((result & expected_postboot_mask()) != expected_postboot_mask()) {
        ESP_LOGE(TAG, "postboot fail");
        bist_hd_safe_state();
        return -1;
    }

    ESP_LOGD(TAG, "init: lp_wdt %u us", (unsigned)CONFIG_ESP_BIST_WDT_TIMEOUT_US);
    lp_wdt_init(CONFIG_ESP_BIST_WDT_TIMEOUT_US);
    ESP_LOGD(TAG, "init: done");
    return 0;
}

int bist_hd_companion_loop(void)
{
    uint32_t result;

    if (s_in_safe_state) {
        return 0;
    }

    ESP_LOGD(TAG, "runtime: feed WDT");
    lp_wdt_feed();

    ESP_LOGD(TAG, "runtime: run tests");
    result = run_runtime_tests() | BIST_HD_BIT_RUNTIME;
    if (send_lp_status(result) != 0) {
        bist_hd_safe_state();
        return -1;
    }
    ESP_LOGD(TAG, "runtime: status 0x%08lx", (unsigned long)result);
    if ((result & expected_runtime_mask()) != expected_runtime_mask()) {
        ESP_LOGE(TAG, "runtime fail");
        bist_hd_safe_state();
        return -1;
    }

#ifdef CONFIG_ESP_BIST_HD_AUDIT_QA
    ESP_LOGD(TAG, "runtime: host QA challenge");
    if (run_challenge_audit() != 0) {
        bist_hd_safe_state();
        return -1;
    }
#endif

    return 0;
}

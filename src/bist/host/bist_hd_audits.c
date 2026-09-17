/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Host-side diagnostic audit handlers.
 */

#include "bist_hd_audits.h"

#include "bist_hd_challenge.h"
#include "bist_hd_platform.h"
#include "bist_hd_transport.h"

#include "bist_conf.h"
#include "bist_esp_types.h"

#ifdef CONFIG_ESP_BIST_HD_AUDIT_CPU
#include "bist_cpu_regs.h"
#endif
#ifdef CONFIG_ESP_BIST_HD_AUDIT_CSR
#include "bist_cpu_csr_regs.h"
#endif

#if defined(CONFIG_ESP_BIST_HD_AUDIT_CPU) || defined(CONFIG_ESP_BIST_HD_AUDIT_CSR)
static uint32_t status_from_bist(bist_esp_err_t err)
{
    return (err == BIST_ESP_OK) ? BIST_HD_STATUS_OK : BIST_HD_STATUS_FAIL;
}
#endif

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

int bist_hd_audit_handle_diag(const bist_hd_msg_t *req)
{
    bist_hd_msg_t rsp = {0};
    uint32_t status = BIST_HD_STATUS_NOT_CONFIGURED;

    if (req == NULL) {
        return -1;
    }

    switch (req->audit_id) {
#ifdef CONFIG_ESP_BIST_HD_AUDIT_CPU
    case BIST_HD_AUDIT_CPU: {
        uint32_t irq_key = bist_hd_platform_irq_lock();

        status = status_from_bist(bist_cpu_regs_test());
        bist_hd_platform_irq_unlock(irq_key);
        break;
    }
#endif
#ifdef CONFIG_ESP_BIST_HD_AUDIT_CSR
    case BIST_HD_AUDIT_CSR: {
        uint32_t irq_key = bist_hd_platform_irq_lock();

        status = status_from_bist(bist_cpu_csr_regs_test());
        bist_hd_platform_irq_unlock(irq_key);
        break;
    }
#endif
    default:
        status = BIST_HD_STATUS_NOT_CONFIGURED;
        break;
    }

    rsp.type = BIST_HD_MSG_DIAG_RSP;
    rsp.audit_id = req->audit_id;
    rsp.seq = req->seq;
    rsp.payload = status;
    rsp.deadline_ticks = req->deadline_ticks;

    return bist_hd_transport_send(&rsp, 10000);
}

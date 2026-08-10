/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Host Diagnostics shared protocol (HP + LP).
 *
 * Typed messages for companion↔agent supervision, plus LP BIST result
 * bitmasks used on the status wire.
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Magic single-word AGENT_READY (HP → LP). */
#define BIST_HD_AGENT_READY_MAGIC 0xCAFECAFEu

/** High byte tag identifying a typed multi-word frame. */
#define BIST_HD_WIRE_TAG 0xB5u

/** Maximum words in an encoded message (header + seq + payload + deadline). */
#define BIST_HD_WIRE_WORDS_MAX 4u

/** Word count for a typed multi-word frame. */
#define BIST_HD_WIRE_TYPED_WORDS 4u

/* Typed-frame header (word0): [TAG:8][type:8][audit_id:8][nwords:8] */
#define BIST_HD_HDR_TAG_SHIFT    24u
#define BIST_HD_HDR_TYPE_SHIFT   16u
#define BIST_HD_HDR_AUDIT_SHIFT   8u
#define BIST_HD_HDR_NWORDS_SHIFT  0u
#define BIST_HD_HDR_FIELD_MASK    0xFFu

/** Seq field occupies the low 16 bits of word1. */
#define BIST_HD_SEQ_MASK 0xFFFFu

/**
 * Half of the uint16 sequence space. Used by bist_hd_seq_is_stale() for
 * wrap-aware "behind / duplicate" detection.
 */
#define BIST_HD_SEQ_HALF_RANGE 0x8000u

#define BIST_HD_HDR_GET_TAG(w) \
    ((uint8_t)(((uint32_t)(w) >> BIST_HD_HDR_TAG_SHIFT) & BIST_HD_HDR_FIELD_MASK))
#define BIST_HD_HDR_GET_TYPE(w) \
    ((uint8_t)(((uint32_t)(w) >> BIST_HD_HDR_TYPE_SHIFT) & BIST_HD_HDR_FIELD_MASK))
#define BIST_HD_HDR_GET_AUDIT(w) \
    ((uint8_t)(((uint32_t)(w) >> BIST_HD_HDR_AUDIT_SHIFT) & BIST_HD_HDR_FIELD_MASK))
#define BIST_HD_HDR_GET_NWORDS(w) \
    ((uint8_t)(((uint32_t)(w) >> BIST_HD_HDR_NWORDS_SHIFT) & BIST_HD_HDR_FIELD_MASK))

/* -------------------------------------------------------------------------- */
/* LP BIST result bits (bit set = test passed) — stable wire values           */
/* -------------------------------------------------------------------------- */

#define BIST_HD_BIT_CPU_REG  (1u << 0)
#define BIST_HD_BIT_CPU_CSR  (1u << 1)
#define BIST_HD_BIT_RAM_A    (1u << 2)
#define BIST_HD_BIT_RAM_X    (1u << 3)
#define BIST_HD_BIT_FLASH    (1u << 4)
#define BIST_HD_BIT_STACK    (1u << 5)
#define BIST_HD_BIT_ABRAHAM  (1u << 6)
#define BIST_HD_BIT_RUNTIME  (1u << 30)
#define BIST_HD_BIT_POSTBOOT (1u << 31)

#define BIST_HD_POSTBOOT_ALL_PASS \
    (BIST_HD_BIT_CPU_REG | BIST_HD_BIT_CPU_CSR | BIST_HD_BIT_RAM_X | \
     BIST_HD_BIT_FLASH | BIST_HD_BIT_ABRAHAM)

#define BIST_HD_RUNTIME_ALL_PASS \
    (BIST_HD_BIT_CPU_REG | BIST_HD_BIT_CPU_CSR | BIST_HD_BIT_RAM_A | \
     BIST_HD_BIT_STACK | BIST_HD_BIT_ABRAHAM)

/** DIAG_RSP / status payload codes. */
#define BIST_HD_STATUS_OK             0u
#define BIST_HD_STATUS_FAIL           1u
#define BIST_HD_STATUS_NOT_CONFIGURED 2u
#define BIST_HD_STATUS_TIMEOUT        3u
#define BIST_HD_STATUS_BAD_SEQ        4u

/**
 * @brief Host-audit catalog identifiers (equal rank; product enables a subset).
 */
typedef enum {
    BIST_HD_AUDIT_QA = 0,
    BIST_HD_AUDIT_CHECKPOINT,
    BIST_HD_AUDIT_RAM,
    BIST_HD_AUDIT_FLASH,
    BIST_HD_AUDIT_CPU,
    BIST_HD_AUDIT_CSR,
    BIST_HD_AUDIT_STACK,
    BIST_HD_AUDIT_CLOCK,
    BIST_HD_AUDIT_WDT,
    BIST_HD_AUDIT_PC,
    BIST_HD_AUDIT_IRQ_LATENCY,
    BIST_HD_AUDIT_GPIO,
    BIST_HD_AUDIT_ADC,
    BIST_HD_AUDIT_CONFIG_NVM,
    BIST_HD_AUDIT_IPC,
    BIST_HD_AUDIT_SECURE_BOOT,
    BIST_HD_AUDIT_DUAL_CHANNEL,
    BIST_HD_AUDIT_COUNT
} bist_hd_audit_id_t;

/**
 * @brief Typed Host Diagnostics message kinds.
 */
typedef enum {
    BIST_HD_MSG_AGENT_READY = 1,
    BIST_HD_MSG_CHALLENGE = 2,
    BIST_HD_MSG_ANSWER = 3,
    BIST_HD_MSG_DIAG_REQ = 4,
    BIST_HD_MSG_DIAG_RSP = 5,
    BIST_HD_MSG_CHECKPOINT = 6,
    BIST_HD_MSG_SAFE_STATE_NOTIFY = 7,
    /** LP BIST result bitmask in payload (POSTBOOT/RUNTIME flag set). */
    BIST_HD_MSG_LP_STATUS = 8,
} bist_hd_msg_type_t;

/**
 * @brief Decoded Host Diagnostics message.
 *
 * Wire encoding is a 1- or 4-word mailbox frame (see bist_hd_msg_encode).
 */
typedef struct {
    uint8_t type;            /**< bist_hd_msg_type_t */
    uint8_t audit_id;        /**< bist_hd_audit_id_t when applicable, else 0 */
    uint16_t seq;            /**< sequence number */
    uint32_t deadline_ticks; /**< absolute deadline / window end (0 if unused) */
    uint32_t payload;        /**< challenge, answer, CRC, status, checkpoint id, … */
} bist_hd_msg_t;

/**
 * @brief Encode a message into mailbox words.
 *
 * AGENT_READY encodes as a single magic word. Every other type uses a
 * 4-word tagged *frame*: an untagged status word would let a
 * stray payload decode as a valid result after a transport desync.
 *
 * @return 0 on success, -1 on invalid args / buffer too small
 */
int bist_hd_msg_encode(const bist_hd_msg_t *msg, uint32_t *words, size_t max_words,
                       size_t *out_nwords);

/**
 * @brief Decode mailbox words into a message.
 *
 * @return 0 on success, -1 on invalid frame
 */
int bist_hd_msg_decode(const uint32_t *words, size_t nwords, bist_hd_msg_t *msg);

/**
 * @brief Frame length implied by its first word.
 *
 * Word-at-a-time transports use it to know how many more words to pull;
 * shared-memory transports use it to find the end of the frame.
 *
 * @return Word count of the frame, or 0 if @p word0 starts no valid frame
 */
size_t bist_hd_frame_nwords(uint32_t word0);

/**
 * @brief Return true if @p actual equals @p expected.
 */
bool bist_hd_seq_check(uint16_t expected, uint16_t actual);

/**
 * @brief Return true if @p incoming is stale or a duplicate of @p last_accepted.
 *
 * Uses unsigned 16-bit wrap-aware distance: incoming is stale when it is not
 * strictly ahead of last_accepted in the forward half of the sequence space.
 */
bool bist_hd_seq_is_stale(uint16_t last_accepted, uint16_t incoming);

/**
 * @brief Return true if @p now_ticks is at or past @p deadline_ticks.
 *
 * Unsigned wrap-aware comparison suitable for free-running tick counters.
 */
bool bist_hd_timeout_expired(uint32_t now_ticks, uint32_t deadline_ticks);

#ifdef __cplusplus
}
#endif

/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * NuttX Host Diagnostics transport (/dev/lp_mailbox).
 *
 * The NuttX LP mailbox character driver transfers one byte per underlying
 * lp_core_mailbox transaction, so each 32-bit protocol word is sent/received
 * as 4 little-endian bytes through the fd.
 */

#include "bist_hd_transport.h"

#include <nuttx/config.h>

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>

#define LP_MAILBOX_PATH "/dev/lp_mailbox"

static int s_fd = -1;

static int write_word(int fd, uint32_t word)
{
    uint8_t b[4] = {
        (uint8_t)(word),
        (uint8_t)(word >> 8),
        (uint8_t)(word >> 16),
        (uint8_t)(word >> 24),
    };
    ssize_t n = write(fd, b, 4);

    return (n == 4) ? 0 : -1;
}

static int read_word(int fd, uint32_t *word)
{
    uint8_t b[4];
    ssize_t n = read(fd, b, 4);

    if (n != 4) {
        return -1;
    }
    *word = ((uint32_t)b[0]) |
            ((uint32_t)b[1] << 8) |
            ((uint32_t)b[2] << 16) |
            ((uint32_t)b[3] << 24);
    return 0;
}

int bist_hd_transport_init(void)
{
    if (s_fd >= 0) {
        return 0;
    }

    s_fd = open(LP_MAILBOX_PATH, O_RDWR);
    return (s_fd >= 0) ? 0 : -1;
}

int bist_hd_transport_send(const bist_hd_msg_t *msg, int32_t timeout_ms)
{
    uint32_t words[BIST_HD_WIRE_WORDS_MAX];
    size_t nwords = 0;
    size_t i;

    (void)timeout_ms;

    if (s_fd < 0 || msg == NULL) {
        return -1;
    }
    if (bist_hd_msg_encode(msg, words, BIST_HD_WIRE_WORDS_MAX, &nwords) != 0) {
        return -1;
    }

    for (i = 0; i < nwords; i++) {
        if (write_word(s_fd, words[i]) != 0) {
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

    (void)timeout_ms;

    if (s_fd < 0 || msg == NULL) {
        return -1;
    }

    if (read_word(s_fd, &words[0]) != 0) {
        return -1;
    }

    nwords = bist_hd_frame_nwords(words[0]);
    if (nwords == 0u) {
        return -1;
    }

    for (i = 1; i < nwords; i++) {
        if (read_word(s_fd, &words[i]) != 0) {
            return -1;
        }
    }

    return bist_hd_msg_decode(words, nwords, msg);
}

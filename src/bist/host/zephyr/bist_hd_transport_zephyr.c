/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Zephyr Host Diagnostics transport (mbox), HP image.
 *
 * The Espressif mbox driver is a shared-memory window plus a doorbell
 * interrupt, so a whole frame travels in one transaction and the receiver
 * locates its end from the self-describing header. The LP companion port
 * mirrors the mbox conventions below; both derive the frame layout from
 * bist_hd_protocol.h, which is what keeps the two ends of the wire agreeing.
 */

#include "bist_hd_transport.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/kernel.h>

#include <string.h>

#if !DT_NODE_EXISTS(DT_PATH(mbox_consumer))
#error "Host Diagnostics needs an /mbox-consumer node with tx and rx channels"
#endif

#define HD_RX_QUEUE_LEN 4

/** Bytes of mbox shared memory each direction must provide. */
#define HD_MBOX_FRAME_BYTES (BIST_HD_WIRE_WORDS_MAX * sizeof(uint32_t))

/** Poll step while the peer has not yet drained the single-slot mbox. */
#define HD_MBOX_RETRY_US 20u

static const struct mbox_dt_spec s_tx = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), tx);
static const struct mbox_dt_spec s_rx = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), rx);

K_MSGQ_DEFINE(s_rx_q, sizeof(bist_hd_msg_t), HD_RX_QUEUE_LEN, 4);

static bool s_inited;

static int hd_mbox_decode(const struct mbox_msg *data, bist_hd_msg_t *msg)
{
	uint32_t words[BIST_HD_WIRE_WORDS_MAX] = {0};
	size_t avail;
	size_t nwords;

	if (data == NULL || data->data == NULL) {
		return -1;
	}

	avail = MIN(data->size, sizeof(words)) / sizeof(uint32_t);
	if (avail == 0u) {
		return -1;
	}
	memcpy(words, data->data, avail * sizeof(uint32_t));

	nwords = bist_hd_frame_nwords(words[0]);
	if (nwords == 0u || nwords > avail) {
		return -1;
	}

	return bist_hd_msg_decode(words, nwords, msg);
}

static void hd_rx_cb(const struct device *dev, mbox_channel_id_t channel_id, void *user_data,
		     struct mbox_msg *data)
{
	bist_hd_msg_t msg;

	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);
	ARG_UNUSED(user_data);

	if (hd_mbox_decode(data, &msg) != 0) {
		return;
	}

	(void)k_msgq_put(&s_rx_q, &msg, K_NO_WAIT);
}

int bist_hd_transport_init(void)
{
	int ret;

	if (s_inited) {
		return 0;
	}

	if (!mbox_is_ready_dt(&s_tx) || !mbox_is_ready_dt(&s_rx)) {
		return -ENODEV;
	}
	if (mbox_mtu_get_dt(&s_tx) < (int)HD_MBOX_FRAME_BYTES) {
		return -EMSGSIZE;
	}

	ret = mbox_register_callback_dt(&s_rx, hd_rx_cb, NULL);
	if (ret != 0) {
		return ret;
	}
	ret = mbox_set_enabled_dt(&s_rx, 1);
	if (ret != 0) {
		return ret;
	}

	s_inited = true;
	return 0;
}

int bist_hd_transport_send(const bist_hd_msg_t *msg, int32_t timeout_ms)
{
	uint32_t words[BIST_HD_WIRE_WORDS_MAX];
	size_t nwords = 0;
	struct mbox_msg out;
	uint32_t waited_us = 0;
	uint32_t timeout_us;

	if (!s_inited || msg == NULL) {
		return -EINVAL;
	}
	if (bist_hd_msg_encode(msg, words, BIST_HD_WIRE_WORDS_MAX, &nwords) != 0) {
		return -EINVAL;
	}

	out.data = words;
	out.size = nwords * sizeof(uint32_t);
	timeout_us = (timeout_ms < 0) ? 0u : (uint32_t)timeout_ms * 1000u;

	for (;;) {
		int ret = mbox_send_dt(&s_tx, &out);

		if (ret != -EBUSY) {
			return ret;
		}
		if (timeout_ms >= 0 && waited_us >= timeout_us) {
			return -EBUSY;
		}
		k_busy_wait(HD_MBOX_RETRY_US);
		waited_us += HD_MBOX_RETRY_US;
	}
}

int bist_hd_transport_recv(bist_hd_msg_t *msg, int32_t timeout_ms)
{
	k_timeout_t timeout = (timeout_ms < 0) ? K_FOREVER : K_MSEC(timeout_ms);

	if (!s_inited || msg == NULL) {
		return -EINVAL;
	}

	return k_msgq_get(&s_rx_q, msg, timeout);
}

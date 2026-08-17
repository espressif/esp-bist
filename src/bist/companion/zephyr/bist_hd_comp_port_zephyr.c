/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Zephyr lpcore companion port.
 *
 * The LP image is single-threaded with no system clock, so received frames
 * land in an ISR-filled ring that the companion polls, and all timing comes
 * from the LP cycle counter instead of the kernel clock.
 *
 * The mbox conventions below mirror the HP transport; both derive the frame
 * layout from bist_hd_protocol.h, which is what keeps the two ends of the
 * wire agreeing.
 */

#include "bist_hd_comp_port.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>

#include <string.h>

#include <ulp_lp_core_utils.h>

#if !DT_NODE_EXISTS(DT_PATH(mbox_consumer))
#error "Host Diagnostics needs an /mbox-consumer node with tx and rx channels"
#endif

/* LP core runs at 40 MHz on the supported targets. */
#define HD_LP_MHZ 40u

#define HD_RX_RING_LEN 4

/** Bytes of mbox shared memory each direction must provide. */
#define HD_MBOX_FRAME_BYTES (BIST_HD_WIRE_WORDS_MAX * sizeof(uint32_t))

/** Poll step while the peer has not yet drained the single-slot mbox. */
#define HD_MBOX_RETRY_US 20u

static const struct mbox_dt_spec s_tx = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), tx);
static const struct mbox_dt_spec s_rx = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), rx);

static bist_hd_msg_t s_rx_ring[HD_RX_RING_LEN];
static volatile uint8_t s_rx_head;
static volatile uint8_t s_rx_tail;
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
	uint8_t next;

	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);
	ARG_UNUSED(user_data);

	next = (uint8_t)((s_rx_head + 1u) % HD_RX_RING_LEN);
	if (next == s_rx_tail) {
		/* Ring full: drop, the companion fails the audit on timeout. */
		return;
	}
	if (hd_mbox_decode(data, &s_rx_ring[s_rx_head]) != 0) {
		return;
	}
	s_rx_head = next;
}

static bool rx_pop(bist_hd_msg_t *msg)
{
	unsigned int key = irq_lock();
	bool got = false;

	if (s_rx_tail != s_rx_head) {
		*msg = s_rx_ring[s_rx_tail];
		s_rx_tail = (uint8_t)((s_rx_tail + 1u) % HD_RX_RING_LEN);
		got = true;
	}

	irq_unlock(key);
	return got;
}

int bist_hd_comp_port_init(void)
{
	int ret;

	if (s_inited) {
		return 0;
	}

	if (!mbox_is_ready_dt(&s_tx) || !mbox_is_ready_dt(&s_rx)) {
		return -1;
	}
	if (mbox_mtu_get_dt(&s_tx) < (int)HD_MBOX_FRAME_BYTES) {
		return -1;
	}

	ret = mbox_register_callback_dt(&s_rx, hd_rx_cb, NULL);
	if (ret != 0) {
		return -1;
	}
	if (mbox_set_enabled_dt(&s_rx, 1) != 0) {
		return -1;
	}

	s_inited = true;
	return 0;
}

int bist_hd_comp_port_send(const bist_hd_msg_t *msg)
{
	uint32_t words[BIST_HD_WIRE_WORDS_MAX];
	size_t nwords = 0;
	struct mbox_msg out;

	if (!s_inited || msg == NULL) {
		return -1;
	}
	if (bist_hd_msg_encode(msg, words, BIST_HD_WIRE_WORDS_MAX, &nwords) != 0) {
		return -1;
	}

	out.data = words;
	out.size = nwords * sizeof(uint32_t);

	/* Single-slot mbox: spin until the host agent has drained the previous frame. */
	while (mbox_send_dt(&s_tx, &out) == -EBUSY) {
		bist_hd_comp_port_delay_us(HD_MBOX_RETRY_US);
	}
	return 0;
}

int bist_hd_comp_port_recv(bist_hd_msg_t *msg, int32_t timeout_us)
{
	uint32_t start = bist_hd_comp_port_tick();

	if (!s_inited || msg == NULL) {
		return -1;
	}

	for (;;) {
		if (rx_pop(msg)) {
			return 0;
		}
		if (timeout_us >= 0 &&
		    bist_hd_comp_port_elapsed_us(start) >= (uint32_t)timeout_us) {
			return -1;
		}
	}
}

uint32_t bist_hd_comp_port_tick(void)
{
	return ulp_lp_core_get_cpu_cycles();
}

uint32_t bist_hd_comp_port_elapsed_us(uint32_t start_tick)
{
	return (ulp_lp_core_get_cpu_cycles() - start_tick) / HD_LP_MHZ;
}

void bist_hd_comp_port_delay_us(uint32_t us)
{
	uint32_t start = ulp_lp_core_get_cpu_cycles();

	while (bist_hd_comp_port_elapsed_us(start) < us) {
	}
}

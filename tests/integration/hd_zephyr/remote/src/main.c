/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * LP-core entry for the Host Diagnostics fail-closed validation app.
 *
 * Identical to samples/zephyr/remote/src/main.c on purpose: the companion is
 * the element under test, so it is built and run exactly as a product would.
 * Every difference between the happy path and the fault cases lives on the HP
 * side.
 *
 * The LP image has no system clock, so pacing uses the companion port delay
 * rather than k_msleep().
 */

#include <zephyr/kernel.h>

#include <bist_hd_comp_port.h>
#include <bist_hd_companion.h>

#define RUNTIME_INTERVAL_US (CONFIG_ESP_BIST_RUNTIME_TEST_INTERVAL_MS * 1000)

int main(void)
{
	if (bist_hd_companion_init() != 0) {
		return 0;
	}

	while (1) {
		bist_hd_companion_loop();
		bist_hd_comp_port_delay_us(RUNTIME_INTERVAL_US);
	}

	return 0;
}

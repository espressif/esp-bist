/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * LP-core entry for the ESP-BIST IDF sample.
 *
 * Host Diagnostics companion handles LP BIST and status reporting; this
 * main owns the loop so product code can run alongside it.
 */

#include "bist_hd_comp_port.h"
#include "bist_hd_companion.h"

#define RUNTIME_INTERVAL_US 10000

int main(void)
{
    if (bist_hd_companion_init() != 0) {
        return 0;
    }

    while (1) {
        bist_hd_companion_loop();
        /* Application-specific LP logic can go here. */
        bist_hd_comp_port_delay_us(RUNTIME_INTERVAL_US);
    }

    return 0;
}

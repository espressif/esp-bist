/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * LP-core entry for the Host Diagnostics fail-closed validation app (NuttX).
 *
 * Identical to samples/nuttx/nuttx_bist/ulp/main.c on purpose: the companion is
 * the element under test, so it is built and run exactly as a product would.
 * Every difference between the happy path and the fault cases lives on the HP
 * side.
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
        bist_hd_comp_port_delay_us(RUNTIME_INTERVAL_US);
    }

    return 0;
}

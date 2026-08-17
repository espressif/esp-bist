# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

"""
Host Diagnostics fail-closed validation on ESP-IDF hardware.

The host either answers with a challenge key the LP companion does not share
(hd_key_mismatch) or withholds its own agent past the challenge window
(hd_starved_agent). Either way the companion enters safe state, stops feeding
the LP watchdog, and the chip resets.

The ESP-BIST library is built exactly as a product would build it; the fault
comes from the app under tests/integration/hd_idf. The happy path lives in
samples/idf. Companion judgment itself is covered off-target by
tests/unit/hd_companion.
"""

import pytest


@pytest.mark.parametrize('target', ['esp32c5', 'esp32c6', 'esp32p4'], indirect=True)
@pytest.mark.parametrize('config', ['hd_key_mismatch', 'hd_starved_agent'], indirect=True)
def test_hd_idf_fail_closed(dut, target, config):
    """Wrong or missing Q&A answer → companion safe state → LP WDT reset."""
    dut.expect('test_HD_agent_ready:PASS', timeout=30)
    dut.expect('test_BIST_postboot:PASS', timeout=30)
    dut.expect('test_HD_fail_closed_armed:PASS', timeout=30)
    dut.expect('CPU has been reset by WDT', timeout=30)

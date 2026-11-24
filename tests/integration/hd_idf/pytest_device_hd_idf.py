# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

"""
Host Diagnostics fail-closed validation on ESP-IDF hardware.

The host either answers with a challenge key the LP companion does not share
(hd_key_mismatch), withholds its own agent past the challenge window
(hd_starved_agent), never sends a checkpoint (hd_skip_checkpoint), or
illegally attempts an HP warm reboot via esp_restart() (hd_host_sw_reset).
Every fault variant ends with the companion entering safe state, recovering
from any LP_WDT tamper, and the chip resetting via LP_WDT.

The ESP-BIST library is built exactly as a product would build it; the fault
comes from the app under tests/integration/hd_idf. The happy path lives in
samples/idf. Companion judgment itself is covered off-target by
tests/unit/hd_companion.
"""

import pytest


@pytest.mark.parametrize("target", ["esp32c5", "esp32c6", "esp32p4"], indirect=True)
@pytest.mark.parametrize(
    "config",
    ["hd_key_mismatch", "hd_starved_agent", "hd_skip_checkpoint", "hd_host_sw_reset"],
    indirect=True,
)
def test_hd_idf_fail_closed(dut, target, config):
    """Host Q&A/checkpoint fault or illegal SW_CPU reset → safe state → LP WDT reset."""
    dut.expect("test_HD_agent_ready:PASS", timeout=30)
    dut.expect("test_BIST_postboot:PASS", timeout=30)
    dut.expect("test_HD_safe_dma_pool:PASS", timeout=30)
    dut.expect("test_HD_fail_closed_armed:PASS", timeout=30)
    dut.expect(
        r"rst:0x10 \((LP_WDT_SYS|RTC_WDT_SYS|CHIP_LP_WDT_RESET)\)",
        timeout=30,
    )

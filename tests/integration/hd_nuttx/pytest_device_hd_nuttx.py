# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

"""
Host Diagnostics fail-closed validation on NuttX hardware.

The host either answers with a challenge key the LP companion does not share
(hd_key_mismatch) or withholds its own agent past the challenge window
(hd_starved_agent). Either way the companion enters safe state, stops feeding
the LP watchdog, and the chip resets.

The ESP-BIST library is built exactly as a product would build it; the fault
comes from the app under tests/integration/hd_nuttx. The happy path lives in
samples/nuttx. Companion judgment itself is covered off-target by
tests/unit/hd_companion.

Flash the matching image before each parametrization. CI flashes
build-<config>/nuttx.merged.bin then runs pytest -k <config>.
"""

import re

import pytest

# NuttX does not ship the ESP-IDF bootloader string "CPU has been reset by WDT".
# After an LP WDT system reset the ROM prints a reset-reason line such as
# rst:0x9 (RTCWDT_SYS) / rst:0x10 (RTCWDT_RTC_RESET) on C6, or
# rst:0x9 (HP_SYS_LP_WDT_RESET) on P4. Keep the IDF substring as an alternate
# so a board that does print it still matches.
_WDT_RESET = re.compile(r"(CPU has been reset by WDT|rst:0x[0-9a-fA-F]+ \([^)]*WDT)")


@pytest.mark.parametrize("config", ["hd_key_mismatch", "hd_starved_agent"])
def test_hd_nuttx_fail_closed(dut, config):
    """Wrong or missing Q&A answer → companion safe state → LP WDT reset."""
    assert config in ("hd_key_mismatch", "hd_starved_agent")

    dut.write("")
    dut.expect(r"nsh>", timeout=10)

    dut.write("hd_nuttx_test")

    dut.expect("test_HD_agent_ready:PASS", timeout=30)
    dut.expect("test_BIST_postboot:PASS", timeout=30)
    dut.expect("test_HD_fail_closed_armed:PASS", timeout=30)
    dut.expect(_WDT_RESET, timeout=30)

# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

"""
Device tests for the ESP-BIST NuttX sample.

Uses pytest-embedded serial DUT, runs ``nuttx_bist`` from NSH,
and verifies Host Diagnostics plus post-boot / runtime BIST markers
printed by the HP core. Fail-closed behaviour is validated separately in
tests/integration/hd_nuttx.
"""

import pytest


@pytest.mark.parametrize("target", ["esp32c6"], indirect=True)
def test_bist_nuttx_sample(dut):
    dut.write("")
    dut.expect(r"nsh>", timeout=2)

    dut.write("nuttx_bist")

    dut.expect("test_HD_agent_ready:PASS", timeout=30)

    # Post-boot tests
    dut.expect("test_BIST_cpu_reg:PASS", timeout=30)
    dut.expect("test_BIST_cpu_csr:PASS", timeout=10)
    dut.expect("test_BIST_ram_march_x:PASS", timeout=10)
    dut.expect("test_BIST_flash_crc:PASS", timeout=10)

    # Runtime tests (first iteration)
    dut.expect("test_BIST_runtime_cpu_reg:PASS", timeout=30)
    dut.expect("test_BIST_runtime_cpu_csr:PASS", timeout=10)
    dut.expect("test_BIST_runtime_ram_march_a:PASS", timeout=10)
    dut.expect("test_BIST_runtime_stack_check:PASS", timeout=10)

    # Verify all 10 runtime loops completed with no failures
    dut.expect(r"Runtime loop 10/10", timeout=60)
    dut.expect("test_HD_challenge:PASS", timeout=10)
    dut.expect("BIST_RESULT:PASS", timeout=10)

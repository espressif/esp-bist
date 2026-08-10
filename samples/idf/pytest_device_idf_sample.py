# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

"""
Device smoke test for the ESP-BIST IDF sample.

Checks the happy path the sample demonstrates: AGENT_READY, post-boot and
runtime LP_STATUS, and repeated Q&A challenge/answer rounds.

Fail-closed behaviour is validated separately in tests/integration/hd_idf, so
this sample stays a readable starting point.
"""

import pytest


@pytest.mark.parametrize('target', ['esp32c5', 'esp32c6', 'esp32p4'], indirect=True)
def test_bist_idf_sample(dut, target):
    dut.expect('test_HD_agent_ready:PASS', timeout=30)

    # Post-boot tests
    dut.expect('test_BIST_cpu_reg:PASS', timeout=30)
    dut.expect('test_BIST_cpu_csr:PASS', timeout=10)
    dut.expect('test_BIST_ram_march_x:PASS', timeout=10)
    dut.expect('test_BIST_ram_abraham:PASS', timeout=30)
    dut.expect('test_BIST_flash_crc:PASS', timeout=10)

    # Runtime tests (first iteration)
    dut.expect('test_BIST_runtime_cpu_reg:PASS', timeout=30)
    dut.expect('test_BIST_runtime_cpu_csr:PASS', timeout=10)
    dut.expect('test_BIST_runtime_ram_march_a:PASS', timeout=10)
    dut.expect('test_BIST_runtime_ram_abraham:PASS', timeout=10)
    dut.expect('test_BIST_runtime_stack_check:PASS', timeout=10)

    # Verify all 10 runtime loops completed with no failures in any iteration
    dut.expect('Runtime loop 10/10', timeout=30)
    dut.expect('test_HD_challenge:PASS', timeout=10)
    dut.expect('BIST_RESULT:PASS', timeout=10)

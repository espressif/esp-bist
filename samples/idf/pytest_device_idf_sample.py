# SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

"""
Device tests for the ESP-BIST IDF sample.

Verifies that the LP core BIST tests run successfully by checking the
structured output printed by the HP core on the main UART.
All assertions run against a single boot to avoid unnecessary reflashes.
"""

import pytest


@pytest.mark.parametrize('target', ['esp32c6'])
def test_bist_idf_sample(dut):
    # Post-boot tests
    dut.expect('test_BIST_cpu_reg:PASS', timeout=30)
    dut.expect('test_BIST_cpu_csr:PASS', timeout=10)
    dut.expect('test_BIST_ram_march_x:PASS', timeout=10)
    dut.expect('test_BIST_flash_crc:PASS', timeout=10)

    # Runtime tests (first iteration)
    dut.expect('test_BIST_runtime_cpu_reg:PASS', timeout=30)
    dut.expect('test_BIST_runtime_cpu_csr:PASS', timeout=10)
    dut.expect('test_BIST_runtime_ram_march_a:PASS', timeout=10)
    dut.expect('test_BIST_runtime_stack_check:PASS', timeout=10)

    # Verify all 10 runtime loops completed with no failures in any iteration
    dut.expect('Runtime loop 10/10', timeout=30)
    dut.expect('BIST_RESULT:PASS', timeout=10)

import time

import pytest

from tests.idf_targets import pytestmark  # noqa: F401


def test_ram_success(dut, target):
    tests_names = [
        "test_BIST_ram_march_a",
        "test_BIST_ram_march_x",
        "test_BIST_ram_abraham",
        "test_BIST_ram_abraham_full",
    ]
    expected_outputs = [f"{test}:PASS" for test in tests_names]

    # expect from what esptool.py printed to sys.stdout
    for expected_output in expected_outputs:
        dut.expect(expected_output)


def ram_error_test(openocd_dut, gdb_instance, dut, test_name, test_bp, injected_value):
    expected_output = "{}:FAIL".format(test_name)
    script = """
    #connect to remote server
    target remote 127.0.0.1:3333
    mon reset halt
    maintenance flush register-cache
    monitor gdb breakpoint_override hard

    # Set a breakpoint at the specific address
    tb {}
    commands
        set _bist_ram_test_start={}
        monitor step
        monitor resume
    end
    continue
    """.format(test_bp, injected_value)
    time.sleep(1)
    with openocd_dut.run():
        gdb_process = gdb_instance.attach(script)
        time.sleep(5)  # Wait for GDB to attach and run the script
        try:
            dut.expect(expected_output, timeout=30)
        finally:
            gdb_instance.stop(gdb_process)


@pytest.mark.injection
def test_ram_march_a_error_device(openocd_dut, gdb_instance, dut, target):
    ram_error_test(
        openocd_dut,
        gdb_instance,
        dut,
        "test_BIST_ram_march_a",
        "bist_ram_test_march_a_step2",
        "0xFF",
    )


@pytest.mark.injection
def test_ram_march_x_error_device(openocd_dut, gdb_instance, dut, target):
    ram_error_test(
        openocd_dut,
        gdb_instance,
        dut,
        "test_BIST_ram_march_x",
        "bist_ram_test_march_x_step2",
        "0xFF",
    )


@pytest.mark.injection
def test_ram_abraham_error_device(openocd_dut, gdb_instance, dut, target):
    ram_error_test(
        openocd_dut,
        gdb_instance,
        dut,
        "test_BIST_ram_abraham",
        "bist_ram_test_abraham_seq1",
        "0xFF",
    )

import time

import pytest

from tests.idf_targets import pytestmark  # noqa: F401


def test_wdt_success(dut, target):
    tests_names = ["test_BIST_WDT", "test_BIST_WDT_INIT_SUB_TICK_TIMEOUT"]
    expected_outputs = [f"{test}:PASS" for test in tests_names]

    # expect from what esptool.py printed to sys.stdout
    for expected_output in expected_outputs:
        dut.expect(expected_output)


def wdt_test_routine(
    openocd_dut, gdb_instance, dut, test_name, test_bp, injected_value, expected_output
):
    expected_output = "{}:{}".format(test_name, expected_output)
    script = """
    #connect to remote server
    target remote 127.0.0.1:3333
    mon reset halt
    maintenance flush register-cache
    monitor gdb breakpoint_override hard

    # Set a breakpoint at the specific address
    tb {}
    commands
        set wdt_timeout_us={}
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
def test_wdt_error_device(openocd_dut, gdb_instance, dut, target):
    wdt_test_routine(
        openocd_dut,
        gdb_instance,
        dut,
        "test_BIST_WDT",
        "bist_test_wdt_timeout",
        "1000000",
        "FAIL",
    )


@pytest.mark.injection
def test_wdt_success_device(openocd_dut, gdb_instance, dut, target):
    wdt_test_routine(
        openocd_dut,
        gdb_instance,
        dut,
        "test_BIST_WDT",
        "bist_test_wdt_timeout",
        "10000",
        "PASS",
    )

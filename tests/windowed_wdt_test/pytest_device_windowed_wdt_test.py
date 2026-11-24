import time

import pytest

from tests.idf_targets import pytestmark  # noqa: F401


def test_windowed_wdt_success(dut, target):
    tests_names = [
        "test_BIST_WINDOWED_WDT_NORMAL",
        "test_BIST_WINDOWED_WDT_UNDERFLOW",
        "test_BIST_WINDOWED_WDT_CONSECUTIVE",
    ]
    expected_outputs = [f"{test}:PASS" for test in tests_names]

    for expected_output in expected_outputs:
        dut.expect(expected_output)


# wdt_is_underflow_detected() call order across Unity tests in main():
#   NORMAL: 2, UNDERFLOW: 1, CONSECUTIVE: 10
_CALLS_BEFORE = {
    "test_BIST_WINDOWED_WDT_NORMAL": 0,
    "test_BIST_WINDOWED_WDT_UNDERFLOW": 2,
    "test_BIST_WINDOWED_WDT_CONSECUTIVE": 3,
}


def windowed_wdt_error_test(openocd_dut, gdb_instance, dut, test_name, injected_value):
    expected_output = "{}:FAIL".format(test_name)
    ignore_count = _CALLS_BEFORE[test_name]
    script = """
    #connect to remote server
    target remote 127.0.0.1:3333
    mon reset halt
    maintenance flush register-cache
    monitor gdb breakpoint_override hard

    # Stop after underflow is assigned, before return (shared label)
    break bist_windowed_wdt_underflow_check
    ignore $bpnum {}
    commands
        set underflow={}
        monitor step
        set $a0={}
        monitor resume
    end
    continue
    """.format(ignore_count, injected_value, injected_value)
    time.sleep(1)
    with openocd_dut.run():
        gdb_process = gdb_instance.attach(script)
        time.sleep(5)  # Wait for GDB to attach and run the script
        try:
            dut.expect(expected_output, timeout=30)
        finally:
            gdb_instance.stop(gdb_process)


@pytest.mark.injection
def test_windowed_wdt_normal_error_device(openocd_dut, gdb_instance, dut, target):
    windowed_wdt_error_test(
        openocd_dut, gdb_instance, dut, "test_BIST_WINDOWED_WDT_NORMAL", "1"
    )


@pytest.mark.injection
def test_windowed_wdt_underflow_error_device(openocd_dut, gdb_instance, dut, target):
    windowed_wdt_error_test(
        openocd_dut, gdb_instance, dut, "test_BIST_WINDOWED_WDT_UNDERFLOW", "0"
    )


@pytest.mark.injection
def test_windowed_wdt_consecutive_error_device(openocd_dut, gdb_instance, dut, target):
    windowed_wdt_error_test(
        openocd_dut, gdb_instance, dut, "test_BIST_WINDOWED_WDT_CONSECUTIVE", "1"
    )

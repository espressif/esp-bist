import time

import pytest

from tests.idf_targets import pytestmark  # noqa: F401


def clock_error_test(openocd_dut, gdb_instance, dut, test_name, test_bp, inject_cmd):
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
        {}
        monitor step
        monitor resume
    end
    continue
    """.format(test_bp, inject_cmd)
    time.sleep(1)
    with openocd_dut.run():
        gdb_process = gdb_instance.attach(script)
        time.sleep(5)  # Wait for GDB to attach and run the script
        try:
            dut.expect(expected_output, timeout=30)
        finally:
            gdb_instance.stop(gdb_process)


@pytest.mark.injection
def test_ext_crystal_fail_error_device(openocd_dut, gdb_instance, dut, target):
    if target not in ("esp32c3",):
        pytest.skip("XT WDT / bist_ext_crystal_check not available on this target")
    clock_error_test(
        openocd_dut,
        gdb_instance,
        dut,
        "test_BIST_ext_crystal_fail",
        "bist_ext_crystal_check",
        "set test_failed=1",
    )


@pytest.mark.injection
def test_main_crystal_error_device(openocd_dut, gdb_instance, dut, target):
    clock_error_test(
        openocd_dut,
        gdb_instance,
        dut,
        "test_BIST_main_crystal",
        "bist_main_crystal_check",
        "set deviation=100",
    )

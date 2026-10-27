import time

import pytest

from tests.idf_targets import pytestmark  # noqa: F401


def test_pc_success(dut, target):
    tests_names = ["test_BIST_PC"]
    expected_outputs = [f"{test}:PASS" for test in tests_names]

    # expect from what esptool.py printed to sys.stdout
    for expected_output in expected_outputs:
        dut.expect(expected_output)


@pytest.mark.injection
def test_pc_error_wdt_device(openocd_dut, gdb_instance, dut, target):
    expected_outputs = ["Reset reason: 1", "Reset reason: 7"]
    script = """
    #connect to remote server
    target remote 127.0.0.1:3333
    mon reset halt
    maintenance flush register-cache
    monitor gdb breakpoint_override hard

    # Break at the PC verification loop entry.
    # pcTestFunctions is a local array optimized onto the stack via memcpy;
    # it is not accessible by name. At this breakpoint $sp points to the
    # first element of the array, so we corrupt it through the stack pointer.
    # Setting the pointer to 0 causes an instruction access fault that the
    # firmware cannot recover from, so the WDT fires and resets the CPU.
    tb bist_verify_pc_test
    commands
        silent
        set $arr = $sp
        set {int}$arr = 0
        set $sp = $arr - 256
        call wdt_init(500)
        set $sp = $arr
        monitor resume
    end
    continue
    """
    time.sleep(5)
    with openocd_dut.run():
        gdb_process = gdb_instance.attach(script)
        time.sleep(5)  # Wait for GDB to attach and run the script
        try:
            for expected_output in expected_outputs:
                dut.expect(expected_output, timeout=30)
        finally:
            gdb_instance.stop(gdb_process)

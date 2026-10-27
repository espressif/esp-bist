import time

import pytest

from tests.idf_targets import pytestmark  # noqa: F401


def test_interrupt_success(dut, target):
    tests_names = ["test_BIST_Interrupt_Source_Map", "test_BIST_Hardware_Interrupt"]
    expected_outputs = [f"{test}:PASS" for test in tests_names]

    # expect from what esptool.py printed to sys.stdout
    for expected_output in expected_outputs:
        dut.expect(expected_output)


def interrupt_error_test(
    openocd_dut, gdb_instance, dut, test_name, test_bp, injected_value
):
    """
    Test the interrupt functionality with a fault injection.
    After the interrupt loop is finalized (defaults to 8 interrupts), the interrupt
    count is set to the injected value, which should be less than 8, forcing the
    test to fail.
    """
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
        set interrupt_count={}
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
def test_interrupt_source_map_error_device(openocd_dut, gdb_instance, dut, target):
    interrupt_error_test(
        openocd_dut,
        gdb_instance,
        dut,
        "test_BIST_Interrupt_Source_Map",
        "bist_interrupt_source_map_count",
        "0",
    )

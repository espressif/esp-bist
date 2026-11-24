import time

import pytest

from tests.idf_targets import pytestmark  # noqa: F401


def test_cpu_stack_success(dut, target):
    tests_names = ["test_BIST_Cpu_Stack_Overflow"]
    expected_outputs = [f"{test}:PASS" for test in tests_names]

    # expect from what esptool.py printed to sys.stdout
    for expected_output in expected_outputs:
        dut.expect(expected_output)


def cpu_stack_error(openocd_dut, gdb_instance, dut, test_name, test_bp, injected_value):
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
        set count_max={}
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
def test_cpu_stack_error_device(openocd_dut, gdb_instance, dut, target):
    cpu_stack_error(
        openocd_dut,
        gdb_instance,
        dut,
        "test_BIST_Cpu_Stack_Overflow",
        "bist_cpu_stack_overflow_data",
        "10",
    )

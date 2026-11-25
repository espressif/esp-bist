import time

import pytest

from tests.idf_targets import pytestmark  # noqa: F401


def test_gpio_input_reciprocal_success(dut, target):
    if target != "esp32p4":
        pytest.skip("Reciprocal GPIO input test is ESP32-P4 only")

    tests_names = [
        "test_BIST_Gpio_Input_Reciprocal",
        "test_BIST_Gpio_Input_Reciprocal_Invalid_Pin",
    ]
    expected_outputs = [f"{test}:PASS" for test in tests_names]

    # expect from what esptool.py printed to sys.stdout
    for expected_output in expected_outputs:
        dut.expect(expected_output)


def gpio_input_reciprocal_error_test(
    openocd_dut, gdb_instance, dut, test_name, test_bp, inject_cmd
):
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
def test_gpio_input_reciprocal_level_mismatch_device(
    openocd_dut, gdb_instance, dut, target
):
    if target != "esp32p4":
        pytest.skip("Reciprocal GPIO input test is ESP32-P4 only")

    gpio_input_reciprocal_error_test(
        openocd_dut,
        gdb_instance,
        dut,
        "test_BIST_Gpio_Input_Reciprocal",
        "bist_gpio_input_verify",
        "set bist_gpio_input_shared_results[1].level = bist_gpio_input_shared_results[0].level ^ 1",
    )

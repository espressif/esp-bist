import time

import pytest

from tests.idf_targets import pytestmark  # noqa: F401


def test_io(dut, target):
    if target in ("esp32h4"):
        pytest.skip("Test is not available for this board")

    tests_names = [
        "test_BIST_ANALOG_IO_INVALID_ADC",
        "test_BIST_ANALOG_IO_LOW_LEVEL",
        "test_BIST_ANALOG_IO_HIGH_LEVEL",
    ]
    if target == "esp32c3":
        tests_names.append("test_BIST_ANALOG_IO_REFERENCE")

    expected_outputs = [f"{test}:PASS" for test in tests_names]

    # expect from what esptool.py printed to sys.stdout
    for expected_output in expected_outputs:
        dut.expect(expected_output)


def analog_io_error_test(
    openocd_dut, gdb_instance, dut, test_name, test_bp, injected_value
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
        set raw={}
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
def test_analog_io_low_level_error_device(openocd_dut, gdb_instance, dut, target):
    if target in ("esp32h4"):
        pytest.skip("Test is not available for this board")
    analog_io_error_test(
        openocd_dut,
        gdb_instance,
        dut,
        "test_BIST_ANALOG_IO_LOW_LEVEL",
        "bist_adc_low_check",
        "0xFFFF",
    )


@pytest.mark.injection
def test_analog_io_high_level_error_device(openocd_dut, gdb_instance, dut, target):
    if target in ("esp32h4"):
        pytest.skip("Test is not available for this board")
    analog_io_error_test(
        openocd_dut,
        gdb_instance,
        dut,
        "test_BIST_ANALOG_IO_HIGH_LEVEL",
        "bist_adc_high_check",
        "0",
    )


@pytest.mark.injection
def test_analog_io_reference_error_device(openocd_dut, gdb_instance, dut, target):
    if target != "esp32c3":
        pytest.skip("Reference ADC test is only available on ESP32-C3")
    analog_io_error_test(
        openocd_dut,
        gdb_instance,
        dut,
        "test_BIST_ANALOG_IO_REFERENCE",
        "bist_adc_reference_check",
        "0",
    )

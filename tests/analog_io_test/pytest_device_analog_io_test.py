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

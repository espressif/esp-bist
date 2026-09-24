import pytest

from tests.idf_targets import pytestmark  # noqa: F401


def test_multicore_dispatch(dut, target):
    if target != "esp32p4":
        pytest.skip("Multicore dispatch is ESP32-P4 only")

    dut.expect("test_BIST_Multicore_Dispatch:PASS")

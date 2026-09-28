import pytest
import queue

from tests.idf_targets import pytestmark  # noqa: F401


def test_multicore_dispatch(qemu_instance, target):
    if target != "esp32p4":
        pytest.skip("Multicore dispatch is ESP32-P4 only")

    qemu, qemu_process, output_queue = qemu_instance
    expected_output = "test_BIST_Multicore_Dispatch:PASS"
    output_lines = []
    found = False
    try:
        while True:
            line = output_queue.get(timeout=10)
            output_lines.append(line)
            if expected_output in line:
                found = True
                break
    except queue.Empty:
        print("No more output from QEMU.")

    assert found, f"Expected output '{expected_output}' not found in QEMU output"

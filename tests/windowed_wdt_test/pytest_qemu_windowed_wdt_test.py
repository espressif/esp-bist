import time
import queue

def test_windowed_wdt_normal_success(qemu_instance):
    """Verify normal windowed feed passes without GDB intervention."""
    qemu, qemu_process, output_queue = qemu_instance
    expected_output = "test_BIST_WINDOWED_WDT_NORMAL:PASS"
    output_lines = []
    try:
        while True:
            line = output_queue.get(timeout=10)
            output_lines.append(line)
            if expected_output in line:
                break
    except queue.Empty:
        print("No more output from QEMU.")

    assert any(expected_output in line for line in output_lines), "Expected output not found in QEMU output"


def test_windowed_wdt_underflow_success(qemu_instance):
    """Verify underflow detection passes without GDB intervention."""
    qemu, qemu_process, output_queue = qemu_instance
    expected_output = "test_BIST_WINDOWED_WDT_UNDERFLOW:PASS"
    output_lines = []
    try:
        while True:
            line = output_queue.get(timeout=10)
            output_lines.append(line)
            if expected_output in line:
                break
    except queue.Empty:
        print("No more output from QEMU.")

    assert any(expected_output in line for line in output_lines), "Expected output not found in QEMU output"


def test_windowed_wdt_consecutive_success(qemu_instance):
    """Verify consecutive windowed feeds pass without GDB intervention."""
    qemu, qemu_process, output_queue = qemu_instance
    expected_output = "test_BIST_WINDOWED_WDT_CONSECUTIVE:PASS"
    output_lines = []
    try:
        while True:
            line = output_queue.get(timeout=15)
            output_lines.append(line)
            if expected_output in line:
                break
    except queue.Empty:
        print("No more output from QEMU.")

    assert any(expected_output in line for line in output_lines), "Expected output not found in QEMU output"

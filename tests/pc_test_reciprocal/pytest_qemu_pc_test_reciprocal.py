import queue
import time

import pytest

from tests.idf_targets import pytestmark  # noqa: F401

CORES = (0, 1)
FUNCTIONS = range(5)

CROSS_CHECKS = {
    0: ("bist_pc_verify_core0_on_core1", 1),
    1: ("bist_pc_verify_core1_on_core0", 0),
}

GDB_HEADER = """\
# connect to the QEMU GDB stub
target remote :1234
"""


@pytest.fixture(autouse=True)
def esp32p4_only(target):
    if target != "esp32p4":
        pytest.skip("Reciprocal PC testing is only available on ESP32-P4")


def read_until(output_queue, expected_outputs):
    output_lines = []
    found_outputs = []
    try:
        # Attempt to read all current output from QEMU
        while len(found_outputs) < len(expected_outputs):
            line = output_queue.get(timeout=10)  # Use a timeout to wait for output
            output_lines.append(line)
            for expected_output in expected_outputs:
                if expected_output in line and expected_output not in found_outputs:
                    found_outputs.append(expected_output)
    except queue.Empty:
        print("No more output from QEMU.")
    return found_outputs


def pc_error_test(qemu_debug_instance, gdb_instance, script, expected_outputs):
    _, _, output_queue = qemu_debug_instance
    gdb_process = gdb_instance.attach(script)
    time.sleep(1)
    try:
        found_outputs = read_until(output_queue, expected_outputs)
    finally:
        gdb_instance.stop(gdb_process)
    for expected_output in expected_outputs:
        assert expected_output in found_outputs, (
            f"Expected output '{expected_output}' not found in QEMU output"
        )


def test_pc_success(qemu_instance, target):
    _, _, output_queue = qemu_instance
    expected_outputs = ["test_BIST_PC:PASS"]
    found_outputs = read_until(output_queue, expected_outputs)
    for expected_output in expected_outputs:
        assert expected_output in found_outputs, (
            f"Expected output '{expected_output}' not found in QEMU output"
        )


def gdb_address_error(core, function):
    label, _ = CROSS_CHECKS[core]
    return (
        GDB_HEADER
        + f"""
    tb {label}
    commands
        set bist_pc_shared_results[{core}].returned_address[{function}] = 0
        continue
    end
    continue
"""
    )


@pytest.mark.parametrize("core", CORES, ids=lambda core: f"core{core}")
@pytest.mark.parametrize(
    "function", FUNCTIONS, ids=lambda function: f"function{function}"
)
def test_pc_expected_address_error(
    qemu_debug_instance, gdb_instance, core, function, target
):
    """Corrupt one function address and require the other core to detect it."""
    _, validating_core = CROSS_CHECKS[core]
    pc_error_test(
        qemu_debug_instance,
        gdb_instance,
        gdb_address_error(core, function),
        [
            f"Failed to validate core {core} PC results on core {validating_core}",
            "test_BIST_PC:FAIL",
        ],
    )

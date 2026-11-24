"""
Reciprocal interrupt tests (ESP32-P4).
Hardware interrupt is not expected to pass: no QEMU support for timer group 0/1.
"""

import queue
import time

import pytest

from tests.idf_targets import pytestmark  # noqa: F401

# Fields where XOR 4 always breaks exact/expected evidence checks.
SW_EVIDENCE_FIELDS = [
    "status",
    "completion_mask",
    "interrupt_count[0]",
    "interrupt_count[1]",
    "interrupt_count[2]",
]


@pytest.fixture(autouse=True)
def esp32p4_only(target):
    if target != "esp32p4":
        pytest.skip("Reciprocal interrupt testing is only available on ESP32-P4")


def read_until(output_queue, expected_outputs):
    output_lines = []
    found_outputs = []
    try:
        while len(found_outputs) < len(expected_outputs):
            line = output_queue.get(timeout=10)
            output_lines.append(line)
            for expected_output in expected_outputs:
                if expected_output in line and expected_output not in found_outputs:
                    found_outputs.append(expected_output)
    except queue.Empty:
        print("No more output from QEMU.")
    return found_outputs


def interrupt_error_test(qemu_debug_instance, gdb_instance, script, expected_outputs):
    qemu, qemu_process, output_queue = qemu_debug_instance
    gdb = gdb_instance
    gdb_process = gdb.attach(script)
    time.sleep(5)
    found_outputs = read_until(output_queue, expected_outputs)
    gdb.stop(gdb_process)
    for expected_output in expected_outputs:
        assert expected_output in found_outputs, (
            f"Expected output '{expected_output}' not found in QEMU output"
        )


def test_interrupt_success(qemu_instance, target):
    qemu, qemu_process, output_queue = qemu_instance
    expected_outputs = ["test_BIST_Interrupt_Source_Map:PASS"]
    found_outputs = read_until(output_queue, expected_outputs)
    for expected_output in expected_outputs:
        assert expected_output in found_outputs, (
            f"Expected output '{expected_output}' not found in QEMU output"
        )


def test_interrupt_source_map_count_error(qemu_debug_instance, gdb_instance, target):
    """Corrupt the first source ISR count after the walk loop finishes for that source."""
    script = """
    target remote :1234

    tb bist_interrupt_source_map_count
    commands
        set s_interrupt_count=0
        continue
    end
    continue
    """
    interrupt_error_test(
        qemu_debug_instance,
        gdb_instance,
        script,
        [
            "Failed to validate core 0 SW interrupt results on core 0",
            "test_BIST_Interrupt_Source_Map:FAIL",
        ],
    )


def test_interrupt_sw_walk_core1_count_error(qemu_debug_instance, gdb_instance, target):
    """Same count corruption on Core 1's first source (skip Core 0's three sources)."""
    script = """
    target remote :1234

    tb bist_interrupt_source_map_count
    ignore $bpnum 3
    commands
        set s_interrupt_count=0
        continue
    end
    continue
    """
    interrupt_error_test(
        qemu_debug_instance,
        gdb_instance,
        script,
        [
            "Failed to validate core 1 SW interrupt results on core 1",
            "test_BIST_Interrupt_Source_Map:FAIL",
        ],
    )


@pytest.mark.parametrize("field", SW_EVIDENCE_FIELDS)
def test_interrupt_sw_slot0_cross_check_error(
    qemu_debug_instance, gdb_instance, field, target
):
    script = """
    target remote :1234

    tb bist_interrupt_sw_verify_core0_on_core1
    commands
        set bist_interrupt_sw_shared_results[0].{field} = bist_interrupt_sw_shared_results[0].{field} ^ 4
        continue
    end
    continue
    """.format(field=field)
    interrupt_error_test(
        qemu_debug_instance,
        gdb_instance,
        script,
        [
            "Failed to validate core 0 SW interrupt results on core 1",
            "test_BIST_Interrupt_Source_Map:FAIL",
        ],
    )


@pytest.mark.parametrize("field", SW_EVIDENCE_FIELDS)
def test_interrupt_sw_slot1_cross_check_error(
    qemu_debug_instance, gdb_instance, field, target
):
    script = """
    target remote :1234

    tb bist_interrupt_sw_verify_core1_on_core0
    commands
        set bist_interrupt_sw_shared_results[1].{field} = bist_interrupt_sw_shared_results[1].{field} ^ 4
        continue
    end
    continue
    """.format(field=field)
    interrupt_error_test(
        qemu_debug_instance,
        gdb_instance,
        script,
        [
            "Failed to validate core 1 SW interrupt results on core 0",
            "test_BIST_Interrupt_Source_Map:FAIL",
        ],
    )


def test_interrupt_sw_slot0_enable_mask_cross_check_error(
    qemu_debug_instance, gdb_instance, target
):
    script = """
    target remote :1234

    tb bist_interrupt_sw_verify_core0_on_core1
    commands
        set bist_interrupt_sw_shared_results[0].enabled_mask_active[0] = bist_interrupt_sw_shared_results[0].enabled_mask_active[0] & ~(1 << 9)
        continue
    end
    continue
    """
    interrupt_error_test(
        qemu_debug_instance,
        gdb_instance,
        script,
        [
            "Failed to validate core 0 SW interrupt results on core 1",
            "test_BIST_Interrupt_Source_Map:FAIL",
        ],
    )


def test_interrupt_sw_slot1_cleared_mask_cross_check_error(
    qemu_debug_instance, gdb_instance, target
):
    script = """
    target remote :1234

    tb bist_interrupt_sw_verify_core1_on_core0
    commands
        set bist_interrupt_sw_shared_results[1].enabled_mask_cleared[0] = bist_interrupt_sw_shared_results[1].enabled_mask_cleared[0] | (1 << 9)
        continue
    end
    continue
    """
    interrupt_error_test(
        qemu_debug_instance,
        gdb_instance,
        script,
        [
            "Failed to validate core 1 SW interrupt results on core 0",
            "test_BIST_Interrupt_Source_Map:FAIL",
        ],
    )

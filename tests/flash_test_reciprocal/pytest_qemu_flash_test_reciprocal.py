"""
Reciprocal flash CRC tests (ESP32-P4).

The test requires CONFIG_ESP_BIST_MULTICORE=y and selects
CONFIG_ESP_BIST_MEMORY_FLASH_TEST_RECIPROCAL, so bist_flash_test() is the
reciprocal implementation: both HP cores compute the .flash.rodata and
.flash.text CRC32 values and cross-check each other.

The walk job is collect-only: it never compares and never returns an early
error, so a corrupted section length does not abort the walk. The
length-injection cases corrupt the covered byte count, so the computed CRC no
longer matches the post-build reference and the failure is reported by the
verifier instead.
"""

import queue
import time

import pytest

from tests.idf_targets import pytestmark  # noqa: F401

# Fields of bist_flash_result_t that the cross-core verifier compares.
EVIDENCE_FIELDS = ["status", "crc_rodata", "crc_text"]


@pytest.fixture(autouse=True)
def esp32p4_only(target):
    if target != "esp32p4":
        pytest.skip("Reciprocal flash testing is only available on ESP32-P4")


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


def flash_error_test(qemu_debug_instance, gdb_instance, script, expected_outputs):
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


def test_flash_success(qemu_instance, target):
    qemu, qemu_process, output_queue = qemu_instance
    expected_outputs = ["test_BIST_flash:PASS"]
    found_outputs = read_until(output_queue, expected_outputs)
    for expected_output in expected_outputs:
        assert expected_output in found_outputs, (
            f"Expected output '{expected_output}' not found in QEMU output"
        )


def test_flash_length_error_core0(qemu_debug_instance, gdb_instance, target):
    """Shorten the .flash.rodata length on Core 0 so its CRC no longer matches."""
    script = """
    target remote :1234

    tb bist_flash_walk_rodata
    commands
        set crc_section_len=0xFF
        continue
    end
    continue
    """
    flash_error_test(
        qemu_debug_instance,
        gdb_instance,
        script,
        ["Failed to validate core 0 flash results on core 0", "test_BIST_flash:FAIL"],
    )


def test_flash_length_error_core1(qemu_debug_instance, gdb_instance, target):
    """Same length corruption on Core 1: the first hit is Core 0, the second is Core 1."""
    script = """
    target remote :1234

    tb bist_flash_walk_rodata
    ignore $bpnum 1
    commands
        set crc_section_len=0xFF
        continue
    end
    continue
    """
    flash_error_test(
        qemu_debug_instance,
        gdb_instance,
        script,
        ["Failed to validate core 1 flash results on core 1", "test_BIST_flash:FAIL"],
    )


def test_flash_length_error_text(qemu_debug_instance, gdb_instance, target):
    """Shorten the .flash.text length on Core 0; the rodata cases cover .flash.rodata."""
    script = """
    target remote :1234

    tb bist_flash_walk_text
    commands
        set crc_section_len=0xFF
        continue
    end
    continue
    """
    flash_error_test(
        qemu_debug_instance,
        gdb_instance,
        script,
        ["Failed to validate core 0 flash results on core 0", "test_BIST_flash:FAIL"],
    )


# Corrupt core 0 evidence after both local verifications passed, so only the
# core 1 cross-validation of slot 0 can detect it.
@pytest.mark.parametrize("field", EVIDENCE_FIELDS)
def test_flash_slot0_cross_check_error(
    qemu_debug_instance, gdb_instance, field, target
):
    script = """
    target remote :1234

    tb bist_flash_verify_core0_on_core1
    commands
        set bist_flash_shared_results[0].{field} = bist_flash_shared_results[0].{field} ^ 4
        continue
    end
    continue
    """.format(field=field)
    flash_error_test(
        qemu_debug_instance,
        gdb_instance,
        script,
        ["Failed to validate core 0 flash results on core 1", "test_BIST_flash:FAIL"],
    )


# Corrupt core 1 evidence after core 1 verified it, so only the core 0
# cross-validation of slot 1 can detect it.
@pytest.mark.parametrize("field", EVIDENCE_FIELDS)
def test_flash_slot1_cross_check_error(
    qemu_debug_instance, gdb_instance, field, target
):
    script = """
    target remote :1234

    tb bist_flash_verify_core1_on_core0
    commands
        set bist_flash_shared_results[1].{field} = bist_flash_shared_results[1].{field} ^ 4
        continue
    end
    continue
    """.format(field=field)
    flash_error_test(
        qemu_debug_instance,
        gdb_instance,
        script,
        ["Failed to validate core 1 flash results on core 0", "test_BIST_flash:FAIL"],
    )

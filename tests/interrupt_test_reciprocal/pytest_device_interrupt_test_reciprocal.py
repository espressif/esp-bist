"""On-device fault injection for the reciprocal interrupt tests."""

import time

import pytest

from tests.idf_targets import pytestmark  # noqa: F401

CORES = (0, 1)

# Reciprocal interrupt tests, keyed by the prefix used in their GDB labels.
TESTS = {
    "sw": "test_BIST_Interrupt_Source_Map",
    "hw": "test_BIST_Hardware_Interrupt",
}

# ISR count evidence corrupted by the cross-check injection.
COUNT_FIELDS = {
    "sw": "interrupt_count[0]",
    "hw": "count_1",
}

GDB_HEADER = """\
# connect to OpenOCD GDB stub
target remote 127.0.0.1:3333
mon reset halt
maintenance flush register-cache
monitor gdb breakpoint_override hard
"""


def interrupt_error_test_device(
    openocd_dut, gdb_instance, dut, script, expected_outputs
):
    """Run ``script`` and require each string in ``expected_outputs`` on the serial port."""
    time.sleep(1)
    with openocd_dut.run():
        gdb_process = gdb_instance.attach(script)
        try:
            for expected_output in expected_outputs:
                dut.expect(expected_output, timeout=30)
        finally:
            gdb_instance.stop(gdb_process)


def test_interrupt_success(dut, target):
    if target != "esp32p4":
        pytest.skip("Reciprocal interrupt testing is only available on ESP32-P4")
    expected_outputs = [f"{test}:PASS" for test in TESTS.values()]

    for expected_output in expected_outputs:
        dut.expect(expected_output)


@pytest.mark.injection
def test_interrupt_source_map_count_error_device(
    openocd_dut, gdb_instance, dut, target
):
    """Zero the ISR count after Core 0's first software source trigger loop.

    The walk copies ``s_interrupt_count`` into the evidence frame right after
    the label, so Core 0's own validation sees 0 != 8 and fails.
    """
    if target != "esp32p4":
        pytest.skip("Reciprocal interrupt testing is only available on ESP32-P4")
    script = (
        GDB_HEADER
        + """
tb bist_interrupt_source_map_count
commands
        set s_interrupt_count = 0
        monitor step
        monitor resume
end
continue
"""
    )
    interrupt_error_test_device(
        openocd_dut,
        gdb_instance,
        dut,
        script,
        [
            "Failed to validate core 0 SW interrupt results on core 0",
            "test_BIST_Interrupt_Source_Map:FAIL",
        ],
    )


@pytest.mark.injection
@pytest.mark.parametrize("kind", sorted(TESTS), ids=sorted(TESTS))
@pytest.mark.parametrize("core", CORES, ids=lambda core: f"core{core}")
def test_interrupt_cross_check_error_device(
    openocd_dut, gdb_instance, dut, kind, core, target
):
    """Corrupt one shared ISR count right before the peer core validates it.

    ``sw`` targets ``bist_interrupt_sw_reciprocal.c``, ``hw`` targets
    ``bist_interrupt_reciprocal.c``. Both corrupt slot ``core`` at the label
    that precedes the dispatched validation of that slot on the other core.
    """
    if target != "esp32p4":
        pytest.skip("Reciprocal interrupt testing is only available on ESP32-P4")
    validating_core = 1 - core
    script = (
        GDB_HEADER
        + f"""
tb bist_interrupt_{kind}_verify_core{core}_on_core{validating_core}
commands
        set bist_interrupt_{kind}_shared_results[{core}].{COUNT_FIELDS[kind]} = 0
        monitor step
        monitor resume
end
continue
"""
    )
    interrupt_error_test_device(
        openocd_dut,
        gdb_instance,
        dut,
        script,
        [
            f"Failed to validate core {core} {kind.upper()} interrupt results "
            f"on core {validating_core}",
            f"{TESTS[kind]}:FAIL",
        ],
    )

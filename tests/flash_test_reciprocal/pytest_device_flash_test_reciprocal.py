"""On-device fault injection for the reciprocal flash CRC test."""

import pytest

from tests.idf_targets import pytestmark  # noqa: F401

CORES = (0, 1)
CRC_FIELDS = ("crc_rodata", "crc_text")

# Stop before each reciprocal cross-validation and corrupt the producing core's
# recorded CRC in shared SRAM so the peer core detects the mismatch.
CROSS_CHECKS = {
    0: ("bist_flash_verify_core0_on_core1", 1),
    1: ("bist_flash_verify_core1_on_core0", 0),
}

GDB_HEADER = """\
# connect to OpenOCD GDB stub
target remote 127.0.0.1:3333
set confirm off
set pagination off
mon reset halt
maintenance flush register-cache
monitor gdb breakpoint_override hard
"""


@pytest.fixture(autouse=True)
def esp32p4_only(target):
    if target != "esp32p4":
        pytest.skip("Reciprocal flash testing is only available on ESP32-P4")


def gdb_crc_error(core, field):
    label, _ = CROSS_CHECKS[core]
    return (
        GDB_HEADER
        + f"""
tb {label}
commands
        set bist_flash_shared_results[{core}].{field} = bist_flash_shared_results[{core}].{field} ^ 4
        monitor step
        monitor resume
end
continue
"""
    )


def flash_error_test_device(openocd_dut, gdb_instance, dut, script, expected_outputs):
    """Run ``script`` and require each string in ``expected_outputs`` on the serial port."""
    # Wait for the initial pytest-embedded RTS reset boot to settle before
    # attaching OpenOCD and issuing a JTAG reset halt.
    dut.expect("test_BIST_flash:PASS", timeout=15)
    with openocd_dut.run():
        gdb_process = gdb_instance.attach(script)
        try:
            for expected_output in expected_outputs:
                dut.expect(expected_output, timeout=30)
        finally:
            gdb_instance.stop(gdb_process)


def test_flash_success(dut, target):
    tests_names = ["test_BIST_flash"]
    expected_outputs = [f"{test}:PASS" for test in tests_names]

    # expect from what esptool.py printed to sys.stdout
    for expected_output in expected_outputs:
        dut.expect(expected_output)


@pytest.mark.injection
@pytest.mark.parametrize("core", CORES, ids=lambda core: f"core{core}")
@pytest.mark.parametrize("field", CRC_FIELDS, ids=CRC_FIELDS)
def test_flash_crc_error_device(openocd_dut, gdb_instance, dut, core, field, target):
    """Corrupt one calculated CRC value and require the peer core to detect it."""
    _, validating_core = CROSS_CHECKS[core]
    flash_error_test_device(
        openocd_dut,
        gdb_instance,
        dut,
        gdb_crc_error(core, field),
        [
            f"Failed to validate core {core} flash results on core {validating_core}",
            "test_BIST_flash:FAIL",
        ],
    )

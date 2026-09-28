"""On-device fault injection for the reciprocal Program Counter test."""

import time

import pytest

from tests.idf_targets import pytestmark  # noqa: F401

CORES = (0, 1)
FUNCTIONS = range(5)

# Stop before each reciprocal validation. The canonical expected-address table
# is const and can reside in Flash, so inject into the returned address stored
# in shared SRAM instead.
CROSS_CHECKS = {
    0: ("bist_pc_verify_core0_on_core1", 1),
    1: ("bist_pc_verify_core1_on_core0", 0),
}

GDB_HEADER = """\
# connect to OpenOCD GDB stub
target remote 127.0.0.1:3333
set confirm off
set pagination off
mon reset halt
maintenance flush register-cache
monitor gdb breakpoint_override hard

# The normal test uses a 10 ms watchdog to recover from invalid jumps. This
# injection only corrupts shared evidence, so extend it to 10 seconds before
# stopping at the later reciprocal-validation breakpoint. Keep initialization
# intact so the test can safely call wdt_deinit().
tb wdt_init
commands
        silent
        set $a0 = 10000000
        continue
end
"""


@pytest.fixture(autouse=True)
def esp32p4_only(target):
    if target != "esp32p4":
        pytest.skip("Reciprocal PC testing is only available on ESP32-P4")


def gdb_address_error(core, function):
    label, _ = CROSS_CHECKS[core]
    return (
        GDB_HEADER
        + f"""
tb {label}
commands
        set bist_pc_shared_results[{core}].returned_address[{function}] = 0
        monitor step
        monitor resume
end
continue
"""
    )


def pc_error_test_device(openocd_dut, gdb_instance, dut, script, expected_outputs):
    """Run ``script`` and require each string in ``expected_outputs`` on the serial port."""
    time.sleep(1)
    with openocd_dut.run():
        gdb_process = gdb_instance.attach(script)
        try:
            for expected_output in expected_outputs:
                dut.expect(expected_output, timeout=30)
        finally:
            gdb_instance.stop(gdb_process)


def test_pc_success(dut, target):
    tests_names = ["test_BIST_PC"]
    expected_outputs = [f"{test}:PASS" for test in tests_names]

    # expect from what esptool.py printed to sys.stdout
    for expected_output in expected_outputs:
        dut.expect(expected_output)


@pytest.mark.injection
@pytest.mark.parametrize("core", CORES, ids=lambda core: f"core{core}")
@pytest.mark.parametrize(
    "function", FUNCTIONS, ids=lambda function: f"function{function}"
)
def test_pc_expected_address_error_device(
    openocd_dut, gdb_instance, dut, core, function, target
):
    """Corrupt one function address and require the other core to detect it."""
    _, validating_core = CROSS_CHECKS[core]
    pc_error_test_device(
        openocd_dut,
        gdb_instance,
        dut,
        gdb_address_error(core, function),
        [
            f"Failed to validate core {core} PC results on core {validating_core}",
            "test_BIST_PC:FAIL",
        ],
    )

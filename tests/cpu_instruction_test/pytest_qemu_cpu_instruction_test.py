import pytest

from tests.idf_targets import pytestmark  # noqa: F401
import time
import queue


def test_cpu_instruction_success(qemu_instance, target):
    qemu, qemu_process, output_queue = qemu_instance
    expected_output = "test_BIST_Cpu_Instructions:PASS"
    found = False
    try:
        while True:
            line = output_queue.get(timeout=10)
            if expected_output in line:
                found = True
                break
    except queue.Empty:
        pass

    assert found, f"Expected output '{expected_output}' not found in QEMU output"


def cpu_instruction_error(
    qemu_debug_instance, gdb_instance, test_bp, injected_value, vector_id
):
    qemu, qemu_process, output_queue = qemu_debug_instance
    gdb = gdb_instance
    expected_output = "test_BIST_Cpu_Instructions:FAIL"
    output_lines = []
    # Break on the vector label, let the first operand be loaded, then corrupt
    # it so the ALU result no longer matches the expected value.
    script = """
    #connect to remote server
    target remote :1234

    # Set a breakpoint at the vector under test
    tb {}
    continue

    # Execute the first operand load and overwrite it
    stepi
    set $t0={}
    continue
    """.format(test_bp, injected_value)
    gdb_process = gdb_instance.attach(script)
    time.sleep(5)  # Wait for GDB to attach and run the script
    try:
        # Attempt to read all current output from QEMU
        while True:
            line = output_queue.get(timeout=10)  # Use a timeout to wait for output
            output_lines.append(line)
            if expected_output in line:
                break
    except queue.Empty:
        print("No more output from QEMU.")
    gdb.stop(gdb_process)
    assert any(expected_output in line for line in output_lines), (
        "Expected output not found in QEMU output"
    )
    expected_vector = "integer ALU failed, vector {}".format(vector_id)
    assert any(expected_vector in line for line in output_lines), (
        f"Expected failure vector '{vector_id}' not reported in QEMU output"
    )


@pytest.mark.parametrize(
    "test_bp, injected_value, vector_id",
    [
        # ADD 5 + 7 == 12, injecting 0 makes it 0 + 7
        ("bist_insn_add_unequal", "0", 3),
        # ADDI 1 + (-1) == 0, injecting 0 makes it 0 + (-1)
        ("bist_insn_addi_imm_neg1", "0", 8),
        # OR 1 | 2 == 3, injecting 0 makes it 0 | 2
        ("bist_insn_or_unequal", "0", 33),
        # SLT 1 < 2 == 1, injecting 5 makes it 5 < 2
        ("bist_insn_slt_lt", "5", 55),
    ],
)
def test_cpu_instruction_error(
    qemu_debug_instance, gdb_instance, test_bp, injected_value, vector_id, target
):
    cpu_instruction_error(
        qemu_debug_instance, gdb_instance, test_bp, injected_value, vector_id
    )

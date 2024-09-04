import pytest
import sys
import os
import time
import queue

# Add the 'scripts' directory to the Python path
current_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.join(current_dir, '..', '..')
scripts_path = os.path.join(project_root, 'scripts')
sys.path.append(scripts_path)

from test_utils import QEMU_RISCV, GDB_RISCV, qemu_instance, qemu_debug_instance, gdb_instance

def test_cpu_reg_success(qemu_instance):
    qemu, qemu_process, output_queue = qemu_instance
    tests_names = ["test_BIST_Cpu_Regs", "test_BIST_Cpu_Csr_Regs"]
    expected_outputs = [f"{test}:PASS" for test in tests_names]
    output_lines = []
    found_outputs = []
    try:
        # Attempt to read all current output from QEMU
        while True:
            line = output_queue.get(timeout=3)  # Use a timeout to wait for output
            output_lines.append(line)
            # Check if the line contains any of the expected outputs
            for expected_output in expected_outputs:
                if expected_output in line and expected_output not in found_outputs:
                    found_outputs.append(expected_output)
                    # If all expected outputs have been found, we can break the loop
                    if len(found_outputs) == len(expected_outputs):
                        break
    except queue.Empty:
        print("No more output from QEMU.")

    # Assert that each expected output was found in the output_lines
    for expected_output in expected_outputs:
        assert expected_output in found_outputs, f"Expected output '{expected_output}' not found in QEMU output"

def cpu_reg_error_test(qemu_debug_instance, gdb_instance, reg_name, script=None):
    qemu, qemu_process, output_queue = qemu_debug_instance
    gdb = gdb_instance
    expected_output = "FAIL"
    output_lines = []
    if script is None:
        script = '''
    #connect to remote server
    target remote :1234

    # Set a breakpoint at the specific address
    tb testRegA_{}
    continue

    # Commands to run when breakpoint is hit
    commands
        set ${}=0x55555555
        continue
    end
    '''.format(reg_name, reg_name)
    gdb_process = gdb_instance.attach(script)
    time.sleep(5) # Wait for GDB to attach and run the script
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
    assert any(expected_output in line for line in output_lines), "Expected output not found in QEMU output"

@pytest.mark.parametrize("reg_name", [
    "ra", "sp", "gp", "tp",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"
])

def test_reg_error(qemu_debug_instance, gdb_instance, reg_name):
    cpu_reg_error_test(qemu_debug_instance, gdb_instance, reg_name)

@pytest.mark.parametrize("csr_name", [
    "mtvec",
    "mscratch", "mepc", "mcause", "mtval",
    "pmpaddr0", "pmpaddr1", "pmpaddr2", "pmpaddr3", "pmpaddr4", "pmpaddr5", "pmpaddr6", "pmpaddr7", "pmpaddr8", "pmpaddr9", "pmpaddr10", "pmpaddr11", "pmpaddr12", "pmpaddr13", "pmpaddr14", "pmpaddr15"
])

def test_reg_csr_error(qemu_debug_instance, gdb_instance, csr_name):
    script = '''
#connect to remote server
target remote :1234

# Set a breakpoint at the specific address
tb testRegA_{}
continue

# Commands to run when breakpoint is hit
commands
    set $t0=0x55555555
    continue
end
'''.format(csr_name)
    cpu_reg_error_test(qemu_debug_instance, gdb_instance, csr_name, script)

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

# Now you can import from test_utils.py
from test_utils import QEMU_RISCV, GDB_RISCV, qemu_instance, qemu_debug_instance, gdb_instance

def test_flash_success(qemu_instance):
    qemu, qemu_process, output_queue = qemu_instance
    tests_names = ["test_BIST_flash"]
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

def flash_error_test(qemu_debug_instance, gdb_instance, test_name, test_bp, injected_value):
    qemu, qemu_process, output_queue = qemu_debug_instance
    gdb = gdb_instance
    expected_output = "{}:FAIL".format(test_name)
    output_lines = []
    script = '''
    #connect to remote server
    target remote :1234

    # Set a breakpoint at the specific address
    tb {}
    continue

    # Commands to run when breakpoint is hit
    commands
        set crc_section_len={}
        continue
    end
    '''.format(test_bp, injected_value)
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


def test_bist_flash_test_data(qemu_debug_instance, gdb_instance):
    flash_error_test(qemu_debug_instance, gdb_instance, "test_BIST_flash", "bist_flash_test_data", "0xFF")

def test_bist_flash_test_text(qemu_debug_instance, gdb_instance):
    flash_error_test(qemu_debug_instance, gdb_instance, "test_BIST_flash",  "bist_flash_test_text", "0xFF")



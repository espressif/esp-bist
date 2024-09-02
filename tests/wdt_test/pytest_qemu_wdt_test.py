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

def wdt_test_routine(qemu_debug_instance, gdb_instance, test_name, test_bp, injected_value, expected_output):
    qemu, qemu_process, output_queue = qemu_debug_instance
    gdb = gdb_instance
    expected_output = "{}:{}".format(test_name, expected_output)
    output_lines = []
    script = '''
    #connect to remote server
    target remote :1234

    # Set a breakpoint at the specific address
    tb {}
    continue

    # Commands to run when breakpoint is hit
    commands
        set wdt_timeout_ms={}
        continue
    end
    '''.format(test_bp, injected_value)
    gdb_process = gdb_instance.attach(script)
    time.sleep(5) # Wait for GDB to attach and run the script
    try:
        # Attempt to read all current output from QEMU
        while True:
            line = output_queue.get(timeout=3)  # Use a timeout to wait for output
            output_lines.append(line)
            if expected_output in line:
                break
    except queue.Empty:
        print("No more output from QEMU.")

    gdb.stop(gdb_process)
    assert any(expected_output in line for line in output_lines), "Expected output not found in QEMU output"

def test_wdt_error(qemu_debug_instance, gdb_instance):
    wdt_test_routine(qemu_debug_instance, gdb_instance, "test_BIST_WDT", "bist_test_wdt_timeout", "10000", "FAIL")

def test_wdt_success(qemu_debug_instance, gdb_instance):
    wdt_test_routine(qemu_debug_instance, gdb_instance, "test_BIST_WDT", "bist_test_wdt_timeout", "100", "PASS")

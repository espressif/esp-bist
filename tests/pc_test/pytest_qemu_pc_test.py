import time
import queue

from tests.idf_targets import pytestmark  # noqa: F401

# pcTestFunctions is a local array copied onto the stack via memcpy, so it is not
# reachable by name. Its offset within the frame is target dependent: ESP32-P4
# holds a fifth entry (TCM region) and places the array at $sp + 12, while the
# other SoCs place it at $sp. Locate the first element by matching the known
# pointer pair instead of assuming a fixed offset.
LOCATE_PC_TEST_ARRAY = '''
        set $slot = 0
        set $i = 0
        while $i < 12
            set $addr = (unsigned int)$sp + $i * 4
            if $slot == 0 && *(unsigned int *)$addr == (unsigned int)&pcTestFunction0
                if *(unsigned int *)($addr + 4) == (unsigned int)&pcTestFunction1
                    set $slot = $addr
                end
            end
            set $i = $i + 1
        end
'''


def test_pc_success(qemu_instance, target):
    qemu, qemu_process, output_queue = qemu_instance
    tests_names = ["test_BIST_PC"]
    expected_outputs = [f"{test}:PASS" for test in tests_names]
    output_lines = []
    found_outputs = []
    try:
        # Attempt to read all current output from QEMU
        while True:
            line = output_queue.get(timeout=10)  # Use a timeout to wait for output
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

'''
The script inject the wrong expected value to the PC BIST test, without triggering WDT
'''
def test_pc_error_no_wdt(qemu_debug_instance, gdb_instance, target):
    qemu, qemu_process, output_queue = qemu_debug_instance
    gdb = gdb_instance
    expected_output = "test_BIST_PC:FAIL"
    output_lines = []
    script = '''
    #connect to remote server
    target remote :1234

    # Break at the PC verification loop entry, then flip bit 2 of the first
    # function pointer to simulate a stuck-at fault on PC bit 2: the CPU jumps
    # to an offset within the function body (skipping the lui that loads the
    # address constant), so the called code returns a wrong value and the
    # verification detects the mismatch.
    tb bist_verify_pc_test
    commands
{locate}
        set *(int*)$slot = *(int*)$slot ^ 4
        continue
    end
    continue
    '''.format(locate=LOCATE_PC_TEST_ARRAY)
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

def test_pc_error_wdt(qemu_debug_instance, gdb_instance, target):
    qemu, qemu_process, output_queue = qemu_debug_instance
    gdb = gdb_instance
    expected_outputs = ["Reset reason: 1", "Reset reason: 7"]
    output_lines = []
    found_outputs = []

    script = '''
    #connect to remote server
    target remote :1234

    # Break at the PC verification loop entry, then zero the first function
    # pointer. The resulting jump to address 0 raises an instruction access
    # fault that the firmware cannot recover from, so the WDT fires and resets
    # the CPU.
    tb bist_verify_pc_test
    commands
{locate}
        set *(int*)$slot = 0
        continue
    end
    continue
    '''.format(locate=LOCATE_PC_TEST_ARRAY)
    gdb_process = gdb_instance.attach(script)
    time.sleep(5) # Wait for GDB to attach and run the script
    try:
        # Attempt to read all current output from QEMU
        while True:
            line = output_queue.get(timeout=10)  # Use a timeout to wait for output
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

    gdb.stop(gdb_process)
    # Assert that each expected output was found in the output_lines
    for expected_output in expected_outputs:
        assert expected_output in found_outputs, f"Expected output '{expected_output}' not found in QEMU output"

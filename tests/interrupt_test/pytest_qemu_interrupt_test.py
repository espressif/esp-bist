'''
Test the interrupt functionality.
Hardware interrupt is not expected to pass: no QEMU support for timer group 0/1.
'''
import time
import queue

from tests.idf_targets import pytestmark  # noqa: F401


def test_interrupt_success(qemu_instance, target):
    qemu, qemu_process, output_queue = qemu_instance
    tests_names = ["test_BIST_Interrupt_Source_Map"]
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


def interrupt_error_test(qemu_debug_instance, gdb_instance, test_name, test_bp, injected_value):
    '''
    Test the interrupt functionality with a fault injection.
    After the interrupt loop is finalized (defaults to 8 interrupts), the interrupt
    count is set to the injected value, which should be less than 8, forcing the 
    test to fail.
    '''
    qemu, qemu_process, output_queue = qemu_debug_instance
    gdb = gdb_instance
    expected_output = "{}:FAIL".format(test_name)
    output_lines = []
    script = '''
    #connect to remote server
    target remote :1234

    # Set a breakpoint at the specific address
    tb {}
    commands
        set interrupt_count={}
        continue
    end
    continue
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


def test_interrupt_source_map_error(qemu_debug_instance, gdb_instance, target):
    interrupt_error_test(qemu_debug_instance, gdb_instance, "test_BIST_Interrupt_Source_Map",
                         "bist_interrupt_source_map_count", "0")

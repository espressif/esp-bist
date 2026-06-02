"""
Configuration and fixtures for pytest to support QEMU and GDB testing on RISC-V ESP32 architecture.

This module provides classes and pytest fixtures to manage QEMU emulation and GDB debugging sessions
for ESP32 RISC-V based projects.

Classes:
        QEMU_RISCV: Manages QEMU emulation process for a given target.
        GDB_RISCV: Manages GDB debugging session with script execution.

Fixtures:
        qemu_instance: Provides a running QEMU instance for testing.
        qemu_debug_instance: Provides a running QEMU instance in debug mode (with GDB support).
        gdb_instance: Provides a GDB debugger instance for debugging.

Command-line Options:
        --executable: Name of the executable file (without extension) to be tested/debugged.

QEMU uses the same ``target`` as idf-ci (``@pytest.mark.parametrize`` / ``tests/idf_targets.py``), not a separate CLI flag.
"""

import pytest
import os
import subprocess
import threading
import queue

class QEMU_RISCV(object):
    def __init__(self, directory=None, executable=None, target="esp32c3"):
        if directory is None or not os.path.isdir(directory):
            raise ValueError("Directory must be specified.")
        if executable is None:
            executable = os.path.basename(os.path.normpath(directory))
            drive_path = os.path.join(directory, "build", executable + ".bin")
            if not os.path.isfile(drive_path):
                raise ValueError(f"Executable {drive_path} not found in build directory.")
            print(f"Executable is not specified. Using the default executable: {executable}")

        self.current_dir = directory
        self.executable = executable
        self.target = target

    def start(self, debug=False):
        """Starts QEMU and returns the process and output queue."""
        qemu_image = "{}_qemu_image.bin".format(self.executable)
        drive = "file={}/build/{},if=mtd,format=raw".format(self.current_dir, qemu_image)
        qemu_command = ["qemu-system-riscv32", "-nographic", "-icount", "3", "-machine", self.target, "-drive", drive]
        if debug:
            qemu_command = ["qemu-system-riscv32", "-s", "-S", "-nographic", "-icount", "3", "-machine", self.target, "-drive", drive]
        print("Starting QEMU with command: " + " ".join(qemu_command))
        qemu_process = subprocess.Popen(qemu_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, bufsize=1, universal_newlines=True)
        output_queue = queue.Queue()

        # Function to read process output in a separate thread
        def enqueue_output(out, queue):
            for line in iter(out.readline, ''):
                self.log(line)
                queue.put(line)
            out.close()

        # Start threads for reading stdout and stderr
        threading.Thread(target=enqueue_output, args=(qemu_process.stdout, output_queue)).start()
        threading.Thread(target=enqueue_output, args=(qemu_process.stderr, output_queue)).start()

        return qemu_process, output_queue

    def stop(self, qemu_process):
        """Terminates the QEMU process."""
        print("Terminating QEMU process.")
        # Check if the process is still running
        if qemu_process.poll() is None:
            # Try to terminate the process using SIGTERM
            qemu_process.terminate()
            try:
                # Wait for the process to terminate, giving it a chance to clean up
                qemu_process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                # If the process does not terminate within the timeout, force kill it
                qemu_process.kill()
                qemu_process.wait()  # Wait again to ensure the process has been killed

    def log(self, message):
        """Logs a message."""
        print("QEMU: " + message)

class GDB_RISCV(object):
    TEMP_FILE_NAME = "gdb_script_temp.gdb"

    def __init__(self, directory=None, executable=None):
        if directory is None or not os.path.isdir(directory):
            raise ValueError("Directory must be specified.")
        if executable is None:
            executable = os.path.basename(os.path.normpath(directory))
            drive_path = os.path.join(directory, "build", executable + ".bin")
            if not os.path.isfile(drive_path):
                raise ValueError(f"Executable {drive_path} not found in build directory.")
            print(f"Executable is not specified. Using the default executable: {executable}")

        self.current_dir = directory
        self.executable = executable
        self.temp_file_path = self.current_dir + "/" + self.TEMP_FILE_NAME
        self._gdb_process = None

    def attach(self, script):
        # Creates a temporary file with script content
        with open(self.temp_file_path, "w") as file:
            file.write(script)

        # Starts GDB and runs the script
        elf_file = "{}.elf".format(self.executable)
        gdb_command = ["riscv32-esp-elf-gdb", f"{self.current_dir}/build/{elf_file}","-q", f"--command={self.temp_file_path}"]
        print("Starting GDB with command: " + " ".join(gdb_command))
        self._gdb_process = subprocess.Popen(gdb_command)
        return self._gdb_process

    def stop(self, gdb_process=None):
        """Terminates the GDB process and removes the temporary script file."""
        proc = gdb_process or self._gdb_process
        if os.path.exists(self.temp_file_path):
            print("Removing temporary files.")
            os.remove(self.temp_file_path)
        if proc is not None:
            print("Terminating GDB process.")
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
        self._gdb_process = None

@pytest.fixture
def qemu_instance(request, target):
    executable = request.config.getoption("--executable")
    qemu = QEMU_RISCV(os.path.dirname(request.fspath), executable=executable, target=target)
    qemu_process, output_queue = qemu.start()
    yield qemu, qemu_process, output_queue
    qemu.stop(qemu_process)


@pytest.fixture
def qemu_debug_instance(request, target):
    executable = request.config.getoption("--executable")
    qemu = QEMU_RISCV(os.path.dirname(request.fspath), executable=executable, target=target)
    qemu_process, output_queue = qemu.start(debug=True)
    yield qemu, qemu_process, output_queue
    qemu.stop(qemu_process)


@pytest.fixture
def gdb_instance(request):
    executable = request.config.getoption("--executable")
    gdb = GDB_RISCV(os.path.dirname(request.fspath), executable=executable)
    yield gdb
    if gdb._gdb_process is not None:
        gdb.stop()


def pytest_addoption(parser):
    """Add custom command-line options for pytest."""
    parser.addoption(
        "--executable",
        action="store",
        help="Name of the executable (without file extension). "
    )

import subprocess
import threading
import queue
import time
import os
import pytest

BIST_ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

class QEMU_RISCV(object):
    def __init__(self, directory=None):
        if directory is None:
            raise ValueError("Directory must be specified.")
        self.current_dir = directory

    def start(self, debug=False):
        """Starts QEMU and returns the process and output queue."""
        qemu_command = ["qemu-system-riscv32", "-nographic", "-icount", "3", "-machine", "esp32c3", "-drive", "file={}/build/critical_fw_esp32c3_qemu_image.bin,if=mtd,format=raw".format(self.current_dir)]
        if debug:
            qemu_command = ["qemu-system-riscv32", "-s", "-S", "-nographic", "-icount", "3", "-machine", "esp32c3", "-drive", "file={}/build/critical_fw_esp32c3_qemu_image.bin,if=mtd,format=raw".format(self.current_dir)]
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
        print("QUEMU: " + message)

class GDB_RISCV(object):
    TEMP_FILE_NAME = "gdb_script_temp.gdb"

    def __init__(self, directory=None):
        if directory is None:
            raise ValueError("Directory must be specified.")
        self.current_dir = directory
        self.temp_file_path = self.current_dir + "/" + self.TEMP_FILE_NAME


    def attach(self, script):
        # Creates a temporary file with script content
        with open(self.temp_file_path, "w") as file:
            file.write(script)

        # Starts GDB and runs the script
        gdb_command = [f"{BIST_ROOT_DIR}/tools/riscv32-esp-elf-gdb/bin/riscv32-esp-elf-gdb", f"{self.current_dir}/build/critical_fw_esp32c3.elf","-q", f"--command={self.temp_file_path}"]
        print("Starting GDB with command: " + " ".join(gdb_command))
        gdb_process = subprocess.Popen(gdb_command)
        return gdb_process

    def stop(self, gdb_process):
        """Terminates the GDB process."""
        print("Removing temporary files.")
        subprocess.run(["rm", self.temp_file_path])
        print("Terminating GDB process.")
        gdb_process.terminate()
        try:
            gdb_process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            gdb_process.kill()
            gdb_process.wait()

@pytest.fixture
def qemu_instance(request):
    qemu = QEMU_RISCV(os.path.dirname(request.fspath))
    qemu_process, output_queue = qemu.start()
    yield qemu, qemu_process, output_queue
    qemu.stop(qemu_process)

@pytest.fixture
def qemu_debug_instance(request):
    qemu = QEMU_RISCV(os.path.dirname(request.fspath))
    qemu_process, output_queue = qemu.start(debug=True)
    yield qemu, qemu_process, output_queue
    qemu.stop(qemu_process)

@pytest.fixture
def gdb_instance(request):
    return GDB_RISCV(os.path.dirname(request.fspath))


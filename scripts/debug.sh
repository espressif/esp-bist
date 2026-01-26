#!/bin/bash
# Debug script: starts OpenOCD in background and launches GDB
# Usage: ./debug.sh <elf_file> [gdbinit_file]
#        or GDBINIT=<path/to/gdbinit> ./debug.sh <elf_file>

set -e

if [ $# -lt 1 ]; then
    echo "Usage: $0 <elf_file> [gdbinit_file]"
    echo "   or: GDBINIT=<path/to/gdbinit> $0 <elf_file>"
    exit 1
fi

ELF_FILE="$1"
GDBINIT_FILE="${2:-${GDBINIT}}"

# Start OpenOCD in background
echo "Starting OpenOCD..."
openocd -c "set ESP_RTOS none" -f board/esp32c3-builtin.cfg -c "init; halt; esp appimage_offset 0x1000" > /dev/null 2>&1 &
OPENOCD_PID=$!

# Wait for OpenOCD to initialize
echo "Waiting for OpenOCD to initialize..."
sleep 2

# Cleanup function
cleanup() {
    echo "Cleaning up OpenOCD (PID: $OPENOCD_PID)..."
    kill $OPENOCD_PID 2>/dev/null || true
}

# Set trap to cleanup on exit
trap cleanup EXIT

# Launch GDB
echo "Launching GDB..."
if [ -n "$GDBINIT_FILE" ] && [ -f "$GDBINIT_FILE" ]; then
    echo "Using GDB script: $GDBINIT_FILE"
    riscv32-esp-elf-gdb -x "$GDBINIT_FILE" "$ELF_FILE"
else
    echo "Using default GDB commands"
    riscv32-esp-elf-gdb -ex "set pagination off" -ex "target remote 127.0.0.1:3333" -ex "mon reset halt" "$ELF_FILE"
fi

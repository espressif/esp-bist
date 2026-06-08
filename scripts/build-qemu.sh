#!/usr/bin/env bash
set -euo pipefail

QEMU_SRC="/opt/qemu-src"

if [ ! -f "$QEMU_SRC/configure" ]; then
    echo "Error: QEMU source not found at $QEMU_SRC"
    echo "Make sure the qemu bind mount is configured in devcontainer.json"
    exit 1
fi

cd /opt
rm -rf qemu
cp -a "$QEMU_SRC" qemu
cd qemu
rm -rf build
mkdir build
cd build
../configure --target-list=riscv32-softmmu
ninja

echo ""
echo "QEMU built successfully."
echo "Run:  export PATH=/opt/qemu/build:\$PATH"

#!/usr/bin/env bash
# Build MCUboot for all supported ESP targets.
#
# Usage:
#   ./build_mcuboot.sh [target ...]
#
# Examples:
#   ./build_mcuboot.sh              # build for all targets
#   ./build_mcuboot.sh esp32c3      # build for esp32c3 only
#   ./build_mcuboot.sh esp32c3 esp32h2
#
# Run inside the devcontainer (or any environment where IDF_PATH and
# MCUBOOT_PATH are set).

set -euo pipefail

ALL_TARGETS=(esp32c3 esp32c5 esp32c6 esp32c61 esp32h2)

if [ $# -gt 0 ]; then
    TARGETS=("$@")
else
    TARGETS=("${ALL_TARGETS[@]}")
fi

: "${IDF_PATH:?IDF_PATH is not set. Source export.sh first.}"
: "${MCUBOOT_PATH:?MCUBOOT_PATH is not set (e.g. /opt/mcuboot).}"

MCUBOOT_ESPRESSIF="$MCUBOOT_PATH/boot/espressif"
if [ ! -d "$MCUBOOT_ESPRESSIF" ]; then
    echo "Error: MCUboot espressif directory not found at $MCUBOOT_ESPRESSIF"
    exit 1
fi

BUILD_DIR="$MCUBOOT_ESPRESSIF/build"

# Source IDF environment if not already active
if ! command -v idf.py &>/dev/null; then
    echo "Sourcing IDF export.sh ..."
    . "$IDF_PATH/export.sh"
fi

passed=()
failed=()

rm -rf "$BUILD_DIR"

for target in "${TARGETS[@]}"; do
    echo ""
    echo "========================================"
    echo "  Building MCUboot for $target"
    echo "========================================"

    if cmake \
        -DCMAKE_TOOLCHAIN_FILE="$MCUBOOT_ESPRESSIF/tools/toolchain-${target}.cmake" \
        -DMCUBOOT_TARGET="${target}" \
        -DESP_HAL_PATH="$IDF_PATH" \
        -B "$BUILD_DIR" \
        -GNinja \
        "$MCUBOOT_ESPRESSIF" \
       && ninja -C "$BUILD_DIR"; then

        echo ">> $target: OK  ->  $BUILD_DIR/mcuboot_${target}.bin"
        passed+=("$target")
    else
        echo ">> $target: FAILED"
        failed+=("$target")
    fi
done

echo ""
echo "========================================"
echo "  MCUboot Build Summary"
echo "========================================"
echo "Passed: ${#passed[@]}"
for t in "${passed[@]}"; do echo "  ✓ $t"; done

if [ ${#failed[@]} -gt 0 ]; then
    echo "Failed: ${#failed[@]}"
    for t in "${failed[@]}"; do echo "  ✗ $t"; done
    exit 1
else
    echo "All MCUboot builds completed successfully."
    echo "Binaries are in: $BUILD_DIR/"
fi

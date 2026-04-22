#!/usr/bin/env bash
# Build all BIST tests for a given ESP target.
#
# Usage:
#   ./build_all_tests.sh <target>
#
# Example:
#   ./build_all_tests.sh esp32c3
#
# Run inside the devcontainer (or any environment where IDF_PATH is set).

set -euo pipefail

if [ $# -lt 1 ]; then
    echo "Usage: $0 <target>"
    echo "Supported targets: esp32c3, esp32c6, esp32h2"
    exit 1
fi

TARGET="$1"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
TESTS_DIR="$ROOT_DIR/tests"

passed=()
failed=()

for test_dir in "$TESTS_DIR"/*/; do
    # Skip directories without a CMakeLists.txt
    [ -f "$test_dir/CMakeLists.txt" ] || continue

    test_name="$(basename "$test_dir")"
    build_dir="$test_dir/build"

    echo ""
    echo "========================================"
    echo "  Building: $test_name for $TARGET"
    echo "========================================"

    rm -rf "$build_dir"
    if cmake -GNinja -B "$build_dir" -DSOC_TARGET="$TARGET" "$test_dir" \
       && ninja -C "$build_dir"; then
        echo ">> $test_name: OK"
        passed+=("$test_name")
    else
        echo ">> $test_name: FAILED"
        failed+=("$test_name")
    fi
done

echo ""
echo "========================================"
echo "  Build Summary ($TARGET)"
echo "========================================"
echo "Passed: ${#passed[@]}"
for t in "${passed[@]}"; do echo "  ✓ $t"; done

if [ ${#failed[@]} -gt 0 ]; then
    echo "Failed: ${#failed[@]}"
    for t in "${failed[@]}"; do echo "  ✗ $t"; done
    exit 1
else
    echo "All tests built successfully."
fi

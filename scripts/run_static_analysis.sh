#!/usr/bin/env bash
# Simple static analysis runner for esp-bist (example)
# Writes outputs to docs/validation-reports/static-analysis/
set -euo pipefail

# Fix git safe.directory issue in containers
REPO_ROOT=$(pwd)
git config --global --add safe.directory "$REPO_ROOT" 2>/dev/null || true

# Accept project directory as first argument (default: current directory)
PROJECT_DIR="${1:-.}"

OUT_DIR="./docs/validation-reports/$PROJECT_DIR/static-analysis"
mkdir -p "$OUT_DIR"

echo "===========================================" > "$OUT_DIR/env.txt"
echo "Static Analysis Environment Information" >> "$OUT_DIR/env.txt"
echo "===========================================" >> "$OUT_DIR/env.txt"
echo "" >> "$OUT_DIR/env.txt"

# Analysis timestamp
echo "Analysis Date: $(date --iso-8601=seconds)" >> "$OUT_DIR/env.txt"
echo "Analysis Host: $(hostname)" >> "$OUT_DIR/env.txt"
echo "Analysis User: $(whoami)" >> "$OUT_DIR/env.txt"
echo "" >> "$OUT_DIR/env.txt"

# Git repository information
echo "-------------------------------------------" >> "$OUT_DIR/env.txt"
echo "Git Repository Information" >> "$OUT_DIR/env.txt"
echo "-------------------------------------------" >> "$OUT_DIR/env.txt"
echo "Current Branch: $(git rev-parse --abbrev-ref HEAD)" >> "$OUT_DIR/env.txt"
echo "Commit Hash: $(git rev-parse HEAD)" >> "$OUT_DIR/env.txt"
echo "Commit Short Hash: $(git rev-parse --short HEAD)" >> "$OUT_DIR/env.txt"
echo "Commit Author: $(git log -1 --format='%an <%ae>')" >> "$OUT_DIR/env.txt"
echo "Commit Date: $(git log -1 --format='%ai')" >> "$OUT_DIR/env.txt"
echo "Commit Message: $(git log -1 --format='%s')" >> "$OUT_DIR/env.txt"
echo "" >> "$OUT_DIR/env.txt"

# Tags
CURRENT_TAG=$(git describe --tags --exact-match 2>/dev/null || echo "none")
LATEST_TAG=$(git describe --tags --abbrev=0 2>/dev/null || echo "none")
echo "Current Tag: $CURRENT_TAG" >> "$OUT_DIR/env.txt"
echo "Latest Tag: $LATEST_TAG" >> "$OUT_DIR/env.txt"
echo "Tag Description: $(git describe --tags 2>/dev/null || echo "none")" >> "$OUT_DIR/env.txt"
echo "" >> "$OUT_DIR/env.txt"


# Build system information
echo "-------------------------------------------" >> "$OUT_DIR/env.txt"
echo "Build System Information" >> "$OUT_DIR/env.txt"
echo "-------------------------------------------" >> "$OUT_DIR/env.txt"
echo "CMake Version: $(cmake --version 2>/dev/null | head -n1 || echo "not found")" >> "$OUT_DIR/env.txt"
echo "Ninja Version: $(ninja --version 2>/dev/null || echo "not found")" >> "$OUT_DIR/env.txt"
echo "Make Version: $(make --version 2>/dev/null | head -n1 || echo "not found")" >> "$OUT_DIR/env.txt"
echo "" >> "$OUT_DIR/env.txt"

# Compiler information
echo "-------------------------------------------" >> "$OUT_DIR/env.txt"
echo "Compiler Information" >> "$OUT_DIR/env.txt"
echo "-------------------------------------------" >> "$OUT_DIR/env.txt"
if command -v riscv32-esp-elf-gcc >/dev/null 2>&1; then
  echo "RISC-V GCC Version:" >> "$OUT_DIR/env.txt"
  riscv32-esp-elf-gcc --version 2>&1 | sed 's/^/  /' >> "$OUT_DIR/env.txt"
  echo "" >> "$OUT_DIR/env.txt"
  echo "RISC-V GCC Target: $(riscv32-esp-elf-gcc -dumpmachine 2>/dev/null || echo "unknown")" >> "$OUT_DIR/env.txt"
else
  echo "riscv32-esp-elf-gcc: not found" >> "$OUT_DIR/env.txt"
fi
echo "" >> "$OUT_DIR/env.txt"

# Static analysis tools
echo "-------------------------------------------" >> "$OUT_DIR/env.txt"
echo "Static Analysis Tools" >> "$OUT_DIR/env.txt"
echo "-------------------------------------------" >> "$OUT_DIR/env.txt"
if command -v cppcheck >/dev/null 2>&1; then
  echo "Cppcheck Version:" >> "$OUT_DIR/env.txt"
  cppcheck --version 2>&1 | sed 's/^/  /' >> "$OUT_DIR/env.txt"
else
  echo "cppcheck: not found" >> "$OUT_DIR/env.txt"
fi
echo "" >> "$OUT_DIR/env.txt"

# System information
echo "-------------------------------------------" >> "$OUT_DIR/env.txt"
echo "System Information" >> "$OUT_DIR/env.txt"
echo "-------------------------------------------" >> "$OUT_DIR/env.txt"
echo "OS: $(uname -s)" >> "$OUT_DIR/env.txt"
echo "Kernel: $(uname -r)" >> "$OUT_DIR/env.txt"
echo "Architecture: $(uname -m)" >> "$OUT_DIR/env.txt"
if [ -f /etc/os-release ]; then
  echo "Distribution: $(. /etc/os-release && echo "$PRETTY_NAME")" >> "$OUT_DIR/env.txt"
fi
echo "" >> "$OUT_DIR/env.txt"

# Python environment (if relevant)
echo "-------------------------------------------" >> "$OUT_DIR/env.txt"
echo "Python Environment" >> "$OUT_DIR/env.txt"
echo "-------------------------------------------" >> "$OUT_DIR/env.txt"
echo "Python Version: $(python3 --version 2>/dev/null || echo "not found")" >> "$OUT_DIR/env.txt"
echo "Python Path: $(which python3 2>/dev/null || echo "not found")" >> "$OUT_DIR/env.txt"
echo "" >> "$OUT_DIR/env.txt"

echo "===========================================" >> "$OUT_DIR/env.txt"
echo "End of Environment Information" >> "$OUT_DIR/env.txt"
echo "===========================================" >> "$OUT_DIR/env.txt"


# Ensure compile_commands.json exists (use CMake option: -DCMAKE_EXPORT_COMPILE_COMMANDS=ON)
if [ ! -f "$PROJECT_DIR/build/compile_commands.json" ]; then
  echo "Error: build/compile_commands.json not found in $PROJECT_DIR. Please configure your build with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON and build first." | tee -a "$OUT_DIR/env.txt"
  exit 1
fi

# Run cppcheck if installed
if command -v cppcheck >/dev/null 2>&1; then
  echo "Running cppcheck with MISRA addon..."
  cppcheck --enable=all --inconclusive --project="$PROJECT_DIR/build/compile_commands.json" --addon=scripts/misra.json --xml 2> "$OUT_DIR/cppcheck-misra.xml" || true
  # Generate HTML report for MISRA
  if command -v cppcheck-htmlreport >/dev/null 2>&1; then
    cppcheck-htmlreport --file="$OUT_DIR/cppcheck-misra.xml" --report-dir="$OUT_DIR/cppcheck-misra-html" || true
  else
    echo "cppcheck-htmlreport not installed; skipping HTML report generation" > "$OUT_DIR/cppcheck-htmlreport-missing.txt"
  fi
else
  echo "cppcheck not installed; skipping cppcheck" > "$OUT_DIR/cppcheck-missing.txt"
fi

# Summarize results
echo "Summary of outputs written to $OUT_DIR"
ls -l "$OUT_DIR" || true

echo "Done"

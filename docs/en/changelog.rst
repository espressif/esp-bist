Changelog
=========

v1.1.0
------

New Features
^^^^^^^^^^^^

- **Enhanced SoC Support and Architecture Optimization**:

  - Improved standalone bare-metal support for ESP32-C5, ESP32-C61, ESP32-H2, ESP32-H4, and ESP32-P4.
  - Selected optimal target-specific RISC-V compiler architecture and ABI flags for each supported chip.
  - Enabled routine execution and program counter self-tests in Tightly Coupled Memory (TCM) on ESP32-P4.
  - Introduced hardware Floating-Point Unit (FPU) support, validating FPU registers and status flags during CPU self-tests.
  - Expanded Control and Status Register (CSR) verification to cover Physical Memory Attribute (PMA) address registers across targets, as well as chip-specific custom CSRs.
  - Refined memory maps and hardware profiles, including updated ROM sizing, external RAM support, high-frequency crystal options, and dedicated memory layouts.

- **Abraham RAM Testing**:
  Added the Abraham RAM testing algorithm for variable memory, delivering comprehensive pattern sensitivity, DC fault, and address decoder fault coverage.

- **Hardware Interrupt Testing**:
  Added hardware timer-based interrupt verification to detect missing or excessive interrupts and confirm interrupt controller routing.

- **Analog I/O (ADC) Self-Tests**:

  - Implemented low-level and high-level ADC self-tests with hardware curve-fitting calibration support.
  - Added internal reference voltage self-testing for ESP32-C3.

- **Automated Performance Metrics**:
  Integrated CPU cycle and instruction count benchmarking into the continuous integration pipeline, dynamically generating performance characterization tables in the documentation.

- **Model Context Protocol (MCP) Server**:
  Added a built-in MCP server providing documentation, API definitions, and configuration options to AI developer assistants.

Improvements and Bug Fixes
^^^^^^^^^^^^^^^^^^^^^^^^^^

- **Clock and Crystal Verification**:

  - Integrated hardware Crystal Watchdog Timer (XT WDT) handling on supported SoCs while allowing graceful fallbacks on chips without it.
  - Refactored main crystal calibration using RTC slow clock calibration and a dedicated frequency deviation calculator.

- **Watchdog and Windowed Supervision**:

  - Added a dedicated test suite and validation scenarios covering normal operation, underflow violations, and overflow reset on hardware and QEMU.
  - Added deinitialization and status query capabilities for windowed watchdog lifecycle management.
  - Refined watchdog test timing intervals and added multi-target reset reason decoding for core watchdog events across all supported targets.
  - Ensured hardware timers and allocated resources are cleanly released when windowed watchdog setup fails.

- **Stack Overflow Sentinel Protection**:

  - Extended stack protection sentinels to cover the entire guard region.
  - Enforced unoptimized compilation for recursive stack depth testing to prevent compiler optimizations from bypassing limits.

- **Safe Stack Execution for RAM Tests**:

  - Implemented dedicated safe stack buffer switching with 16-byte alignment during destructive March and Abraham RAM testing to protect the call stack from self-corruption.

- **Tooling and Build System**:

  - Upgraded baseline dependency to ESP-IDF v6.0 and the RISC-V GCC 15.2.0 toolchain.
  - Switched default BIST library compilation optimization from ``-O0`` to ``-Os`` for reduced binary footprint while preserving debuggability.
  - Added automated build and test orchestration scripts.
  - Integrated pre-commit formatting and linting hooks using ``astyle_py`` and ``Ruff``.
  - Introduced host-native unit testing using CTest for off-target validation of algorithmic helpers.

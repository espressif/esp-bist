Test Traceability Matrix
========================

Overview
--------

This document provides a comprehensive traceability overview linking IEC 60730 requirements to design, implementation, tests, and results. Traceability is essential for IEC 60730-1 Annex H compliance to demonstrate that all requirements are implemented, tested, and validated.

Traceability Structure
----------------------

The traceability matrix follows this structure:

1. **Requirement**: IEC 60730 Table H.1 component ID and description
2. **Design**: Module design documentation and implementation
3. **Test**: Test implementation (QEMU and/or hardware)
4. **Results**: Test execution results and evidence location

Detailed Test Traceability
---------------------------

CPU Register Test (IEC 60730 ID: 1.1)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: Verify integrity of CPU general-purpose registers (X1-X31); on FPU supported devices, also single-precision FPU registers (f0–f31)

**Design**:

- Module: ``src/bist/core/cpu/bist_cpu_regs.c``
- Function: ``bist_cpu_regs_test()``
- Design Doc: :doc:`module_design_and_coding` (CPU Register Test section)

**Test Implementation**:

- QEMU: ``tests/cpu_reg_test/pytest_qemu_cpu_reg_test.py``
  - Success test: All integer registers pass pattern test; on FPU supported devices, all FPU registers pass
  - Failure test: GDB fault injection per register (32 integer test cases; +32 FPU on FPU supported devices via ``test_freg_error``)
- Hardware: ``tests/cpu_reg_test/pytest_device_cpu_reg_test.py``
  - All integer registers validated on real hardware; FPU registers on FPU supported devices

**Test Results**:

- QEMU: ``tests/cpu_reg_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml``
- Hardware: ``tests/cpu_reg_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: 32 integer registers × 2 test cases (pass + fail) = 64 test cases; on FPU supported devices, +32 FPU registers × 2 = 128 total

CPU CSR Test (IEC 60730 ID: 1.1)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: Verify integrity of CPU Control and Status Registers; on FPU supported devices, includes FPU CSRs (``fflags``, ``frm``, ``fcsr``)

**Design**:

- Module: ``src/bist/core/cpu/bist_cpu_csr_regs.c``
- Function: ``bist_cpu_csr_regs_test()``
- Design Doc: :doc:`module_design_and_coding` (CPU CSR Register Test section)

**Test Implementation**:

- QEMU: ``tests/cpu_reg_test/pytest_qemu_cpu_reg_test.py``
  - Success test: All CSRs pass pattern test
  - Failure test: GDB fault injection per CSR; on FPU supported devices, +3 FPU CSRs via ``test_freg_csr_error`` (``fflags``, ``frm``, ``fcsr``)
- Hardware: ``tests/cpu_reg_test/pytest_device_cpu_reg_test.py``

**Test Results**:

- QEMU: ``tests/cpu_reg_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml``
- Hardware: ``tests/cpu_reg_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: CSR count varies by SoC — 25 on ESP32-C3 (5 trap + 16 pmpaddr + 4 pmpcfg), 37 on ESP32-C6/H2/C61 (adds 12 pma_addr), 39 on ESP32-C5 (adds 12 pma_addr + mexstatus + mhint), 40 on ESP32-H4/P4 (adds 12 pma_addr + ``fflags``, ``frm``, ``fcsr``). Each CSR has 2 test cases (pass + fail).

Program Counter Test (IEC 60730 ID: 1.3)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: Exercise PC bits, detect unexpected jumps or stuck PC

**Design**:

- Module: ``src/bist/core/cpu/bist_pc.c``
- Function: ``bist_pc_test()``
- Design Doc: :doc:`module_design_and_coding` (Program Counter Test section)

**Test Implementation**:

- QEMU: ``tests/pc_test/pytest_qemu_pc_test.py``
  - Success test: 4 functions in different memory regions
  - Failure test: Return address modification, function pointer corruption
- Hardware: ``tests/pc_test/pytest_device_pc_test.py``

**Test Results**:

- QEMU: ``tests/pc_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml``
- Hardware: ``tests/pc_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: 4 functions × 2 test cases (pass + fail) = 8 test cases total

Clock Test (IEC 60730 ID: 3)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: Validate main and reference clock sources, monitor drift

**Design**:

- Module: ``src/bist/core/clock/bist_clock.c``
- Functions: ``bist_clock_ext_crystal_test()``, ``bist_clock_main_crystal_test()``
- Design Doc: :doc:`module_design_and_coding` (Clock Test section)

**Test Implementation**:

- Hardware-only: ``tests/clock_test/pytest_device_*_crystal_test.py``
  - 32kHz crystal: XT WDT monitoring
  - 40MHz crystal: Frequency drift measurement

**Test Results**:

- Hardware: ``tests/clock_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: 2 clock sources tested on hardware

Flash CRC Test (IEC 60730 ID: 4.1)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: Validate flash image via CRC32

**Design**:

- Module: ``src/bist/core/memory/bist_flash.c``
- Function: ``bist_flash_test()``
- Design Doc: :doc:`module_design_and_coding` (Flash CRC Test section)

**Test Implementation**:

- QEMU: ``tests/flash_test/pytest_qemu_flash_test.py``
  - Success test: CRC validation for ``.flash.text`` and ``.flash.rodata``
  - Failure test: CRC length corruption via GDB
- Hardware: ``tests/flash_test/pytest_device_flash_test.py``

**Test Results**:

- QEMU: ``tests/flash_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml``
- Hardware: ``tests/flash_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: 2 sections × 2 test cases (pass + fail) = 4 test cases total

RAM Test (IEC 60730 ID: 4.2)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: RAM March tests, Abraham test (H.2.19.1), pattern checks

**Design**:

- Module: ``src/bist/core/memory/bist_ram.c``
- Functions: ``bist_ram_test_march_a()``, ``bist_ram_test_march_x()``,
  ``bist_ram_test_abraham()``, ``bist_ram_test_abraham_full()``
- Design Doc: :doc:`module_design_and_coding` (RAM Test section)

**Test Implementation**:

- QEMU: ``tests/ram_test/pytest_qemu_ram_test.py``
  - Success test: March A, March X, Abraham (single period), Abraham (full schedule)
  - Failure test: Memory corruption via GDB (March A, March X, Abraham)
- Hardware: ``tests/ram_test/pytest_device_ram_test.py``

**Test Results**:
- QEMU: ``tests/ram_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml``
- Hardware: ``tests/ram_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: 3 algorithms × 2 test cases (pass + fail) + 1 full-schedule pass = 7 test cases total

Stack Overflow Test (IEC 60730 ID: 4.2)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: Stack overflow detection

**Design**:

- Module: ``src/bist/core/cpu/bist_cpu_stack.c``
- Functions: ``bist_cpu_stack_overflow_init()``, ``bist_cpu_stack_overflow_check()``, ``bist_cpu_stack_overflow_test()``
- Design Doc: :doc:`module_design_and_coding` (Stack Overflow Test section)

**Test Implementation**:

- QEMU: ``tests/cpu_stack_test/pytest_qemu_cpu_stack_test.py``
  - Success test: Stack overflow detected via sentinel corruption
  - Failure test: Insufficient recursion depth via GDB
- Hardware: ``tests/cpu_stack_test/pytest_device_cpu_stack_test.py``

**Test Results**:

- QEMU: ``tests/cpu_stack_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml``
- Hardware: ``tests/cpu_stack_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: 2 test cases (pass + fail)

Watchdog Test (IEC 60730 ID: 6.3)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: Windowed watchdog operation, timing monitoring

**Design**:

- Module: ``src/bist/core/wdt/bist_wdt.c``
- Function: ``bist_wdt_test()``
- Design Doc: :doc:`module_design_and_coding` (Watchdog Test section)

**Test Implementation**:

- QEMU: ``tests/wdt_test/pytest_qemu_wdt_test.py``
  - Success test: Two-boot sequence, reset reason verification
  - Failure test: Timeout too long via GDB
- Hardware: ``tests/wdt_test/pytest_device_wdt_test.py``

**Test Results**:

- QEMU: ``tests/wdt_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml``
- Hardware: ``tests/wdt_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: 2 test cases (pass + fail)

Windowed Watchdog Test (IEC 60730 ID: 6.3)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: Windowed watchdog operation; underflow and normal feed window validation

**Design**:

- Module: ``src/bist/drivers/wdt.c`` (windowed WDT driver)
- Functions: ``wdt_init_windowed()``, ``wdt_feed()``, ``wdt_is_underflow_detected()``, ``wdt_windowed_deinit()``
- Design Doc: :doc:`module_design_and_coding` (Watchdog Operation Test / Windowed WDT section)

**Test Implementation**:

- QEMU: ``tests/windowed_wdt_test/pytest_qemu_windowed_wdt_test.py``
  - Normal operation within feed window
  - Underflow detection (feed before underflow timeout)
  - Consecutive feed cycles
- Hardware: ``tests/windowed_wdt_test/pytest_device_windowed_wdt_test.py``

**Test Results**:

- QEMU: ``tests/windowed_wdt_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml``
- Hardware: ``tests/windowed_wdt_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: 3 test cases (normal, underflow, consecutive)


GPIO Test (IEC 60730 ID: 7.1)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: GPIO configuration and plausibility checks

**Design**:

- Module: ``src/bist/core/io/bist_gpio.c``
- Function: ``bist_gpio_test()``
- Design Doc: :doc:`module_design_and_coding` (GPIO Test section)

**Test Implementation**:

- Hardware-only: ``tests/digital_io_test/pytest_device_digital_io_test.py``
  - Invalid GPIO test
  - GPIO output test
  - GPIO input test

**Test Results**:

- Hardware: ``tests/digital_io_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: 3 test cases

ADC Test (IEC 60730 ID: 7.2)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: ADC configuration and plausibility checks

**Design**:

- Module: ``src/bist/core/io/bist_adc.c``
- Functions: ``bist_adc_low_level_test()``, ``bist_adc_high_level_test()``, ``bist_adc_reference_test()`` (ESP32-C3 only)
- Design Doc: :doc:`module_design_and_coding` (ADC Plausibility Test section)

**Test Implementation**:

- Hardware-only: ``tests/analog_io_test/pytest_device_analog_io_test.py``
  - Invalid ADC unit/channel test
  - ADC low level test (internal pull-down)
  - ADC high level test (internal pull-up)
  - ADC reference test (ESP32-C3 only, internal VREF)

**Test Results**:

- Hardware: ``tests/analog_io_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: 3 test cases (4 on ESP32-C3)

Traceability Summary
--------------------

- **Total IEC 60730 Components**: 8 (1.1, 1.3, 3, 4.1, 4.2, 6.3, 7.1, 7.2)
- **Total Test Modules**: 11
- **Total Test Cases**: 100+
- **Test Environments**: QEMU (emulation) + Hardware (real-world)
- **Coverage**: 100% of safety-relevant functions tested

This traceability matrix demonstrates complete coverage from requirements through design, implementation, testing, and validation, supporting IEC 60730-1 Annex H compliance.

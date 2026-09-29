Test Traceability Matrix
========================

Overview
--------

This document provides a traceability overview linking IEC 60730 Table H.1 requirements to module design, test implementations, test results, and coverage. Traceability demonstrates that all safety requirements are implemented and validated.

Traceability Structure
----------------------

Each component entry records:

1. **Requirement**: IEC 60730 Table H.1 component ID and safety objective
2. **Design**: Implementation source files, entry functions, and design reference
3. **Test Implementation**: Test suites across QEMU emulation, target hardware, and host environments
4. **Test Results**: Location of machine-readable test execution reports (JUnit XML)
5. **Coverage**: Test case counts and fault coverage summary

Detailed Test Traceability
---------------------------

CPU Register Test (IEC 60730 ID: 1.1)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: Verify integrity of CPU general-purpose registers (X1–X31) and single-precision FPU registers (f0–f31 on FPU targets).

**Design**: ``src/bist/core/cpu/bist_cpu_regs.c`` (``bist_cpu_regs_test()``; :ref:`cpu-register-test`)

**Test Implementation**:

- QEMU: ``tests/cpu_reg_test/pytest_qemu_cpu_reg_test.py`` (pattern pass test and GDB fault injection per register)
- Hardware: ``tests/cpu_reg_test/pytest_device_cpu_reg_test.py`` (hardware execution on target DUT)

**Test Results**:

- QEMU: ``tests/cpu_reg_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml``
- Hardware: ``tests/cpu_reg_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: 32 integer registers (64 test cases); +32 FPU registers (+64 test cases) on FPU targets.

CPU CSR Test (IEC 60730 ID: 1.1)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: Verify integrity of CPU Control and Status Registers, including FPU CSRs (``fflags``, ``frm``, ``fcsr``) on FPU targets.

**Design**: ``src/bist/core/cpu/bist_cpu_csr_regs.c`` (``bist_cpu_csr_regs_test()``; :doc:`module_design_and_coding`)

**Test Implementation**:

- QEMU: ``tests/cpu_reg_test/pytest_qemu_cpu_reg_test.py`` (CSR pattern pass and GDB fault injection per CSR)
- Hardware: ``tests/cpu_reg_test/pytest_device_cpu_reg_test.py`` (hardware execution on target DUT)

**Test Results**:

- QEMU: ``tests/cpu_reg_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml``
- Hardware: ``tests/cpu_reg_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: 25 to 40 CSRs tested per SoC (2 test cases each: pass and GDB fault injection).

Program Counter Test (IEC 60730 ID: 1.3)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: Exercise PC bits across execution regions and detect unexpected jumps or stuck PC.

**Design**: ``src/bist/core/cpu/bist_pc.c`` (``bist_pc_test()``; :ref:`program-counter-test`)

**Test Implementation**:

- QEMU: ``tests/pc_test/pytest_qemu_pc_test.py`` (multi-region pass, GDB bit-flip fault injection, and zero-pointer Watchdog reset)
- Hardware: ``tests/pc_test/pytest_device_pc_test.py`` (multi-region execution on target DUT)

**Test Results**:

- QEMU: ``tests/pc_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml``
- Hardware: ``tests/pc_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: 3 QEMU test cases + 1 hardware test case; exercises 24 of 30 addressable PC bits (25 on ESP32-P4) across up to 5 execution regions.

Interrupt Test (IEC 60730 ID: 2)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: Validate interrupt handling, matrix routing for software IRQ sources, and concurrent hardware interrupt delivery.

**Design**: ``src/bist/core/interrupt/bist_interrupt_sw.c``, ``src/bist/core/interrupt/bist_interrupt.c`` (``bist_interrupt_source_map_test()``, ``bist_hardware_interrupt_test()``; :ref:`interrupt-test`)

**Test Implementation**:

- QEMU: ``tests/interrupt_test/pytest_qemu_interrupt_test.py`` (software source map pass and GDB count corruption)
- Hardware: ``tests/interrupt_test/pytest_device_interrupt_test.py`` (software source map and dual-timer 2:1 period ratio validation)

**Test Results**:

- QEMU: ``tests/interrupt_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml``
- Hardware: ``tests/interrupt_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: Software interrupt matrix routing covered in QEMU and hardware; dual-timer delivery validated on hardware.

Clock Test (IEC 60730 ID: 3)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: Validate main and reference clock sources and monitor frequency drift.

**Design**: ``src/bist/core/clock/bist_clock_fail.c``, ``src/bist/core/clock/include/bist_clock_math.h`` (``bist_ext_crystal_fail_test()``, ``bist_main_crystal_test()``, ``xtal_deviation_percent()``; :ref:`clock-test`)

**Test Implementation**:

- Hardware: ``tests/clock_test/`` (``main.c``; 32 kHz XT WDT and main crystal drift against ``CONFIG_XTAL_FREQ``)
- Host: ``tests/unit/clock_math/test_bist_clock_math.c`` (deviation arithmetic validated under ``ctest``)

**Test Results**:

- Hardware: ``tests/clock_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``
- Host: ``ctest --test-dir build/clock_math_test --output-on-failure``

**Coverage**: 2 clock sources validated on hardware + deviation math verified on host.

Flash CRC Test (IEC 60730 ID: 4.1)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: Validate invariable memory integrity via CRC32.

**Design**: ``src/bist/core/memory/bist_flash.c`` (``bist_flash_test()``; :ref:`flash-test`)

**Test Implementation**:

- QEMU: ``tests/flash_test/pytest_qemu_flash_test.py`` (text and rodata CRC pass + GDB length corruption)
- Hardware: ``tests/flash_test/pytest_device_flash_test.py`` (CRC validation on target DUT)

**Test Results**:

- QEMU: ``tests/flash_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml``
- Hardware: ``tests/flash_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: 2 flash sections tested for integrity; 2 GDB fault-injection tests in QEMU; verified on hardware.

RAM Test (IEC 60730 ID: 4.2)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: Variable memory testing using March A, March X, and Abraham (H.2.19.1) algorithms.

**Design**: ``src/bist/core/memory/bist_ram.c`` (``bist_ram_test_march_a()``, ``bist_ram_test_march_x()``, ``bist_ram_test_abraham()``, ``bist_ram_test_abraham_full()``; :ref:`ram-test`)

**Test Implementation**:

- QEMU: ``tests/ram_test/pytest_qemu_ram_test.py`` (March A, March X, and Abraham pass + GDB memory corruption)
- Hardware: ``tests/ram_test/pytest_device_ram_test.py`` (algorithm execution on target DUT)

**Test Results**:

- QEMU: ``tests/ram_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml``
- Hardware: ``tests/ram_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: 3 algorithms × 2 test cases (pass + fail) + 1 full-schedule pass = 7 test cases total.

Stack Overflow Test (IEC 60730 ID: 4.2)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: Detect stack overflow via pattern sentinel and watermark monitoring.

**Design**: ``src/bist/core/cpu/bist_cpu_stack.c`` (``bist_cpu_stack_overflow_init()``, ``bist_cpu_stack_overflow_check()``, ``bist_cpu_stack_overflow_test()``; :doc:`module_design_and_coding`)

**Test Implementation**:

- QEMU: ``tests/cpu_stack_test/pytest_qemu_cpu_stack_test.py`` (bounds verification, overflow detection, and GDB recursion limit)
- Hardware: ``tests/cpu_stack_test/pytest_device_cpu_stack_test.py`` (stack overflow detection on target DUT)

**Test Results**:

- QEMU: ``tests/cpu_stack_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml``
- Hardware: ``tests/cpu_stack_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: 3 test cases (bounds verification, overflow pass, and shallow recursion fail).

Watchdog Test (IEC 60730 ID: 6.3)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: Hardware Main Watchdog Timer (MWDT) timeout detection and reset reason verification.

**Design**: ``src/bist/core/wdt/bist_wdt.c`` (``bist_wdt_test()``; :doc:`module_design_and_coding`)

**Test Implementation**:

- QEMU: ``tests/wdt_test/pytest_qemu_wdt_test.py`` (two-boot reset verification and GDB timeout failure injection)
- Hardware: ``tests/wdt_test/pytest_device_wdt_test.py`` (two-boot reset check and sub-tick timeout rejection)

**Test Results**:

- QEMU: ``tests/wdt_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml``
- Hardware: ``tests/wdt_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: 2 QEMU test cases (pass + fail) + 2 hardware test cases (two-boot reset + sub-tick rejection).

Windowed Watchdog Test & esp_timer Validation (IEC 60730 ID: 6.3)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: Windowed watchdog timing monitoring (underflow and normal feed windows) and software timer (esp_timer) driver operation.

**Design**: ``src/bist/drivers/wdt.c``, ``src/bist/drivers/esp_timer.c`` (``wdt_init_windowed()``, ``wdt_feed()``, ``wdt_is_underflow_detected()``, ``wdt_windowed_deinit()``; :ref:`windowed-watchdog-operation`)

**Test Implementation**:

- QEMU: ``tests/windowed_wdt_test/pytest_qemu_windowed_wdt_test.py`` (normal feed, underflow detection, consecutive cycles)
- Hardware: ``tests/windowed_wdt_test/pytest_device_windowed_wdt_test.py`` (normal feed, underflow, consecutive cycles)

**Test Results**:

- QEMU: ``tests/windowed_wdt_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml``
- Hardware: ``tests/windowed_wdt_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: 3 test cases validating windowed WDT logic and in-situ ``esp_timer`` driver functionality.

GPIO Test (IEC 60730 ID: 7.1)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: GPIO configuration and plausibility checks (output drive and input read).

**Design**: ``src/bist/core/io/bist_gpio.c``, ``src/bist/drivers/gpio.c`` (``bist_gpio_output_test()``, ``bist_gpio_input_test()``; :ref:`gpio-plausibility-test`)

**Test Implementation**:

- Hardware-only: ``tests/digital_io_test/pytest_device_digital_io_test.py`` (invalid pin rejection, GPIO2 output drive, configured input level)

**Test Results**:

- Hardware: ``tests/digital_io_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: 3 test cases on target hardware.

ADC Test (IEC 60730 ID: 7.2)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Requirement**: ADC configuration and plausibility checks (internal pull-down, pull-up, and VREF biasing).

**Design**: ``src/bist/core/io/bist_adc.c``, ``src/bist/drivers/adc_oneshot.c`` (``bist_adc_low_level_test()``, ``bist_adc_high_level_test()``, ``bist_adc_reference_test()``; :ref:`adc-plausibility-test`)

**Test Implementation**:

- Hardware-only: ``tests/analog_io_test/pytest_device_analog_io_test.py`` (invalid parameter rejection, pull-down, pull-up, and VREF on ESP32-C3; skipped on ESP32-H4)

**Test Results**:

- Hardware: ``tests/analog_io_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml``

**Coverage**: 3 test cases (4 on ESP32-C3); skipped on ESP32-H4.

Traceability Summary
--------------------

- **Total IEC 60730 Components**: 9 (1.1, 1.3, 2, 3, 4.1, 4.2, 6.3, 7.1, 7.2)
- **Total Test Applications**: 10 standalone target test suites + 1 host unit test suite
- **Test Environments**: QEMU (deterministic emulation & GDB fault injection), Hardware (real-world DUT), Host (algorithmic verification)
- **Coverage & Evidence**: Complete requirement-to-test mapping and JUnit XML report locations are tabulated in :doc:`coverage_analysis`.

This traceability matrix demonstrates complete coverage from requirements through design, implementation, testing, and validation, supporting IEC 60730-1 Annex H compliance.

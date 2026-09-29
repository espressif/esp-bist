Coverage Analysis
=================

Overview
--------

This document provides coverage analysis for the ESP-BIST library, including code coverage metrics and requirements coverage analysis. Coverage analysis is essential for IEC 60730-1 Annex H compliance to demonstrate that all safety-relevant code and requirements are adequately tested.

Code Coverage
-------------

Code Coverage Methodology
^^^^^^^^^^^^^^^^^^^^^^^^^

Code coverage is measured through:

1. **Functional Testing**: All BIST test functions are executed in both QEMU and hardware test environments
2. **Fault Injection**: Negative test cases use GDB fault injection to exercise error paths
3. **Static Analysis**: Code paths are analyzed statically to identify unreachable code

Module Coverage
^^^^^^^^^^^^^^^

The following table provides code coverage information for each BIST module:

.. list-table::
   :header-rows: 1
   :widths: 20 12 15 18 15 20

   * - Module
     - Functions
     - Tested Functions
     - Test Cases (Pass)
     - Test Cases (Fail)
     - Coverage Evidence

   * - CPU Registers
     - 1
     - 1 (100%)
     - 1 (32 GPRs; +32 FPU on H4/P4)
     - 32 (fault injection; +32 FPU)
     - :doc:`software_validation` (CPU Register Test)

   * - CPU CSRs
     - 1
     - 1 (100%)
     - 1 (25–40 CSRs depending on SoC)
     - 25–40 (fault injection)
     - :doc:`software_validation` (CPU CSR Test)

   * - Program Counter
     - 1
     - 1 (100%)
     - 1 (4 regions; 5 on ESP32-P4)
     - 2 (fault injection)
     - :doc:`software_validation` (Program Counter Test)

   * - Interrupt Test
     - 2
     - 2 (100%)
     - 2 (SW source map, HW dual-timer)
     - 1 (GDB fault injection)
     - :doc:`software_validation` (Interrupt Handling and Execution Test)

   * - Stack Overflow
     - 3
     - 3 (100%)
     - 2 (bounds check, overflow detection)
     - 1 (insufficient recursion)
     - :doc:`software_validation` (Stack Overflow Test)

   * - RAM Test
     - 4
     - 4 (100%)
     - 4 (March A, March X, Abraham, Abraham Full)
     - 3 (memory corruption)
     - :doc:`software_validation` (RAM Test)

   * - Flash CRC
     - 1
     - 1 (100%)
     - 1 (2 sections: text and rodata)
     - 2 (data and text CRC corruption)
     - :doc:`software_validation` (Flash Test)

   * - Clock Test
     - 3
     - 3 (100%)
     - 2 (32 kHz XT WDT, main crystal) + 1 host (math)
     - 1 (crystal failure)
     - :doc:`software_validation` (Clock Test, Host Unit Tests)

   * - Watchdog (MWDT)
     - 1
     - 1 (100%)
     - 2 (reset reason, sub-tick rejection)
     - 1 (timeout too long)
     - :doc:`software_validation` (Watchdog Test)

   * - Windowed WDT & Timer
     - 4
     - 4 (100%)
     - 3 (normal, underflow, consecutive)
     - 1 (premature feed underflow)
     - :doc:`software_validation` (Windowed Watchdog Test)

   * - GPIO Plausibility
     - 2
     - 2 (100%)
     - 3 (invalid pin, output drive, input level)
     - Hardware-only
     - :doc:`software_validation` (GPIO Test)

   * - ADC Plausibility
     - 3
     - 3 (100%)
     - 3 (invalid ADC, pull-down, pull-up; +1 VREF C3)
     - Hardware-only (skipped on H4)
     - :doc:`software_validation` (ADC Test)

Coverage Metrics
^^^^^^^^^^^^^^^^

- **Function Coverage**: 100% of public BIST API functions are tested
- **Statement Coverage**: All safety-relevant code paths are exercised through positive and negative test cases
- **Branch Coverage**: Error paths are tested through fault injection (GDB) and hardware fault simulation
- **Condition Coverage**: All conditional branches in test logic are exercised

Requirements Coverage
---------------------

Requirements Coverage Methodology
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Requirements coverage is measured by mapping each IEC 60730 Table H.1 component requirement to:

1. **Design Implementation**: Module design documented in :doc:`module_design_and_coding`
2. **Test Implementation**: Validation tests documented in :doc:`software_validation`
3. **Test Results**: Test execution results (JUnit XML reports)

Requirements Coverage Matrix
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 10 25 20 20 25

   * - IEC 60730 ID
     - Requirement
     - Design Implementation
     - Test Implementation
     - Test Results

   * - 1.1
     - CPU Register Integrity
     - ``bist_cpu_regs_test()``
     - QEMU + Hardware (32 GPRs; +32 FPU on H4/P4; fault injection)
     - ``tests/cpu_reg_test/build/tests/*_report.xml``

   * - 1.1
     - CPU CSR Integrity
     - ``bist_cpu_csr_regs_test()``
     - QEMU + Hardware (25–40 CSRs depending on SoC: 25 C3, 37 C6/H2/C61, 39 C5, 40 H4/P4; fault injection)
     - ``tests/cpu_reg_test/build/tests/*_report.xml``

   * - 1.3
     - Program Counter Integrity
     - ``bist_pc_test()``
     - QEMU + Hardware (4 regions; 5 on ESP32-P4 with TCM; fault injection)
     - ``tests/pc_test/build/tests/*_report.xml``

   * - 2
     - Interrupt Source Map
     - ``bist_interrupt_source_map_test()``
     - QEMU + Hardware (``CPU_INTR_FROM_CPU_0..3``; fault injection)
     - ``tests/interrupt_test/build/tests/*_report.xml``

   * - 2
     - Hardware Interrupt Delivery
     - ``bist_hardware_interrupt_test()``
     - Hardware (dual-timer TIMG 2:1 period ratio; QEMU TIMG unsupported)
     - ``tests/interrupt_test/build/tests/*_report.xml``

   * - 3
     - Clock Source Validation
     - ``bist_ext_crystal_fail_test()``, ``bist_main_crystal_test()``, ``xtal_deviation_percent()``
     - Hardware-only (32 kHz XT WDT, main-crystal drift) + Host unit test (deviation math)
     - ``tests/clock_test/build/tests/*_report.xml``; ``ctest --test-dir build/clock_math_test``

   * - 4.1
     - Flash Memory Integrity
     - ``bist_flash_test()``
     - QEMU + Hardware (CRC validation, fault injection)
     - ``tests/flash_test/build/tests/*_report.xml``

   * - 4.2
     - RAM Integrity
     - ``bist_ram_test_march_a()``, ``bist_ram_test_march_x()``, ``bist_ram_test_abraham()``, ``bist_ram_test_abraham_full()``
     - QEMU + Hardware (March + Abraham algorithms, safe stack, fault injection)
     - ``tests/ram_test/build/tests/*_report.xml``

   * - 4.2
     - Stack Overflow Detection
     - ``bist_cpu_stack_overflow_init()``, ``bist_cpu_stack_overflow_check()``, ``bist_cpu_stack_overflow_test()``
     - QEMU + Hardware (sentinel check, overflow detection, recursion fault injection)
     - ``tests/cpu_stack_test/build/tests/*_report.xml``

   * - 6.3
     - Timing Monitoring (Watchdog)
     - ``bist_wdt_test()``
     - QEMU + Hardware (two-boot reset verification, sub-tick rejection, fault injection)
     - ``tests/wdt_test/build/tests/*_report.xml``

   * - 6.3
     - Windowed Watchdog & Timer
     - ``wdt_init_windowed()``, ``wdt_feed()``, ``wdt_is_underflow_detected()``, ``wdt_windowed_deinit()``
     - QEMU + Hardware (underflow detection, normal window feed, consecutive cycles)
     - ``tests/windowed_wdt_test/build/tests/*_report.xml``

   * - 7.1
     - Digital I/O Plausibility
     - ``bist_gpio_output_test()``, ``bist_gpio_input_test()``
     - Hardware-only (invalid pin rejection, GPIO2 output drive, configured input read)
     - ``tests/digital_io_test/build/tests/*_report.xml``

   * - 7.2
     - Analog I/O Plausibility
     - ``bist_adc_low_level_test()``, ``bist_adc_high_level_test()``, ``bist_adc_reference_test()``
     - Hardware-only (invalid ADC parameter rejection, pull-down, pull-up, VREF on ESP32-C3; skipped on ESP32-H4)
     - ``tests/analog_io_test/build/tests/*_report.xml``

Coverage Gaps and Justification
--------------------------------

Known Coverage Limitations
^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. **Clock Tests (Hardware-Only & Board Variations)**: QEMU cannot accurately model crystal oscillators or XT WDT hardware; physical target hardware validation is used. On boards without an external 32 kHz crystal, clock deviation arithmetic is regression-tested on host via ``tests/unit/clock_math/``. On SoCs lacking ``SOC_XT_WDT_SUPPORTED``, the 32 kHz crystal watchdog test is skipped.

2. **GPIO Tests (Hardware-Only)**: GPIO pad drivers and input buffers require physical silicon routing; validated on device via ``tests/digital_io_test/``.

3. **ADC Tests (Hardware-Only & SoC Omission)**: ADC conversion and internal biasing (pull-down, pull-up, VREF) require physical analog silicon; validated on device via ``tests/analog_io_test/``. On ESP32-H4, the ADC peripheral is not present in hardware and the ADC test suite is skipped.

4. **Hardware Interrupt Path (TIMG)**: QEMU does not support Timer Group 0/1 hardware alarms used by ``bist_hardware_interrupt_test()``. The software interrupt source map path is validated in QEMU (including fault injection); the dual-TIMG hardware path is validated on physical devices.

Justification
^^^^^^^^^^^^^

All coverage limitations are justified:

- **Hardware-Only Tests**: Physical hardware is required for accurate validation of clock, GPIO, ADC, and TIMG hardware-interrupt functionality. QEMU limitations are documented and hardware testing provides adequate coverage.

- **Test Environment**: Both QEMU (deterministic, repeatable) and hardware (real-world conditions) testing provide complementary coverage.

- **Fault Injection**: GDB-based fault injection in QEMU enables systematic testing of error paths that are difficult to reproduce on hardware.

Coverage Summary
----------------

- **Code Coverage**: 100% of safety-relevant functions are tested
- **Requirements Coverage**: 100% of IEC 60730 Table H.1 components are implemented and tested
- **Test Environments**: Both QEMU (emulation) and hardware (real-world) validation plus host unit testing
- **Fault Coverage**: Error paths tested through systematic fault injection

This coverage analysis demonstrates that all safety-relevant code and requirements are adequately tested, supporting IEC 60730-1 Annex H compliance.

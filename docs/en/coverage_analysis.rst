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
   :widths: 20 15 15 15 15 20

   * - Module
     - Functions
     - Tested Functions
     - Test Cases (Pass)
     - Test Cases (Fail)
     - Coverage Evidence

   * - CPU Registers
     - 1
     - 1 (100%)
     - 1 (32 registers)
     - 32 (fault injection)
     - :doc:`software_validation` (CPU Register Test)

   * - CPU CSRs
     - 1
     - 1 (100%)
     - 1 (25–39 CSRs depending on SoC: 25 C3, 37 C6/H2, 39 C5)
     - 25–39 (fault injection)
     - :doc:`software_validation` (CPU CSR Test)

   * - Program Counter
     - 1
     - 1 (100%)
     - 1 (4 functions)
     - 2 (fault injection)
     - :doc:`software_validation` (Program Counter Test)

   * - Interrupt Test
     - 2
     - 2 (100%)
     - 2 (SW source map, HW dual TIMG)
     - Hardware / QEMU SW path
     - :doc:`software_validation` (Interrupt Handling and Execution Test)

   * - Stack Overflow
     - 3
     - 3 (100%)
     - 1 (overflow detection)
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
     - 1 (2 sections)
     - 1 (CRC length corruption)
     - :doc:`software_validation` (Flash Test)

   * - Clock Test
     - 2
     - 2 (100%)
     - 2 (32kHz, 40MHz)
     - Hardware-only (crystal removal)
     - :doc:`software_validation` (Clock Test)

   * - Watchdog
     - 3
     - 3 (100%)
     - 1 (WDT reset)
     - 1 (timeout too long)
     - :doc:`software_validation` (Watchdog Test)

   * - GPIO Test
     - 1
     - 1 (100%)
     - 3 (invalid, output, input)
     - Hardware-only
     - :doc:`software_validation` (GPIO Test)

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
     - QEMU + Hardware (32 registers, fault injection)
     - ``tests/cpu_reg_test/build/tests/*_report.xml``

   * - 1.1
     - CPU CSR Integrity
     - ``bist_cpu_csr_regs_test()``
     - QEMU + Hardware (25–39 CSRs depending on SoC: 25 C3, 37 C6/H2, 39 C5; fault injection)
     - ``tests/cpu_reg_test/build/tests/*_report.xml``

   * - 1.3
     - Program Counter Integrity
     - ``bist_pc_test()``
     - QEMU + Hardware (4 regions, fault injection)
     - ``tests/pc_test/build/tests/*_report.xml``

   * - 2
     - Interrupt Source Map
     - ``bist_interrupt_source_map_test()``
     - QEMU + Hardware (``CPU_INTR_FROM_CPU_0..3``)
     - ``tests/interrupt_test/build/tests/*_report.xml``

   * - 2
     - Hardware Interrupt Delivery
     - ``bist_hardware_interrupt_test()``
     - Hardware (dual TIMG 2:1 period ratio; QEMU TIMG unsupported)
     - ``tests/interrupt_test/build/tests/*_report.xml``

   * - 3
     - Clock Source Validation
     - ``bist_clock_test()``
     - Hardware-only (32kHz XT WDT, 40MHz drift)
     - ``tests/clock_test/build/tests/*_report.xml``

   * - 4.1
     - Flash Memory Integrity
     - ``bist_flash_test()``
     - QEMU + Hardware (CRC validation, fault injection)
     - ``tests/flash_test/build/tests/*_report.xml``

   * - 4.2
     - RAM Integrity
     - ``bist_ram_test_march_a()``, ``bist_ram_test_march_x()``, ``bist_ram_test_abraham()``, ``bist_ram_test_abraham_full()``
     - QEMU + Hardware (March + Abraham algorithms, fault injection)
     - ``tests/ram_test/build/tests/*_report.xml``

   * - 4.2
     - Stack Overflow Detection
     - ``bist_cpu_stack_overflow_check()``
     - QEMU + Hardware (sentinel check, stress test)
     - ``tests/cpu_stack_test/build/tests/*_report.xml``

   * - 6.3
     - Timing Monitoring (Watchdog)
     - ``bist_wdt_test()``, windowed WDT
     - QEMU + Hardware (timeout, reset reason)
     - ``tests/wdt_test/build/tests/*_report.xml``

   * - 7.1
     - Digital I/O Plausibility
     - ``bist_gpio_test()``
     - Hardware-only (GPIO config, I/O levels)
     - ``tests/digital_io_test/build/tests/*_report.xml``

   * - 7.2
     - Analog I/O Plausibility
     - ``bist_adc_test()``
     - Hardware-only (ADC config, pull-up/pull-down/VREF readings)
     - ``tests/analog_io_test/build/tests/*_report.xml``

Coverage Gaps and Justification
--------------------------------

Known Coverage Limitations
^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. **Clock Tests (Hardware-Only)**: QEMU cannot accurately model crystal oscillators. Hardware testing is required and sufficient for validation.

2. **GPIO Tests (Hardware-Only)**: GPIO functionality requires physical hardware. QEMU simulation is not applicable.

3. **Watchdog Two-Boot Sequence**: Watchdog reset verification requires two consecutive boots. This is tested on both QEMU and hardware.

4. **ADC Tests (Hardware-Only)**: ADC functionality requires physical hardware and internal pull resistors or VREF. QEMU simulation is not applicable.

5. **Hardware Interrupt Path (TIMG)**: QEMU does not support Timer Group 0/1 alarms used by ``bist_hardware_interrupt_test()``. The software interrupt source map path is validated in QEMU; the dual-TIMG hardware path is validated on device.

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
- **Test Environments**: Both QEMU (emulation) and hardware (real-world) validation
- **Fault Coverage**: Error paths tested through systematic fault injection

This coverage analysis demonstrates that all safety-relevant code and requirements are adequately tested, supporting IEC 60730-1 Annex H compliance.

Risk Assessment
===============

Overview
--------

This chapter provides a systematic risk assessment for the ESP-BIST library, mapping potential hazards to safety functions and documenting the risk mitigation measures implemented. This assessment supports IEC 60730-1 Annex H compliance by demonstrating that all identified hazards are addressed through appropriate safety mechanisms.

Risk Assessment Methodology
----------------------------

The risk assessment follows a systematic approach:

1. **Hazard Identification**: Identification of potential hazards that could arise from hardware or software faults
2. **Risk Classification**: Classification of risks based on severity and probability
3. **Safety Function Mapping**: Mapping of hazards to IEC 60730 component IDs and safety functions
4. **Mitigation Verification**: Documentation of how each safety function mitigates the identified risk

Hazard Analysis and Risk Mitigation
------------------------------------

The following table maps identified hazards to safety functions, risk levels, and mitigation measures:

.. list-table::
   :header-rows: 1
   :widths: 10 25 10 20 30

   * - Hazard ID
     - Hazard Description
     - Risk Level
     - IEC 60730 Component
     - Mitigation Measure

   * - H1
     - CPU register corruption leading to incorrect computation or control flow
     - High
     - 1.1 (CPU Registers)
     - ``bist_cpu_regs_test()`` verifies all general-purpose registers can store and retrieve 0/1 patterns, detecting stuck-at faults and data path errors; on FPU supported devices, also tests all 32 single-precision FPU registers

   * - H2
     - CPU CSR corruption causing incorrect exception handling or memory protection failures
     - High
     - 1.1 (CPU CSRs)
     - ``bist_cpu_csr_regs_test()`` validates critical CSRs (MTVEC, MEPC, MCAUSE, PMPADDR, PMPCFG, and on PMA-capable SoCs: PMA address, MEXSTATUS, MHINT; on FPU supported devices: ``fflags``, ``frm``, ``fcsr``) maintain integrity

   * - H3
     - Program counter corruption causing execution outside intended code regions
     - High
     - 1.3 (Program Counter)
     - ``bist_pc_test()`` exercises PC bits by calling functions placed in IRAM, Flash, and RTC regions, detecting stuck-at faults or unexpected jumps

   * - H4
     - Clock oscillator failure causing timing violations and missed deadlines
     - High
     - 3 (Clock)
     - ``bist_clock_test()`` monitors 32kHz crystal via XT WDT and validates 40MHz main crystal frequency within ±1% tolerance

   * - H5
     - Flash memory corruption leading to execution of incorrect code or use of corrupted data
     - High
     - 4.1 (Invariable Memory)
     - ``bist_flash_test()`` computes CRC32 over ``.flash.text`` and ``.flash.rodata`` sections and compares against post-build injected checksums

   * - H6
     - RAM corruption causing data integrity loss or stack overflow
     - High
     - 4.2 (Variable Memory)
     - ``bist_ram_test_march_a()`` and ``bist_ram_test_march_x()`` detect coupling and transition faults; ``bist_cpu_stack_overflow_check()`` monitors stack sentinel

   * - H7
     - Infinite loop or deadlock preventing watchdog feeding and system recovery
     - High
     - 6.3 (Timing)
     - Windowed watchdog (MWDT) with configurable timeout enforces periodic ``wdt_feed()`` calls; early feeds detect underflow; timeout triggers system reset

   * - H8
     - GPIO misconfiguration or stuck I/O lines causing incorrect system behavior
     - Medium
     - 7.1 (Digital I/O)
     - ``bist_gpio_test()`` verifies GPIO configuration and validates output/input levels are readable and controllable

Risk Level Definitions
----------------------

- **High**: Hazards that could lead to system failure, unsafe operation, or inability to detect faults
- **Medium**: Hazards that could degrade system performance or reduce fault detection capability
- **Low**: Hazards with minimal impact on safety or functionality

Residual Risk Assessment
------------------------

All identified hazards are mitigated through the safety functions documented in :doc:`module_design_and_coding`. The residual risk for each hazard is considered acceptable for IEC 60730 Class B applications because:

1. **Multiple Detection Mechanisms**: Critical components (CPU, memory, clock) are tested both at startup and during runtime
2. **Deterministic Execution**: All BIST code runs from IRAM, eliminating cache-related timing variability
3. **Fail-Safe Design**: Test failures trigger explicit error codes, allowing application to transition to safe state
4. **Watchdog Escalation**: If application cannot recover, watchdog reset ensures system returns to known safe state
5. **Comprehensive Coverage**: Tests cover all IEC 60730 Table H.1 components relevant to embedded systems

Traceability
------------

Each hazard maps directly to:

- **IEC 60730 Component ID**: See :doc:`software_safety_requirements` for component mapping
- **Architecture Design**: System architecture documented in :doc:`software_architecture`
- **Safety Function**: Documented in :doc:`module_design_and_coding`
- **Validation Evidence**: Test results documented in :doc:`software_validation`

This traceability ensures that all identified risks are addressed through verified safety mechanisms, supporting the safety case for IEC 60730 compliance.

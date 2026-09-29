ESP-BIST Documentation
======================

The BIST library is a cmake library with a set of routines designed to verify the integrity and proper operation of the hardware components in Espressif's SoCs. The library includes tests for the CPU registers, configuration and status registers (CSRs), interrupt handling and execution, volatile memory, non-volatile memory, cpu stack, program counter (PC), clock sources and safety related peripherals.

This documentation provides a comprehensive overview of ESP-BIST, including its software safety requirements, risk assessment, architecture, module design and coding, validation methods, test traceability, coverage analysis, tool qualification, safety case summary, application guidance, and API reference. It also details the project's conformity with IEC 60335-1 Annex R and IEC 60730-1 Annex H (class B) standards.

.. list-table:: Revision History
   :header-rows: 1
   :widths: 15 15 70

   * - Version
     - Date
     - Description
   * - `v1.0.0 <https://documentation.espressif.com/IEC%2060730%20Class%20B%20&%20IEC60335-1%20Class%20R1%20Safety%20Certification.pdf>`_
     - Jan 23, 2026
     - Initial release
   * - v1.1.0
     - TBD
     - Enhanced SoC support across all targets, expanded self-test coverage (Abraham RAM, ADC, hardware interrupts). See :doc:`changelog`.

.. only:: html

    To switch to a different SoC target, choose target from the dropdown in the upper left.

.. toctree::
    :maxdepth: 1

    Get Started <get_started>
    Changelog <changelog>
    Software Safety Requirements <software_safety_requirements>
    Risk Assessment <risk_assessment>
    Software Architecture <software_architecture>
    Module Design and Coding <module_design_and_coding>
    Software Validation <software_validation>
    Test Traceability Matrix <test_traceability_matrix>
    Coverage Analysis <coverage_analysis>
    Tool Qualification <tool_qualification>
    Safety Case Summary <safety_case_summary>
    Application Guide <application_guide>
    API Reference <api>

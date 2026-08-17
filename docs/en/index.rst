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
   * - v1.0.0
     - Jan 23, 2026
     - Initial release

.. only:: html

    To switch to a different SoC target, choose target from the dropdown in the upper left.

.. toctree::
    :maxdepth: 1

    Get Started <get_started>
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
    Host Diagnostics <host_diagnostics>
    API Reference <api>
    MCP Server <mcp_server>

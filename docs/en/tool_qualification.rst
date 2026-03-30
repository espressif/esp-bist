Tool Qualification
==================

Overview
--------

This document provides tool qualification evidence for development tools used in the ESP-BIST project. Tool qualification is required for IEC 60730-1 Annex H compliance to ensure that tools used in the development of safety-relevant software do not introduce errors that could compromise system safety.

Tool Classification
-------------------

According to IEC 61508-3 (which provides guidance for IEC 60730), tools are classified based on their potential impact on safety:

- **T1**: Tools that cannot introduce errors into the executable code (e.g., editors, version control)
- **T2**: Tools that can introduce errors but are verified by other qualified tools (e.g., linkers)
- **T3**: Tools that can introduce errors and are not fully verified (e.g., compilers, static analyzers)

Qualified Tools
---------------

Compiler Toolchain
^^^^^^^^^^^^^^^^^^

**Tool**: RISC-V GCC Toolchain (riscv32-esp-elf-gcc)

**Version**: Determined by ESP-IDF version in dev container (GCC 12.2.0 for ESP-IDF v5.1.4)

**Classification**: T3 (code generation tool)

**Qualification Method**:

1. **Version Control**: Toolchain version is pinned in the development container (``.devcontainer/Dockerfile``), ensuring reproducible builds
2. **Build Reproducibility**: All compiler flags are documented in ``src/bist/CMakeLists.txt`` and recorded in build artifacts
3. **Verification**: Generated code is validated through:

   - Static analysis (see Static Analysis Tools section)
   - Runtime testing (QEMU and hardware validation)
   - Code review and inspection
4. **Configuration**: Compiler flags are explicitly set to ``-O0 -ggdb`` for the BIST library to ensure traceability and debuggability

**Compiler Flags** (from ``src/bist/CMakeLists.txt``):

- ``-O0``: No optimization (ensures code matches source)
- ``-ggdb``: Full debug information
- ``-Wall -Wextra -Werror=all``: Strict warnings treated as errors
- ``-std=gnu17``: C standard
- ``-ffunction-sections -fdata-sections``: Section flags for linker optimization
- ``-fstrict-volatile-bitfields``: Safety-critical volatile bitfield handling

**Evidence Location**:

- Toolchain configuration: ``cmake/toolchain.cmake``
- Build configuration: ``src/bist/CMakeLists.txt``
- Build artifacts: ``build/``

Linker Tool
^^^^^^^^^^^

**Tool**: RISC-V Binutils Linker (riscv32-esp-elf-ld)

**Version**: Matches GCC version (from ESP-IDF toolchain)

**Classification**: T2 (can introduce errors but verified by other tools)

**Qualification Method**:

1. **Linker Script Verification**: Linker script (``src/soc/{IDF_TARGET_PATH_NAME}/ld/linker.ld``) is reviewed and validated
2. **Memory Map Validation**: Generated memory map (``.map`` file) is verified against linker script
3. **Runtime Verification**: Memory placement is validated through:
   - PC test functions placed in specific regions
   - Stack sentinel placement verification
   - IRAM code execution verification

**Linker Flags** (from ``cmake/toolchain.cmake``):

- ``-nostartfiles``: Custom startup code
- ``-march=rv32imc_zicsr_zifencei``: RISC-V ISA specification
- ``--specs=nosys.specs``: Minimal system library

**Evidence Location**:

- Linker script: ``src/soc/{IDF_TARGET_PATH_NAME}/ld/linker.ld``
- Memory map: ``build/<app_name>.map``

Static Analysis Tools
^^^^^^^^^^^^^^^^^^^^^

**Tool**: cppcheck (static code analyzer)

**Version**: As specified in ``scripts/run_static_analysis.sh``

**Classification**: T3 (analysis tool)

**Qualification Method**:

1. **Configuration**: Analysis rules and suppressions are documented in:

   - ``cppcheck_suppressions.txt``: Suppressed warnings with justification
   - ``scripts/misra_rules.txt``: MISRA-C rule violations (if applicable)
   - ``scripts/misra.json``: MISRA-C configuration

2. **Automated Execution**: Static analysis is run as part of the build/CI process
3. **Results Review**: All warnings and errors are reviewed and addressed

**Evidence Location**:

- Analysis script: ``scripts/run_static_analysis.sh``
- Suppressions: ``cppcheck_suppressions.txt``
- MISRA configuration: ``scripts/misra.json``

Build System Tools
^^^^^^^^^^^^^^^^^^

**Tool**: CMake

**Version**: 3.22+ (as specified in build requirements)

**Classification**: T1 (build configuration tool, does not generate code)

**Qualification Method**:

1. **Version Control**: CMake version is specified in build requirements
2. **Deterministic Builds**: Build configuration is version-controlled in ``cmake/`` directory
3. **Reproducibility**: All build settings are recorded in ``CMakeCache.txt``

**Tool**: Ninja

**Version**: 1.10+ (as specified in build requirements)

**Classification**: T1 (build executor, does not generate code)

**Qualification Method**:

1. **Deterministic Execution**: Ninja ensures reproducible build order
2. **Build Artifacts**: All build outputs are traceable to source files

Test Tools
^^^^^^^^^^

**Tool**: QEMU (qemu-system-riscv32)

**Version**: As specified in development container

**Classification**: T2 (simulation tool, verified by hardware testing)

**Qualification Method**:

1. **Hardware Correlation**: QEMU test results are validated against hardware test results
2. **Fault Injection**: QEMU enables deterministic fault injection via GDB for test validation
3. **Limitations Documented**: Clock tests are hardware-only (QEMU cannot model crystal oscillators)

**Tool**: GDB (riscv32-esp-elf-gdb)

**Version**: From ESP-IDF toolchain

**Classification**: T1 (debugging tool, does not affect production code)

**Qualification Method**:

1. **Fault Injection**: Used for deterministic test validation
2. **No Production Impact**: GDB is only used during development and testing

**Tool**: pytest

**Version**: As specified in development container

**Classification**: T1 (test framework, does not affect production code)

**Qualification Method**:

1. **Test Execution**: Automated test execution with JUnit XML output
2. **Results Traceability**: Test results are recorded in ``build/tests/*_report.xml``

Evidence Summary
----------------

All tools used in the ESP-BIST development process are:

- **Version-controlled**: Tool versions are specified in development container and build configuration
- **Reproducible**: Build environment is containerized for consistency
- **Validated**: Tools are validated through testing and code review
- **Documented**: Tool configurations and qualifications are documented in this document and build artifacts

This tool qualification evidence supports IEC 60730-1 Annex H compliance by demonstrating that development tools are appropriately managed and validated.

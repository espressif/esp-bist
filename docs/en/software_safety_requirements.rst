Software Safety Requirements
=============================

Safety-Related Functions and Controlled Faults
-----------------------------------------------

The ESP-BIST library implements on-startup and runtime self-tests and monitoring intended to detect hardware and software faults that could cause hazardous behavior. The following table maps each safety-related function to the IEC 60730 standard component IDs (Table H.1) and the specific faults it is designed to control:

.. list-table::
   :header-rows: 1
   :widths: 5 20 30 25 20

   * - ID
     - Component
     - Description
     - Controlled Faults
     - Module Reference

   * - 1.1
     - CPU - Registers
     - Verifies integrity of CPU general-purpose registers
     - Register corruption, stuck-at faults, data path errors
     - :ref:`cpu-register-test`

   * - 1.3
     - CPU - Program Counter
     - Exercises PC bits, detects unexpected jumps or stuck PC
     - Control-flow faults, stuck PC, unexpected jumps, code execution outside intended regions
     - :ref:`program-counter-test`

   * - 3
     - Clock
     - Validates main and reference clock sources, monitors drift
     - Clock drift, oscillator failure, timing faults, missed deadlines
     - :ref:`clock-test`

   * - 4.1
     - Invariable Memory
     - Validates flash image via CRC32 (post-build and runtime)
     - Flash corruption, code/data tampering, image integrity loss
     - :ref:`flash-test`

   * - 4.2
     - Variable Memory
     - March tests, pattern/CRC checks, stack sentinel checks
     - RAM corruption, coupling/transition faults, stack overflows, memory retention errors
     - :ref:`ram-test`

   * - 6.3
     - Timing
     - Ensures time-slot and logical monitoring
     - Missed deadlines, infinite loops, program counter faults
     - :ref:`windowed-watchdog-operation`

   * - 7.1
     - Digital I/O
     - Verifies GPIO configuration and plausibility of IO levels
     - Peripheral misconfiguration, stuck-at faults, IO line failures, unexpected HW state
     - :ref:`gpio-plausibility-test`

This table provides direct traceability between each implemented safety function, IEC 60730 standard component identifiers, and the classes of faults each test is intended to control, supporting certification and safety case documentation.

Software Units Based on Class R1
---------------------------------

R1 (low-integrity) units are identified conservatively. For ESP-BIST, the entire BIST library is safety-critical and should be treated as safety-relevant code (SR). Non-safety supporting utilities (scripts under ``scripts/``) are NSR.

Safety-Relevant (SR) Units
^^^^^^^^^^^^^^^^^^^^^^^^^^

- Runtime tests and startup tests
- Watchdog interfaces
- CRC verification
- Failure reporting

Non-Safety-Relevant (NSR) Units
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- Test harness scripts
- Build helpers
- Host-side test utilities
- QEMU wrapper scripts

Management Strategy
-------------------

Error Reporting
^^^^^^^^^^^^^^^

The ESP-BIST library returns explicit error codes (``bist_esp_err_t``) for all test results. The application is responsible for interpreting these codes and implementing fail-safe handling, such as:

- Logging errors
- Initiating graceful shutdown
- Transitioning to safe state
- Triggering recovery

Watchdog Escalation
^^^^^^^^^^^^^^^^^^^

If the application cannot recover from detected faults within a configurable time window, the watchdog timer (WDT) is allowed to reset the SoC to recover to a known safe state. The application must periodically call ``wdt_feed()`` during normal operation; failure to do so triggers WDT timeout and reset.

Hardware/Software Interfaces
-----------------------------

The BIST library interfaces with hardware through a HAL layer; low-level access is implemented in ``components/`` and ``soc/`` directories. Key interfaces include:

- **Clock control and RTC** (``components/esp_hw_support/esp_clk.c``, ``soc/rtc.h``): CPU frequency, APB frequency, XTAL frequency queries, and RTC clock calibration for clock tests (32kHz external crystal and 40MHz main crystal monitoring)
- **Flash memory mapping and CRC regions** (``src/bist/core/memory/bist_flash.c``, ``scripts/calculate_crc32.py``): Post-build CRC32 injection into reserved flash sections and runtime readback/validation of ``.flash.text`` and ``.flash.rodata`` sections
- **GPIO and peripheral registers** (``src/bist/drivers/gpio.c``, ``hal/gpio_hal.h``): GPIO configuration, level setting/reading, pull-up/pull-down control for digital I/O plausibility tests
- **Watchdog APIs**:

  - Main System Watchdog Timer (MWDT) (``src/bist/drivers/wdt.c``, ``hal/wdt_hal.h``): Hardware watchdog for system monitoring and reset capability
  - External Crystal Watchdog (XT WDT) (``src/bist/drivers/xt_wdt.c``, ``hal/xt_wdt_hal.h``): 32kHz crystal oscillator failure detection
- **System Timer (SYSTIMER)** (``src/bist/drivers/esp_timer.c``, ``hal/systimer_hal.h``): Microsecond-precision timer for windowed watchdog underflow detection and clock test timing
- **Interrupt controller** (``esp_intr_alloc.h``, interrupt routing): Interrupt allocation, routing, and handler registration for watchdog timeouts, XT WDT failures, and timer callbacks
- **Peripheral control** (``components/esp_hw_support/periph_ctrl.c``): Peripheral clock gating and reset control for enabling/disabling hardware modules (e.g., SYSTIMER)
- **Memory interfaces**:

  - RAM direct access: Direct memory read/write for March A/X RAM tests (``src/bist/core/memory/bist_ram.c``)
  - RTC memory: RTC memory region for PC test function placement (``src/bist/core/cpu/bist_pc.c``)
  - Linker-defined memory regions: IRAM, Flash, and RTC memory sections via linker scripts (``src/soc/{IDF_TARGET_PATH_NAME}/ld/linker.ld``)

Safety-Relevant/Non-Safety-Relevant Software Interfaces
-------------------------------------------------------

Safety-Relevant (SR) Interfaces
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- The API that performs runtime and startup tests (``bist_*`` APIs in ``src/bist/``)
- Watchdog registration and callback interfaces

Non-Safety-Relevant (NSR) Interfaces
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- Host-side test utilities
- QEMU controllers
- Scripts used only in development/test

Build System
------------

The ESP-BIST project uses a CMake-based build system with explicit toolchain and compiler configuration to ensure reproducibility and traceability for safety certification.

**Key Safety Requirements:**

- **Build reproducibility:** All builds use a pinned toolchain version from the development container (see :doc:`tool_qualification` for detailed tool versions and qualification evidence)
- **Optimization level:** The BIST library is compiled with ``-O0 -ggdb`` (no optimization, full debug info) to ensure traceability, debuggability, and suitability for static analysis
- **Compiler flags:** Strict warning flags (``-Wall -Wextra -Werror=all``) and safety-critical flags (``-fstrict-volatile-bitfields``) are applied to the BIST library target
- **Build configuration authority:** Build settings are defined in ``src/bist/CMakeLists.txt`` and ``cmake/toolchain-{IDF_TARGET_PATH_NAME}.cmake``; these files are the authoritative source for all build settings

For detailed information on tool versions, compiler/linker flags, toolchain configuration, and tool qualification methodology, see :doc:`tool_qualification`.

Memory Model
------------

The project uses a static memory model: All safety code is built to run from fixed sections (RAM/IRAM/RODATA/FLASH) with explicit linker scripts. Dynamic memory allocation is avoided in safety-critical modules.

RAM Details
^^^^^^^^^^^

RAM usage is constrained: stack and data sizes are configured in the build system. BIST performs RAM tests that check patterns and stack boundaries. Persistent data is minimized in SR modules.

Memory Regions
^^^^^^^^^^^^^^

All physical memory regions are defined by the linker script (``src/soc/{IDF_TARGET_PATH_NAME}/ld/linker.ld``):

.. only:: esp32c3

  - ROM 0 (instruction): 0x4000_0000 - 0x4003_FFFF (256 KB, read-only ROM)
  - ROM 1 (instruction): 0x4004_0000 - 0x4005_FFFF (128 KB, read-only ROM)
  - ROM 1 (data): 0x3FF0_0000 - 0x3FF1_FFFF (128 KB, read-only ROM)
  - SRAM 0 (instructions): 0x4037_C000 - 0x4037_FFFF (16KB SRAM)
  - SRAM 1 (data/instruction): 0x3FC8_0000 - 0x3FCD_FFFF (384 KB SRAM)
  - RTC (data/instruction): 0x5000_0000 - 0x5000_1FFF
  - External Flash (data): 0x3C00_0000 - 0x3C7F_FFFF (8MB external flash)
  - External Flash (instruction): 0x4200_0000 - 0x427F_FFFF (8MB external flash)

.. only:: esp32c5

  - ROM: 0x4000_0000 - 0x4004_FFFF (320 KB, read-only ROM)
  - HP SRAM (data/instructions): 0x4080_0000 - 0x4085_FFFF (384 KB HP SRAM)
  - LP SRAM: 0x5000_0000 - 0x5000_3FFF (16 KB LP SRAM, retained in deep sleep; accessible by HP and LP CPU)
  - External Flash (via cache/MMU): 0x4200_0000 - 0x43FF_FFFF (up to 32 MB external flash)
  - External RAM (via cache/MMU): 0x4200_0000 - 0x43FF_FFFF (up to 32 MB external flash)

.. only:: esp32c6

  - ROM: 0x4000_0000 - 0x4004_FFFF (320 KB, read-only ROM)
  - HP SRAM (data/instructions): 0x4080_0000 - 0x4087_FFFF (512 KB HP SRAM)
  - LP SRAM: 0x5000_0000 - 0x5000_3FFF (16 KB LP SRAM, retained in deep sleep; accessible by HP and LP CPU)
  - External Flash (via cache/MMU): 0x4200_0000 - 0x42FF_FFFF (up to 16 MB external flash)

.. only:: esp32c61

  - ROM: 0x4000_0000 - 0x4003_FFFF (256 KB, read-only ROM)
  - HP SRAM (data/instructions): 0x4080_0000 - 0x4084_FFFF (320 KB HP SRAM)
  - External Flash (via cache/MMU): 0x4200_0000 - 0x43FF_FFFF (up to 32 MB external flash)

.. only:: esp32h2

  - ROM: 0x4000_0000 - 0x4001_FFFF (218 KB, read-only ROM)
  - HP SRAM (data/instructions): 0x4080_0000 - 0x4084_FFFF (320 KB HP SRAM)
  - LP SRAM: 0x5000_0000 - 0x5000_0FFF (4 KB LP SRAM, retained in deep sleep; accessible by HP and LP CPU)
  - External Flash (via cache/MMU): 0x4200_0000 - 0x42FF_FFFF (up to 16 MB external flash)

.. only:: esp32p4

  - HP ROM: 0x4FC0_0000 - 0x4FC1_FFFF (128 KB, read-only ROM)
  - HP SPM (data/instructions): 0x3010_0000 - 0x3010_1FFF (8 KB HP SPM, a volatile memory accessed by the HP CPU, finishing one access in two cycles)
  - HP L2MEM (data/instructions): 0x4FF0_0000 - 0x4FFB_FFFF (768 KB L2MEM, retained in light sleep)
  - LP ROM: 0x5010_0000 - 0x5010_3FFF (16 KB LP ROM, read-only ROM)
  - LP SRAM: 0x5010_8000 - 0x5010_FFFF (32 KB LP SRAM, retained in light sleep)
  - External Flash (via cache/MMU): 0x4000_0000 - 0x43FF_FFFF (up to 64 MB external flash)
  - External RAM (via cache/MMU): 0x4800_0000 - 0x4BFF_FFFF (up to 64 MB external RAM)

Section Placement
^^^^^^^^^^^^^^^^^

Code and data are mapped to specific regions:

- ``.text`` and ``.flash.text`` (code) to IROM or IRAM as required
- ``.rodata`` and ``.flash.rodata`` (read-only data) to DROM
- ``.data`` and ``.bss`` (initialized/uninitialized data) to DRAM
- Stack and heap boundaries are explicitly defined
- Special sections for test routines (e.g., ``.pc_test_X`` for PC test functions) are mapped to IRAM or RTC/LP IRAM to exercise specific address bits
- A dedicated region for CRC checksums is reserved in flash for integrity validation

Safety-Related Mapping
^^^^^^^^^^^^^^^^^^^^^^

- All ESP-BIST library code (from ``libbist_esp.a``) is placed in IRAM by default to guarantee deterministic execution independent of flash cache state. This eliminates timing variability and improves fault detection reliability.
- The stack sentinel region is placed at the end of DRAM and filled with a known pattern for overflow detection.
- Backup buffers for RAM tests are placed in a safe RAM section (``.dram0.safe_ram``) that is excluded from RAM test coverage.

Configurability and Evidence
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- The linker script is parameterized by CMake and Kconfig options, allowing stack size, heap size, and section addresses to be configured per build.
- All memory layout decisions are visible in the linker script and the generated map file (``.map`` in the build output).
- The exact memory layout and section assignments are recorded in the build artifacts for certification evidence.

For further details, see ``src/soc/{IDF_TARGET_PATH_NAME}/ld/linker.ld`` and the generated map file (e.g., ``samples/standalone/build/<app_name>.map``). These files provide the authoritative reference for memory layout and section placement in the ESP-BIST firmware.

Clock Rate
^^^^^^^^^^

The {IDF_TARGET_NAME} uses the following reference clock sources:

- **External low-frequency crystal:** 32.768 kHz

.. only:: esp32c3

  - **Main crystal oscillator:** 40 MHz

.. only:: esp32c5

  - **Main crystal oscillator:** 40 MHz

.. only:: esp32c6

  - **Main crystal oscillator:** 40 MHz

.. only:: esp32c61

  - **Main crystal oscillator:** 40 MHz

.. only:: esp32h2

  - **Main crystal oscillator:** 32 MHz

.. only:: esp32h4

  - **Main crystal oscillator:** 32 MHz

.. only:: esp32p4

  - **Main crystal oscillator:** 40 MHz

The BIST clock tests validate that the main crystal and the 32.768 kHz external crystal are present and operating within expected tolerances. The frequency ratio is measured at runtime and compared against thresholds defined by build-time constants and Kconfig options (see ``src/bist/Kconfig``).

For example, the allowed drift for the main crystal is set by ``CONFIG_ESP_BIST_CLOCK_PERCENT_FREQUENCY_DRIFT`` (default: ±1%). The BIST test will fail if the measured frequency deviates beyond this window, indicating a possible clock or oscillator fault.

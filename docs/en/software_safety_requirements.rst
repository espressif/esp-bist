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
     - Verifies integrity of CPU general-purpose registers; on FPU supported devices, also single-precision FPU registers (f0–f31)
     - Register corruption, stuck-at faults, data path errors
     - :ref:`cpu-register-test`

   * - 1.3
     - CPU - Program Counter
     - Exercises PC bits, detects unexpected jumps or stuck PC
     - Control-flow faults, stuck PC, unexpected jumps, code execution outside intended regions
     - :ref:`program-counter-test`

   * - 2
     - Interrupt handling and execution
     - Validates interrupt-matrix routing for software IRQ sources and concurrent hardware interrupt delivery via dual timer-group alarms
     - Stuck or missed interrupts, incorrect interrupt-matrix routing, enable/disable faults, incorrect interrupt frequency or period
     - :ref:`interrupt-test`

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
     - March tests (Class B), Abraham test (H.2.19.1, Class C), stack sentinel checks
     - RAM corruption, coupling/transition faults, address decoder faults, stack overflows, memory retention errors
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

   * - 7.2
     - Analog I/O
     - Verifies ADC configuration and plausibility of analog readings
     - ADC misconfiguration, stuck-at faults, conversion path failures, unexpected analog HW state
     - :ref:`adc-plausibility-test`

This table provides direct traceability between each implemented safety function, IEC 60730 standard component identifiers, and the classes of faults each test is intended to control, supporting certification and safety case documentation.

Software Unit Classification (SR / NSR)
---------------------------------------

In accordance with IEC 60730-1 and IEC 60335-1 Annex R, software units are classified into Safety-Relevant (SR) and Non-Safety-Relevant (NSR) units. All code executing on target hardware as part of the library is SR; off-target scripts and test applications are NSR.

Safety-Relevant (SR) Units
^^^^^^^^^^^^^^^^^^^^^^^^^^

- **BIST core modules** (``src/bist/core/``): CPU/CSR/FPU register integrity, PC, stack sentinel, RAM (March A/X, Abraham), Flash CRC32, interrupt, clock, and I/O plausibility tests.
- **Peripheral drivers** (``src/bist/drivers/``): Watchdog (MWDT and windowed WDT), XT WDT, high-resolution timer (SYSTIMER), GPIO, and ADC oneshot drivers.
- **SoC platform support** (``src/soc/``, ``components/``): Reset initialization, vector tables, panic handlers, and memory-mapping linker scripts.

Non-Safety-Relevant (NSR) Units
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **Validation test apps** (``tests/``): Standalone test binaries for QEMU and hardware validation.
- **Tooling and scripts** (``scripts/``, ``mcp-server/``): Build helpers, test runners, metrics collectors, and developer utilities.

NSR units run strictly off-target or in isolated test binaries and are never linked into production firmware, guaranteeing non-interference with SR execution.

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

- **Clock control and RTC** (``components/esp_hw_support/esp_clk.c``, ``soc/rtc.h``): CPU frequency, APB frequency, XTAL frequency queries, and RTC clock calibration for the main-crystal test and, where ``SOC_XT_WDT_SUPPORTED`` is set, the 32 kHz crystal-fail test
- **Flash memory mapping and CRC regions** (``src/bist/core/memory/bist_flash.c``, ``scripts/calculate_crc32.py``): Post-build CRC32 injection into reserved flash sections and runtime readback/validation of ``.flash.text`` and ``.flash.rodata`` sections
- **GPIO and peripheral registers** (``src/bist/drivers/gpio.c``, ``hal/gpio_hal.h``): GPIO configuration, level setting/reading, pull-up/pull-down control for digital I/O plausibility tests
- **ADC oneshot driver** (``src/bist/drivers/adc_oneshot.c``, ``src/bist/core/io/bist_adc.c``): ADC unit configuration, channel setup, raw reading, and GPIO pull control for analog I/O plausibility tests
- **Watchdog APIs**:

  - Main System Watchdog Timer (MWDT) (``src/bist/drivers/wdt.c``, ``hal/wdt_hal.h``): Hardware watchdog for system monitoring and reset capability
  - External Crystal Watchdog (XT WDT) (``src/bist/drivers/xt_wdt.c``, ``hal/xt_wdt_hal.h``): 32kHz crystal oscillator failure detection
- **System Timer (SYSTIMER)** (``src/bist/drivers/esp_timer.c``, ``hal/systimer_hal.h``): Microsecond-precision timer for windowed watchdog underflow detection and clock test timing
- **Interrupt controller** (``esp_cpu.h``, ``esp_rom_sys.h``, ``esp_intr_alloc.h``, interrupt matrix): CPU interrupt enable/disable, priority/type configuration, handler installation, and interrupt-matrix routing for watchdog timeouts, XT WDT failures, timer callbacks, and the interrupt self-test (software IRQ sources and timer-group alarms)
- **Timer Groups (TIMG)** (``hal/timer_hal.h``, ``hal/timer_ll.h``, ``hal/timg_ll.h``): GPTimer alarms used by interrupt test to exercise concurrent hardware interrupt delivery and period ratio checks
- **Peripheral control** (``components/esp_hw_support/periph_ctrl.c``): Peripheral clock gating and reset control for enabling/disabling hardware modules (e.g., SYSTIMER)
- **Memory interfaces**:

  - RAM direct access: Direct memory read/write for March A/X RAM tests (``src/bist/core/memory/bist_ram.c``)
  - RTC memory: RTC memory region for PC test function placement (``src/bist/core/cpu/bist_pc.c``)
  - Linker-defined memory regions: IRAM, Flash, and RTC memory sections via linker scripts (``src/soc/{IDF_TARGET_PATH_NAME}/ld/linker.ld``)

Build System
------------

The ESP-BIST project uses a CMake-based build system with explicit toolchain and compiler configuration to ensure reproducibility and traceability for safety certification.

**Key Safety Requirements:**

- **Build reproducibility:** All builds use a pinned toolchain version from the development container (see :doc:`tool_qualification` for detailed tool versions and qualification evidence).
- **Optimization level:** The BIST library is compiled with ``-Os -ggdb`` (optimize for size, full debug info). The ``-Os`` level is used because at ``-O0`` all local variables are spilled to the stack, making safety-critical code vulnerable to self-corruption when a test writes to memory overlapping its own stack frame (see RAM test safe-stack design in :doc:`module_design_and_coding`). Specific function-level attributes safeguard execution semantics under ``-Os``:

  - ``__attribute__((optimize("O0")))`` on ``bist_cpu_stack_recursive`` to prevent the compiler from optimizing away recursive calls.
  - ``__attribute__((naked))`` on ``bist_cpu_regs_test`` and ``bist_cpu_csr_regs_test`` to prevent compiler prologue/epilogue generation from corrupting register tests and caller stack frames.
  - ``__attribute__((noinline))`` on RAM march routines (``march_a_impl``, ``march_x_impl``, ``abraham_impl``) to preserve call boundaries during safe-stack relocation.

- **Compiler flags:** Strict warning flags (``-Wall -Wextra -Werror=all``) and safety-critical flags (``-fstrict-volatile-bitfields``) are applied to all BIST targets. All C compilation units force-include ``esp_assert.h`` via compile options.
- **Build configuration authority:** Build settings are defined in ``src/bist/CMakeLists.txt``, ``cmake/project.cmake``, and ``cmake/toolchain.cmake``; these files are the authoritative source for all build settings.

**Warning Suppressions:**

The following compiler warning suppressions are applied to third-party ESP-IDF source files compiled as part of the BIST build. Each suppression is limited to the specific file and justified below.

.. list-table::
   :header-rows: 1
   :widths: 25 15 60

   * - File
     - Warning
     - Justification
   * - ``esp_system/panic.c`` (ESP-IDF)
     - ``-Wno-shadow``
     - ESP-IDF's panic handler intentionally declares a local ``rtc_wdt_ctx`` that shadows the file-scope static of the same name and type (``wdt_hal_context_t``). The local copy is used to avoid race conditions when multiple cores enter the panic handler simultaneously. This is a defensive coding pattern in the upstream IDF code (see IDF source comment at the declaration site). The shadowed variable is confined to a single function (``esp_panic_handler_enable_rtc_wdt``) and does not affect BIST safety-relevant code paths. No BIST-owned source files have this suppression applied.

No warning suppressions are applied to any BIST-owned source files (``src/``, ``components/``).

**Per-File Compiler Option Overrides:**

The following non-default compiler options are applied to specific third-party ESP-IDF source files. Each override is limited to the specific file and justified below.

.. list-table::
   :header-rows: 1
   :widths: 25 15 60

   * - File
     - Option
     - Justification
   * - ``esp_hal_wdt/xt_wdt_hal.c`` (ESP-IDF)
     - ``-fgnu89-inline``
     - ESP-IDF's low-level XT WDT header (``hal/xt_wdt_ll.h``) declares helper functions as ``inline`` without ``static``. Under the project ``-std=gnu17``, that is an inline definition and does not emit an external symbol. The linker then fails with undefined references. ``src/bist/CMakeLists.txt`` adds ``-fgnu89-inline`` to this one translation unit on ESP32-C3, on top of the library ``-Os``, so the function bodies are emitted in place. The override does not change BIST-owned code.
   * - ``esp_hw_support/port/esp32c61/ocode_init.c`` (ESP-IDF)
     - ``-include esp_attr.h``
     - ESP-IDF v6.0's ``esp32c61/ocode_init.c`` uses attributes ``NOINLINE_ATTR`` and ``IRAM_ATTR`` without explicitly including ``esp_attr.h``. Under strict compiler options, macros default to empty or trigger undeclared identifier errors. ``cmake/project.cmake`` force-includes ``esp_attr.h`` for this single translation unit when building for ESP32-C61.

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
  - External RAM (via cache/MMU): 0x4200_0000 - 0x43FF_FFFF (up to 32 MB external RAM)

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

  - ROM: 0x4000_0000 - 0x4001_FFFF (128 KB, read-only ROM)
  - HP SRAM (data/instructions): 0x4080_0000 - 0x4084_FFFF (320 KB HP SRAM)
  - LP SRAM: 0x5000_0000 - 0x5000_0FFF (4 KB LP SRAM, retained in deep sleep; accessible by HP and LP CPU)
  - External Flash (via cache/MMU): 0x4200_0000 - 0x42FF_FFFF (up to 16 MB external flash)

.. only:: esp32h4

  - ROM: 0x4000_0000 - 0x4004_FFFF (320 KB, read-only ROM)
  - HP SRAM (data/instructions): 0x4081_0000 - 0x4085_FFFF (320 KB HP SRAM)
  - External Flash (via cache/MMU): 0x4200_0000 - 0x43FF_FFFF (up to 32 MB external flash)
  - External RAM (via cache/MMU): 0x4200_0000 - 0x43FF_FFFF (up to 32 MB external RAM)

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

Code, data, and safety structures are mapped to explicit linker sections:

- **IRAM (``.iram0.text``)**: All ESP-BIST library code (``libbist_esp.a``), startup routines, exception vector tables, and ``pc_test_0`` are placed in IRAM to guarantee deterministic execution independent of flash cache state.
- **Flash / IROM (``.flash.text``)**: General application code, as well as ``pc_test_1`` and ``pc_test_2`` (separated by a 64 KB gap to test upper address bits without consuming RAM).
- **DRAM (``.dram0.data``, ``.dram0.bss``)**: Application and BIST variables. All BIST library read-only data (``*libbist_esp.a:*(.rodata*)``) is placed in DRAM to ensure cache-independent access during self-tests.
- **Flash / DROM (``.flash.rodata``)**: Read-only application constants, descriptors, and tables. No executable code is mapped to DROM; the memory protection unit enforces read-only access.
- **Safe RAM (``.dram0.safe_ram``)**: Dedicated 256-byte RAM-test safe stack and backup buffers, excluded from the RAM test region so that the march algorithms can test the entire DRAM without self-corruption.
- **Stack Sentinel (``.dram0.overflow``)**: Guard block at the bottom of the stack (sized by ``CONFIG_ESP_BIST_STACK_PROTECTION_BLOCK_SIZE``) initialized with pattern ``0xA55A5AA5`` for stack overflow detection.
- **Dedicated Test Regions (``.rtc.text``, ``.tcm.text``)**: Holds ``pc_test_3`` in RTC/LP RAM, and ``pc_test_4`` in TCM on ESP32-P4 to verify execution across disparate memory spaces.
- **Flash CRC Sections (``.crc_section_text``, ``.crc_section_data``)**: Dedicated 4-byte fields placed at the end of DROM holding post-build CRC32 checksums of ``.flash.text`` and ``.flash.rodata`` for runtime integrity verification.

Configurability and Evidence
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- The linker script is parameterized by CMake and Kconfig options, allowing stack size, heap size, and section addresses to be configured per build.
- All memory layout decisions are visible in the linker script and the generated map file (``.map`` in the build output).
- The exact memory layout and section assignments are recorded in the build artifacts for certification evidence.

For further details, see ``src/soc/{IDF_TARGET_PATH_NAME}/ld/linker.ld`` and the generated map file (e.g., ``samples/standalone/build/<app_name>.map``). These files provide the authoritative reference for memory layout and section placement in the ESP-BIST firmware.

Reference Clock Sources and Crystal Frequencies
-----------------------------------------------

The {IDF_TARGET_NAME} uses the following reference crystal clock sources:

- **External low-frequency crystal:** 32.768 kHz

.. only:: esp32c3

  - **Main crystal oscillator:** 40 MHz

.. only:: esp32c5

  - **Main crystal oscillator:** 48 MHz

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

.. note::

   These values represent the physical crystal oscillators (``CONFIG_XTAL_FREQ``) tested by ``bist_main_crystal_test()``.

``bist_main_crystal_test()`` measures the main crystal against the 32.768 kHz reference with ``rtc_clk_cal()`` and fails when the deviation exceeds ``CONFIG_ESP_BIST_CLOCK_PERCENT_FREQUENCY_DRIFT``. The expected main-crystal frequency is ``CONFIG_XTAL_FREQ`` in the SoC ``sdkconfig.h``. ``bist_ext_crystal_fail_test()`` monitors the 32.768 kHz crystal with the XT WDT only when ``SOC_XT_WDT_SUPPORTED`` is set; otherwise it returns ``BIST_ESP_OK`` without a measurement.

For example, the allowed drift for the main crystal is set by ``CONFIG_ESP_BIST_CLOCK_PERCENT_FREQUENCY_DRIFT`` (default: ±1%). The BIST test will fail if the measured frequency deviates beyond this window, indicating a possible clock or oscillator fault.

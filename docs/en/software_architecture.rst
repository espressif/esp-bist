Software Architecture
=====================

Architecture Overview and Layering
-----------------------------------

This chapter describes the **standalone** bare-metal architecture
(``samples/standalone``). For the LP-companion plus HP-agent supervision
architecture (ESP-IDF, Zephyr, NuttX), see :doc:`host_diagnostics`.

The ESP-BIST standalone firmware uses a three-layer architecture:

- **Application layer** — Integrates BIST tests and fail-safe logic (e.g., ``samples/standalone/main.c``). Runs post-boot tests, registers watchdog and crystal failure callbacks, initializes stack overflow detection, and runs runtime tests in the main loop.
- **BIST library layer** (``src/bist/``) — Core safety test routines and driver wrappers:

  - Core modules ``core/cpu/``, ``core/interrupt/``, ``core/memory/``, ``core/clock/``, ``core/wdt/``, ``core/io/``
  - Driver layer ``drivers/`` for watchdog, XT WDT, GPIO, timer
  - Public API headers ``src/bist/include/`` and ``src/bist/core/include/`` expose test functions and error codes
- **SoC and HAL layer** (``soc/{IDF_TARGET_PATH_NAME}/``, ``components/``) — Startup code, vector table, linker script, minimal HAL stubs, and memory layout control.

Key Build Properties
^^^^^^^^^^^^^^^^^^^^
- CMake + Ninja build; toolchain pinned in dev container and ``cmake/toolchain.cmake``
- BIST library compiled with ``-Os``/``-ggdb``, strict warnings, ``-std=gnu17``, section flags, and volatile bitfield safety flags; per-function ``-O0`` overrides where needed
- All BIST objects placed in IRAM by linker for deterministic timing; CRC regions reserved in flash
- Kconfig-driven configuration compiled into ``bist_conf.h``; timing and memory parameters recorded in build artifacts

Modules Architecture
--------------------

- **CPU tests** (``core/cpu/``): register integrity, CSR integrity, PC integrity (functions placed in IRAM/Flash/RTC), stack overflow detection
- **Memory tests** (``core/memory/``): RAM March A/X, Abraham (H.2.19.1 time-division); flash CRC validation
- **Interrupt tests** (``core/interrupt/``): software IRQ source mapping and dual timer-group hardware interrupt delivery
- **Clock tests** (``core/clock/``): XT WDT 32kHz monitoring (on SoCs with ``SOC_XT_WDT_SUPPORTED``); 40MHz crystal drift measurement
- **WDT tests** (``core/wdt/``): watchdog init and stack overflow handler registration
- **IO tests** (``core/io/``): GPIO output/input and ADC low/high/reference plausibility checks
- **Drivers** (``drivers/``): MWDT/windowed WDT, XT WDT, GPIO, timer wrappers
- **Performance Metrics** (``include/bist_metrics.h``): Macro-based Performance Counter CSR interface for measuring CPU cycles, instruction counts, and microarchitectural events during BIST test execution
- **SoC support** (``soc/{IDF_TARGET_PATH_NAME}/``): startup, vectors, linker script, newlib stubs

Hierarchy & Call Structure
--------------------------

.. blockdiag::
    :scale: 100%
    :caption: BIST Call Structure
    :align: center

    blockdiag {
        Boot -> "BIST Post boot tests" -> "Success?";
        "Success?" -> Application [label = "Yes"];
        "Success?" -> "Safe State" [label = "No"];
        Application -> "Runtime Loop";
        "Runtime Loop" -> "Periodic BIST";
        "Periodic BIST" -> "Tests Pass?";
        "Tests Pass?" -> "Runtime Loop" [label = "Yes"];
        "Tests Pass?" -> "Safe State" [label = "No"];

        Boot [shape = roundedbox];
        "BIST Post boot tests" [shape = box];
        "Success?" [shape = diamond];
        Application [shape = box];
        "Safe State" [shape = box];
        "Runtime Loop" [shape = box];
        "Periodic BIST" [shape = box];
        "Tests Pass?" [shape = diamond];
    }

Interrupt Handling
------------------

- Vector table in ``soc/{IDF_TARGET_PATH_NAME}/vectors.S`` mapped to IRAM.
- MWDT interrupt before reset; callback registered via ``wdt_register_callback`` must be short and deterministic.
- XT WDT interrupt for 32kHz crystal failure via ``esp_xt_wdt_register_callback`` (available on SoCs with ``SOC_XT_WDT_SUPPORTED``; on other SoCs the external crystal fail test is skipped).
- Interrupt self-test (``core/interrupt/``, enabled by ``CONFIG_ESP_BIST_INTERRUPT_TEST``) temporarily installs ISRs and interrupt-matrix routes, then tears them down:

  - Software path: maps ``CPU_INTR_FROM_CPU_0..3`` to CPU interrupt line 9, triggers each source, and verifies enable-mask state plus ISR delivery count.
  - Hardware path: routes TIMG0/TIMG1 GPTimer alarms to CPU interrupt lines 9 and 10, then checks that ISR counts follow the configured 2:1 period ratio.
- Outside the interrupt self-test, the library does not retain application ISRs; watchdog and XT WDT paths continue to use registration callbacks only.

Data Storage Model
------------------

- **Flash/ROM**: ``.flash.text`` / ``.flash.rodata`` mapped to IROM/DROM; CRC stored in dedicated flash region.
- **IRAM**: All ``libbist_esp.a`` code placed in IRAM for deterministic timing; ``pc_test_0`` placed at end of IRAM.
- **DRAM**: ``.data``/``.bss`` for app and BIST; rodata placed in DRAM for timing determinism; stack/heap bounded; stack sentinel region at bottom of stack (sized by ``CONFIG_ESP_BIST_STACK_PROTECTION_BLOCK_SIZE``); ``.dram0.safe_ram`` section holds backup buffer and the 256-byte RAM-test safe stack, excluded from the RAM test region so the march algorithms can test the full DRAM including the normal stack.
- **Flash (ICache)**: ``pc_test_1`` and ``pc_test_2`` placed in ``.flash.text`` with a 64KB gap to invert bits [3:15]; avoids consuming SRAM.
- **RTC/LP RAM**: Small RAM region used by ``pc_test_3``.
- **Configuration**: Generated ``bist_conf.h`` carries Kconfig options (timeouts, drift thresholds, etc.).

Time-Based Dependencies
-----------------------

- **Watchdog timing**: ``CONFIG_ESP_BIST_WDT_TIMEOUT_US`` defines feed window; reset at ~2× stage interval.
- **Windowed watchdog**: ``CONFIG_ESP_BIST_WDT_WINDOWED_UNDERFLOW_TIMEOUT_US`` enforces minimum feed interval; early feeds flag underflow.
- **Clock tests**: Measure frequency ratio vs 32.768 kHz; tolerance via ``CONFIG_ESP_BIST_CLOCK_PERCENT_FREQUENCY_DRIFT``.
- **XT WDT**: Detects 32kHz failure after ~200 cycles.
- **Runtime tests**: CPU/CSR/stack/PC executed within watchdog window; deterministic bounded execution.
- **Post-boot tests**: RAM, flash, stack, GPIO, and interrupt tests run once at startup; integrators ensure total time fits safety goals.

Hardware/Software Interfaces
----------------------------

- Clock control via XT WDT (where supported) and ESP timer
- Flash CRC injection/readback via ``scripts/calculate_crc32.py`` and runtime CRC in ``bist_flash_test``
- GPIO configuration and I/O via driver wrappers
- ADC oneshot configuration and raw reading via driver wrappers
- Watchdog APIs for MWDT/windowed and XT WDT callbacks
- Interrupt controller and matrix for watchdog-related handlers and the interrupt self-test

Error Control Measures (Architecture)
-------------------------------------

- SR/NSR separation: BIST library and SoC support are SR; scripts/tests are NSR.
- Minimal TCB: small modular tests, no dynamic allocation in SR.
- CRC redundancy: post-build CRC injection; runtime verification.
- Build reproducibility: toolchain and flags fixed in CMake; IRAM placement for deterministic timing.
- Stack boundary checking: sentinel region ``[_stack_overflow_protection_end, _stack_overflow_protection_start)`` defined by linker/Kconfig.

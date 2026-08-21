Software Validation
===================

Validation Overview
-------------------

The ESP-BIST library is validated with three complementary approaches:

1. **QEMU-based emulation**: full system simulation with fault injection via GDB for deterministic, repeatable testing in CI pipelines

2. **Hardware testing**: On-device validation on actual {IDF_TARGET_NAME} hardware for real-world condition verification

3. **Host unit testing**: Off-target tests in ``tests/unit/`` that run on the build machine without hardware or an emulator, covering pure algorithmic helpers and protocol logic that can be exercised outside the firmware environment

QEMU and hardware tests use pytest with the Unity test framework. Host unit tests are built with CMake and run under ctest.

Test Execution Infrastructure
-----------------------------

QEMU Environment Setup
^^^^^^^^^^^^^^^^^^^^^^
- QEMU: ``qemu-system-riscv32`` machine ``{IDF_TARGET_PATH_NAME}`` with ``-icount 3`` (instruction counting mode for deterministic execution)
- Flash image: ``build/<app_name>_qemu_image.bin`` - Raw binary image with MCUboot header and metadata
- GDB: ``riscv32-esp-elf-gdb`` for remote debugging at TCP port `:1234` (QEMU debug server)
- Test framework: Unity testing framework (from ESP-IDF) with assertion macros (`TEST_ASSERT_EQUAL`, etc.)

Test Execution Flow
^^^^^^^^^^^^^^^^^^^

.. blockdiag::
    :scale: 100%
    :caption: Test Execution Flow
    :align: center

    blockdiag {
        Build -> "Start QEMU\n(debug/normal)" -> "GDB fault\ninjection\n" -> "Test runs" -> "pytest validates\noutput" -> Cleanup;
    }

GDB Fault Injection Pattern
^^^^^^^^^^^^^^^^^^^^^^^^^^^

All fault injection follows this GDB script pattern:

.. code-block::

    # Connect to QEMU debug server
    target remote :1234

    # Break, modify, resume
    tb <test_label>
    continue

    commands
        set <reg_or_var>=<fault_value>
        continue
    end

Key characteristics:

- Breakpoints are set before test execution begins
- Modifications are applied deterministically when breakpoint is triggered
- Execution resumes immediately after modification
- GDB process terminates, leaving QEMU running the faulted test
- Test output is captured by pytest and validated


CPU Register Integrity Test
---------------------------

**Purpose:** Verify that all CPU general-purpose registers (X1-X31) maintain integrity and can retain both 0 and 1 patterns without corruption.

QEMU/Emulation Validation
^^^^^^^^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/cpu_reg_test/pytest_qemu_cpu_reg_test.py``

**Execution:**

1. QEMU starts in normal mode (no debug), executes ``bist_cpu_regs_test()``
2. Test writes 0xAAAAAAAA to each register X1-X31, reads back and compares
3. Test writes 0x55555555 to each register, reads back and compares
4. Test returns ``BIST_ESP_OK`` and prints "test_BIST_Cpu_Regs:PASS"
5. pytest validates output presence within 3-second timeout

**Expected Output:**

.. code-block::

    test_BIST_Cpu_Regs:PASS

**Failure Test: ``test_reg_error`` (Parameterized, 32 registers)**

**GDB Fault Injection Script:**

.. code-block::

    # Connect to QEMU debug server
    target remote :1234

    # Set temporary breakpoint at register test label
    tb testRegA_<reg_name>
    continue

    # When breakpoint is hit, modify register to wrong value
    commands
        set $ra=0x55555555
        continue
    end

**Script Parameterization:**

The script is executed 32 times, once for each register:

- ``ra``, ``sp``, ``gp``, ``tp``
- ``t0``, ``t1``, ``t2``, ``t3``, ``t4``, ``t5``, ``t6``
- ``s0``, ``s1``, ``s2``, ``s3``, ``s4``, ``s5``, ``s6``, ``s7``, ``s8``, ``s9``, ``s10``, ``s11``
- ``a0``, ``a1``, ``a2``, ``a3``, ``a4``, ``a5``, ``a6``, ``a7``

Each test replaces ``<reg_name>`` with the specific register name.

**Fault Injection Method:**

1. GDB sets breakpoint at label ``testRegA_<reg_name>`` (where register is tested against 0xAAAAAAAA)
2. When breakpoint hits, register is forcibly set to 0x55555555 (wrong value)
3. Test continues and reads register, expecting 0xAAAAAAAA
4. Mismatch detected, test returns error code ``BIST_ESP_CPU_TEST_ERR``
5. Test prints "test_BIST_Cpu_Regs:FAIL"

**Test Coverage:** 32 test cases (one per register)

**Expected Output per Register:**

.. code-block::

    test_BIST_Cpu_Regs:FAIL

Hardware Validation
^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/cpu_reg_test/pytest_device_cpu_reg_test.py``

**Expected Inputs/Environment:**

- Device running at nominal clock frequency (40 MHz typical)
- No active interrupts during test execution

**Execution:**

1. Flash firmware to device via ``ninja -C build flash``
2. Run ``pytest pytest_device_cpu_reg_test.py`` to capture serial output
3. Application initializes and runs ``bist_cpu_regs_test()``
4. Test writes patterns (0xAAAAAAAA, 0x55555555) to all 32 registers
5. Serial monitor captures "test_BIST_Cpu_Regs:PASS" message
6. pytest validates presence in output

**Expected Behavior:**

- All 32 registers pass integrity checks on real hardware
- No stuck-at faults or register corruption detected

CPU CSR Integrity Test
----------------------

**Purpose:** Verify that CPU Control and Status Registers maintain integrity and can be read/written reliably. Tested CSRs include trap registers (MTVEC, MSCRATCH, MEPC, MCAUSE, MTVAL), PMP registers (PMPADDR0–15, PMPCFG0–3), PMA address registers (pma_addr0–11) on PMA-capable SoCs (C6, H2, C5), and MEXSTATUS/MHINT on C5 only.

QEMU/Emulation Validation
^^^^^^^^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/cpu_reg_test/pytest_qemu_cpu_reg_test.py``

**Execution:**

1. QEMU executes ``bist_cpu_csr_regs_test()``
2. Test saves original CSR value
3. Writes 0xAAAAAAAA masked by CSR-specific write mask, reads back
4. Writes 0x55555555 masked by CSR-specific write mask, reads back
5. Restores original CSR value
6. Returns ``BIST_ESP_OK`` if all CSRs pass, ``BIST_ESP_CPU_CSR_TEST_ERR`` on mismatch

**Expected Output:**

.. code-block::

    test_BIST_Cpu_Csr_Regs:PASS

**GDB Fault Injection Script:**

.. code-block::

    # Connect to QEMU debug server
    target remote :1234

    # Set temporary breakpoint at CSR test label
    tb testRegA_<csr_name>
    continue

    # When breakpoint is hit, modify temporary register used in CSR verification
    commands
        set $t0=0x55555555
        continue
    end

**Script Parameterization:**

The script is executed once for each CSR. The full list varies by SoC:

- **Trap CSRs** (all SoCs): ``mtvec``, ``mscratch``, ``mepc``, ``mcause``, ``mtval``
- **PMP Address CSRs** (all SoCs): ``pmpaddr0``–``pmpaddr15``
- **PMP Configuration CSRs** (all SoCs): ``pmpcfg0``–``pmpcfg3``
- **PMA Address CSRs** (C6, H2, C5 only): ``0xBD0``–``0xBDB`` (pma_addr0–11)
- **Custom CSRs** (C5 only): ``0x7E1`` (mexstatus), ``0x7C5`` (mhint)

Each test replaces ``<csr_name>`` with the specific CSR name or number.

**Note:** GDB modifies the temporary register ``t0`` which is used by the test logic to verify CSR contents.

**Fault Injection Method:**

1. GDB sets breakpoint at CSR test label
2. When hit, modifies temporary register (t0) to incorrect value
3. Test logic uses t0 to verify CSR contents
4. Mismatch detected, test fails
5. Returns ``BIST_ESP_CPU_CSR_TEST_ERR``

**Test Coverage:** 25 test cases on ESP32-C3 (5 trap + 16 pmpaddr + 4 pmpcfg), 37 on ESP32-C6/H2/C61 (adds 12 pma_addr), 39 on ESP32-C5 (adds 12 pma_addr + mexstatus + mhint), 40 on ESP32-H4/P4 (adds 12 pma_addr + ``fflags``, ``frm``, ``fcsr``)

**Expected Output per CSR:**

.. code-block::

    test_BIST_Cpu_Csr_Regs:FAIL

Hardware Validation
^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/cpu_reg_test/pytest_device_cpu_reg_test.py``

**Expected Inputs/Environment:**

- Device running with standard CSR configuration
- Machine mode privilege level (typical for bare-metal)
- PMP registers available for testing; PMA address registers available on C6/H2/C5/P4; mexstatus/mhint on C5 only; FPU CSRs on H4/P4

**Execution:**

1. Flash firmware to device
2. Run ``pytest pytest_device_cpu_reg_test.py`` to capture serial output
3. Application runs ``bist_cpu_csr_regs_test()``
4. Test saves original CSR values, writes patterns, verifies, and restores

**Expected Behavior:**

- All CSRs pass integrity checks on real hardware (25 on C3, 37 on C6/H2/C61, 39 on C5, 40 on H4/P4)
- No stuck-at faults or CSR corruption detected

CI Test Results
^^^^^^^^^^^^^^^

**QEMU Test:**

.. xml-junit-test-results:: tests/cpu_reg_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml
   :title: CPU Register QEMU Test Results

**Device Test:**

.. xml-junit-test-results:: tests/cpu_reg_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml
   :title: CPU Register Device Test Results

Stack Overflow Detection Test
-----------------------------

**Purpose:** Detect and report stack overflow conditions that could corrupt memory or cause undefined behavior.

QEMU/Emulation Validation
^^^^^^^^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/cpu_stack_test/pytest_qemu_cpu_stack_test.py``

**Execution:**

1. QEMU runs the Unity test suite in ``tests/cpu_stack_test/main.c``
2. ``test_BIST_Cpu_Stack_Overflow_Bounds``: verifies that ``bist_cpu_stack_overflow_init()`` fills only the protection region ``[_end, _start)`` and does **not** overwrite the word at ``_stack_overflow_protection_start``. The test writes a probe pattern (``0xCAFEBABE``) to that word, calls init, and asserts the probe survives. A minimum SP margin (``BOUNDS_TEST_MIN_SP_MARGIN = 256``) is checked first to ensure init's own stack frame cannot reach the probe.
3. ``test_BIST_Cpu_Stack_Overflow``: initializes sentinel, performs intentional deep recursion with 128-byte stack frames (count_max=20000), and detects sentinel corruption via ``bist_cpu_stack_overflow_check()``.

**Expected Output:**

.. code-block::

    test_BIST_Cpu_Stack_Overflow_Bounds:PASS
    test_BIST_Cpu_Stack_Overflow:PASS

**GDB Fault Injection Script:**

.. code-block::

    # Connect to QEMU debug server
    target remote :1234

    # Set temporary breakpoint at stack test data
    tb bist_cpu_stack_overflow_data
    continue

    # Modify recursion depth to insufficient value
    commands
        set count_max=10
        continue
    end

**Fault Injection Method:**

1. GDB sets breakpoint at stack test initialization
2. When hit, reduces recursion depth (count_max) to 10 (insufficient for overflow)
3. Test attempts recursion but cannot reach deep enough to corrupt sentinel
4. Stack overflow detection fails to trigger
5. Returns ``BIST_ESP_STACK_TEST_ERR``

**Expected Output:**

.. code-block::

    test_BIST_Cpu_Stack_Overflow:FAIL

Hardware Validation
^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/cpu_stack_test/pytest_device_cpu_stack_test.py``

**Expected Inputs/Environment:**

- Device with configured stack size and sentinel region
- Sufficient RAM for 20000 recursive calls with 128-byte frames

**Execution:**

1. Flash firmware to device
2. Run ``pytest pytest_device_cpu_stack_test.py``
3. Application runs ``bist_cpu_stack_overflow_test()``
4. Deep recursion corrupts stack sentinel
5. Overflow detection confirms protection is functional

**Expected Behavior:**

- Stack overflow detected on real hardware
- Sentinel pattern successfully corrupted and identified

CI Test Results
^^^^^^^^^^^^^^^

**QEMU Test:**

.. xml-junit-test-results:: tests/cpu_stack_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml
    :title: Stack Overflow QEMU Test Results

**Device Test:**

.. xml-junit-test-results:: tests/cpu_stack_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml
    :title: Stack Overflow Device Test Results

RAM Integrity Test
------------------

**Purpose:** Verify that RAM can store and retrieve both 0 and 1 patterns reliably, detecting coupling faults and data path errors.

QEMU/Emulation Validation
^^^^^^^^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/ram_test/pytest_qemu_ram_test.py``

**Execution:**

1. QEMU executes all RAM test algorithms over linker-defined RAM region
2. **March A** (ascending):
   - Write 0x00000000 to all addresses ascending
   - Read 0, write 0xFFFFFFFF ascending
   - Read 0xFFFFFFFF ascending
3. **March X** (asc/desc):
   - Write 0x00000000 to all addresses ascending
   - Read 0, write 0xFFFFFFFF ascending
   - Read 0xFFFFFFFF, write 0 descending
   - Read 0, write 0xFFFFFFFF ascending
   - Read 0xFFFFFFFF, write 0 descending
4. **Abraham** (H.2.19.1, time-division):
   - Single period: tests one partition pair through the 10-element/30n-op sequence
   - Full: resets pair schedule and runs all C(N,2) pairs to completion
5. Backup/restore chunk buffer preserves RAM content on failure
6. Returns ``BIST_ESP_OK`` if all patterns verified

**Expected Output:**

.. code-block::

    test_BIST_ram_march_a:PASS
    test_BIST_ram_march_x:PASS
    test_BIST_ram_abraham:PASS
    test_BIST_ram_abraham_full:PASS

**Failure Test: Memory Corruption via GDB**

**GDB Fault Injection Script:**

.. code-block::

    # Connect to QEMU debug server
    target remote :1234

    # Set temporary breakpoint at RAM test step 2
    tb bist_ram_test_march_a_step2
    continue

    # Corrupt memory to cause pattern mismatch
    commands
        set _bist_ram_test_start=0xFF
        continue
    end

**Fault Injection Method:**

1. GDB sets breakpoint during RAM pattern verification (step 2: Read 0, Write 1)
2. When hit, writes ``0xFF`` to the first word of the test region via the linker
   symbol ``_bist_ram_test_start`` (the local pointer ``start_addr`` is optimized
   into a register by ``-Os`` and is not accessible to GDB)
3. Test continues and reads memory, expecting the all-zeros pattern written in step 1
4. Mismatch detected, returns ``BIST_ESP_RAM_TEST_ERR``
5. Test prints "test_BIST_ram_march_x:FAIL"

**Test Coverage:** 3 test cases (March A, March X, and Abraham algorithms)

**Expected Output per Test:**

.. code-block::

    test_BIST_ram_march_x:FAIL

Hardware Validation
^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/ram_test/pytest_device_ram_test.py``

**Expected Inputs/Environment:**

- Device with full RAM available for testing
- No external access to RAM during test execution
- Backup buffer in safe RAM section (excluded from test)

**Execution:**

1. Flash firmware to device
2. Run ``pytest pytest_device_ram_test.py``
3. Application runs ``bist_ram_test_march_a()``, ``bist_ram_test_march_x()``,
   ``bist_ram_test_abraham()``, and ``bist_ram_test_abraham_full()``
4. All algorithms verify RAM patterns
5. Serial output captured and validated

**Expected Behavior:**

- March A, March X, and Abraham tests pass on real hardware
- No stuck-at, coupling, or address decoder faults detected
- RAM backup/restore prevents data loss during test

CI Test Results
^^^^^^^^^^^^^^^

**QEMU Test:**

.. xml-junit-test-results:: tests/ram_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml
    :title: RAM QEMU Test Results

**Device Test:**

.. xml-junit-test-results:: tests/ram_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml
    :title: RAM Device Test Results

Flash CRC Validation Test
-------------------------

**Purpose:** Verify that flash code and data sections have not been corrupted or tampered with since firmware build time.

QEMU/Emulation Validation
^^^^^^^^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/flash_test/pytest_qemu_flash_test.py``

**Execution:**

1. QEMU executes ``bist_flash_test()``
2. Test reads post-build injected CRC values from flash
3. Computes CRC32 over ``.flash.text`` section in chunks (1024 bytes default)
4. Compares computed CRC against stored value
5. Repeats for ``.flash.rodata`` section
6. Returns ``BIST_ESP_OK`` if both CRCs match

**Expected Output:**

.. code-block::

    test_BIST_flash:PASS

**Failure Test: CRC Length Corruption via GDB**

**GDB Fault Injection Script:**

.. code-block::

    # Connect to QEMU debug server
    target remote :1234

    # Set temporary breakpoint at CRC test data
    tb bist_flash_test_data
    continue

    # Corrupt section length to cause wrong CRC
    commands
        set crc_section_len=0xFF
        continue
    end

**Fault Injection Method:**

1. GDB sets breakpoint at CRC test initialization
2. When hit, modifies section length to incorrect value (0xFF)
3. Test computes CRC over wrong region
4. Computed CRC does not match stored value
5. Returns ``BIST_ESP_FLASH_TEST_ERR``

**Expected Output:**

.. code-block::

    test_BIST_flash:FAIL

Hardware Validation
^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/flash_test/pytest_device_flash_test.py``

**Expected Inputs/Environment:**

- Device with CRC values injected post-build
- Firmware unchanged since flashing
- No external flash modifications during test

**Execution:**

1. Flash firmware to device (with injected CRCs)
2. Run ``pytest pytest_device_flash_test.py``
3. Application runs ``bist_flash_test()``
4. CRCs computed and verified against stored values
5. Serial output captured and validated

**Expected Behavior:**

- Both ``.flash.text`` and ``.flash.rodata`` CRCs match on real hardware
- No flash corruption or tampering detected
- Test detects any flash section modifications

CI Test Results
^^^^^^^^^^^^^^^

**QEMU Test:**

.. xml-junit-test-results:: tests/flash_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml
    :title: Flash CRC QEMU Test Results

**Device Test:**

.. xml-junit-test-results:: tests/flash_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml
    :title: Flash CRC Device Test Results

Program Counter Integrity Test
------------------------------

**Purpose:** Verify that the program counter maintains integrity and that code execution occurs in expected memory regions (IRAM, Flash, RTC).

QEMU/Emulation Validation
^^^^^^^^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/pc_test/pytest_qemu_pc_test.py``

**Execution:**

1. QEMU executes ``bist_pc_test()``
2. Test calls four functions placed in different memory regions:
   - ``pcTestFunction0``
   - ``pcTestFunction1``
   - ``pcTestFunction2``
   - ``pcTestFunction3``
3. Each function returns its own address
4. Test verifies return addresses match expected function addresses
5. Returns ``BIST_ESP_OK`` if all addresses verified (covers 24 of 30 addressable PC bits)

**Expected Output:**

.. code-block::

    test_BIST_PC:PASS

**Failure Test 1: Function Pointer Bit-Flip (no WDT)**

This test simulates a single stuck-at fault on PC bit 2 by flipping that bit in
the first function pointer. The CPU jumps to an offset within the function body,
skipping the ``lui`` that loads the address constant, so the called code returns
a wrong value and the verification detects the mismatch — without crashing.

**GDB Fault Injection Script:**

.. code-block::

    # Connect to QEMU debug server
    target remote :1234

    # Break at the PC verification loop entry; $sp points to
    # the first element of the pcTestFunctions array on the stack.
    tb bist_verify_pc_test
    continue

    # Flip bit 2 of the first function pointer (stuck-at fault on PC bit 2).
    commands
        set *(int*)$sp = *(int*)$sp ^ 4
        continue
    end

**Failure Test 2: Function Pointer Corruption (WDT Reset)**

This test zeroes the first function pointer so that ``jalr`` jumps to address 0,
triggering an instruction access fault. The firmware cannot recover, so the
watchdog timer fires and resets the CPU (reset reason 7).

**GDB Fault Injection Script:**

.. code-block::

    # Connect to QEMU debug server
    target remote :1234

    # Break at the PC verification loop entry; $sp points to
    # the first element of the pcTestFunctions array on the stack.
    tb bist_verify_pc_test
    continue

    # Zero the first function pointer — causes an instruction access fault
    # that leads to a WDT reset.
    commands
        set *(int*)$sp = 0
        continue
    end

**Fault Injection Method:**

1. GDB sets a breakpoint at ``bist_verify_pc_test`` (the loop label)
2. At the breakpoint ``$sp`` points to the ``pcTestFunctions`` array on the stack
3. Test 1 flips bit 2 of the pointer — the CPU jumps to a valid but wrong address
   and the return-value comparison detects the mismatch (``BIST_ESP_PC_TEST_ERR``)
4. Test 2 zeroes the pointer — the CPU jumps to address 0, faults, and the WDT
   resets the system (reset reason 7)

**Expected Output:**

.. code-block::

    test_BIST_PC:FAIL

Hardware Validation
^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/pc_test/pytest_device_pc_test.py``

**Expected Inputs/Environment:**

- Device running with standard memory layout (IRAM, Flash, RTC)
- Functions placed in correct memory regions per linker script
- No code relocation or address space layout randomization (ASLR)

**Execution:**

1. Flash firmware to device
2. Run ``pytest pytest_device_pc_test.py``
3. Application runs ``bist_pc_test()``
4. Four function calls with return address verification
5. Serial output captured and validated

**Expected Behavior:**

- All four function addresses verified correctly on real hardware
- 24 PC bits (2-17, 19-21, 25, 28) exercised successfully
- No unexpected code execution or address corruption detected

CI Test Results
^^^^^^^^^^^^^^^

**QEMU Test:**

.. xml-junit-test-results:: tests/pc_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml
    :title: Program Counter QEMU Test Results

**Device Test:**

.. xml-junit-test-results:: tests/pc_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml
    :title: Program Counter Device Test Results

Interrupt Handling and Execution Test
-------------------------------------

**Purpose:** Verify interrupt-matrix routing and ISR delivery for software interrupt sources, and concurrent hardware interrupt delivery via dual timer-group alarms (IEC 60730 Table H.1 ID 2).

QEMU/Emulation Validation
^^^^^^^^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/interrupt_test/pytest_qemu_interrupt_test.py``

**Note:** QEMU validates the software interrupt source map path only. Timer Group (TIMG0/TIMG1) alarms used by ``bist_hardware_interrupt_test()`` are not supported in QEMU, so the hardware interrupt Unity test is not expected to PASS under emulation.

**Execution:**

1. QEMU executes ``bist_interrupt_source_map_test()``
2. Test configures CPU interrupt line 9 and maps each software source ``CPU_INTR_FROM_CPU_0`` through ``CPU_INTR_FROM_CPU_3``
3. Each source is triggered eight times; the ISR clears the request and increments a counter
4. Test verifies the enable mask and that the ISR count equals eight for each source
5. Returns ``BIST_ESP_OK`` and prints ``test_BIST_Interrupt_Source_Map:PASS``

**Expected Output:**

.. code-block::

    test_BIST_Interrupt_Source_Map:PASS

Hardware Validation
^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/interrupt_test/pytest_device_interrupt_test.py``

**Expected Inputs/Environment:**

- Device with interrupt matrix and two Timer Groups (``TIMG_LL_INST_NUM >= 2``)
- Free CPU interrupt lines 9 and 10 available for the test
- No conflicting handlers on ``CPU_INTR_FROM_CPU_0..3`` or the TIMG GPTimer alarm sources used by the test

**Execution:**

1. Flash firmware to device
2. Run ``pytest pytest_device_interrupt_test.py``
3. Application runs ``bist_interrupt_source_map_test()`` then ``bist_hardware_interrupt_test()``
4. Software path: map/trigger/unmap four software IRQ sources on CPU interrupt 9
5. Hardware path: start TIMG0 (500 µs) and TIMG1 (1000 µs) alarms on CPU interrupts 9 and 10; wait until both ISR counts reach 1000 while checking the 2:1 period ratio with ±1 count tolerance
6. Serial output captured and validated

**Expected Behavior:**

- ``test_BIST_Interrupt_Source_Map:PASS``
- ``test_BIST_Hardware_Interrupt:PASS``
- Incorrect routing, missed ISRs, or period-ratio failures return ``BIST_ESP_INTERRUPT_TEST_ERR``

**Expected Output:**

.. code-block::

    test_BIST_Interrupt_Source_Map:PASS
    test_BIST_Hardware_Interrupt:PASS

CI Test Results
^^^^^^^^^^^^^^^

**QEMU Test:**

.. xml-junit-test-results:: tests/interrupt_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml
    :title: Interrupt QEMU Test Results

**Device Test:**

.. xml-junit-test-results:: tests/interrupt_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml
    :title: Interrupt Device Test Results

Clock Integrity Test
--------------------

**Purpose:** Verify that external 32kHz and main 40MHz crystal oscillators are operating within specified tolerances.

QEMU/Emulation Validation
^^^^^^^^^^^^^^^^^^^^^^^^^^

**Note:** Clock tests are hardware-only. QEMU cannot accurately model crystal oscillators or frequency measurements.

Hardware Validation
^^^^^^^^^^^^^^^^^^^

**Test Scripts:**


**External 32kHz Crystal Test: ``test_BIST_ext_crystal_fail``**

**Execution:**

1. Flash firmware to device
2. Run ``pytest pytest_device_ext_crystal_test.py``
3. Application registers XT WDT callback via ``esp_xt_wdt_register_callback()``
4. Configure XT WDT with 200-cycle timeout (approximately 6 µs)
5. Test waits ~2 ms without crystal failure
6. If callback is not fired, returns ``BIST_ESP_OK`` (crystal operating)
7. If callback is fired, returns ``BIST_ESP_CLOCK_TEST_ERR`` (crystal failure detected)

**Fail Case:** Remove or disconnect 32kHz crystal → XT WDT fires → ``FAIL``

**Expected Output:**

.. code-block::

    test_BIST_ext_crystal_fail:PASS

**Main 40MHz Crystal Test: ``test_BIST_main_crystal``**

**Execution:**

1. Flash firmware to device
2. Run ``pytest pytest_device_main_crystal_test.py``
3. Get expected frequency via ``rtc_clk_xtal_freq_get()``
4. Measure frequency ratio between main and 32kHz reference via ``rtc_clk_cal_ratio()``
5. Calculate deviation via ``xtal_deviation_percent()`` (``bist_clock_math.h``): ``|measured - expected| / expected * 100``
6. Check if deviation exceeds ``CONFIG_ESP_BIST_CLOCK_PERCENT_FREQUENCY_DRIFT`` (default: ±1%)
7. Returns ``BIST_ESP_OK`` if within tolerance, ``BIST_ESP_CLOCK_TEST_ERR`` if drift exceeds threshold

**Fail Case:** Remove or significantly damage 40MHz crystal → large frequency deviation → ``FAIL``

**Expected Output:**

.. code-block::

    test_BIST_main_crystal:PASS

**Expected Behavior:**

- External 32kHz crystal monitored successfully
- Main crystal frequency within ±1% of expected value
- Any crystal failure or significant frequency drift detected and reported

.. _watchdog-operation-test:

Watchdog Operation Test
-----------------------

**Purpose:** Verify that the watchdog timer functions correctly to detect and recover from system faults.

QEMU/Emulation Validation
^^^^^^^^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/wdt_test/pytest_qemu_wdt_test.py``

**Success Test: ``test_wdt_success`` (Two-Boot Sequence)**

**First Boot:**

1. QEMU starts firmware first time
2. Application calls ``bist_wdt_test()``
3. Initialize MWDT with timeout (``CONFIG_ESP_BIST_WDT_TIMEOUT_US``, typically 10000 µs in test)
4. Delay for longer than timeout (e.g., 50000 µs)
5. Watchdog timeout expires → system reset

**Second Boot:**

1. QEMU restarts after WDT reset
2. Application checks reset reason via ``rom_get_reset_reason()``
3. Verify reset reason is ``RESET_REASON_CORE_MWDT0`` (MWDT0 reset)
4. Returns ``BIST_ESP_OK`` (watchdog working correctly)

.. note::
   ESP32-P4 reset reason is ``RESET_REASON_CORE_MWDT`` instead of ``RESET_REASON_CORE_MWDT0``.

**Expected Output:**

.. code-block::

    test_wdt_success:PASS

**Failure Test: ``test_wdt_error`` (No WDT Reset)**

**GDB Fault Injection Script:**

.. code-block::

    # Connect to QEMU debug server
    target remote :1234

    # Set breakpoint at WDT test timeout setup
    tb bist_test_wdt_timeout
    continue

    # Set timeout too long to trigger during test
    commands
        set wdt_timeout_us=10000
        continue
    end

**Fault Injection Method:**

1. GDB sets breakpoint at WDT timeout initialization
2. When hit, sets timeout to very large value (1 second)
3. Test delay (50 ms) completes before timeout expires
4. No WDT reset occurs
5. Returns ``BIST_ESP_WDT_TEST_ERR`` (watchdog failed)

**Expected Output:**

.. code-block::

    test_wdt_success:FAIL

Hardware Validation
^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/wdt_test/pytest_device_wdt_test.py``

**Expected Inputs/Environment:**

- Device with functional watchdog hardware
- Two-boot testing supported (persistent reset reason storage)
- Serial output captured across both boots

**Execution:**

1. Flash firmware to device
2. Run first iteration: ``pytest pytest_device_wdt_test.py``
3. First boot runs WDT test, triggers timeout and reset
4. Device restarts automatically
5. Second boot verifies reset reason and completes test
6. Serial output captured and validated

**Expected Behavior:**

- First boot: Watchdog timeout detected, system resets
- Second boot: Reset reason correctly identified as ``RESET_REASON_CORE_MWDT0`` (ESP32-P4: ``RESET_REASON_CORE_MWDT``)
- Test returns PASS on real hardware
- Two-boot sequence completes within test timeout window

CI Test Results
^^^^^^^^^^^^^^^

**QEMU Test:**

.. xml-junit-test-results:: tests/wdt_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml
    :title: Watchdog QEMU Test Results

**Device Test:**

.. xml-junit-test-results:: tests/wdt_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml
    :title: Watchdog Device Test Results

Windowed Watchdog Test
----------------------

**Purpose:** Verify that the windowed watchdog correctly enforces the feed window (minimum and maximum time between feeds) and detects underflow (feeding too early).

QEMU/Emulation Validation
^^^^^^^^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/windowed_wdt_test/pytest_qemu_windowed_wdt_test.py``

**Test Cases:**

1. **Normal operation:** Feed after underflow timeout plus margin; no underflow detected.
2. **Underflow detection:** Feed immediately after a previous feed (before underflow window); ``wdt_is_underflow_detected()`` returns true.
3. **Consecutive cycles:** Multiple feed cycles within the allowed window; all pass without underflow.

**Expected Output:**

.. code-block::

    test_BIST_WINDOWED_WDT_NORMAL:PASS
    test_BIST_WINDOWED_WDT_UNDERFLOW:PASS
    test_BIST_WINDOWED_WDT_CONSECUTIVE:PASS

Hardware Validation
^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/windowed_wdt_test/pytest_device_windowed_wdt_test.py``

**Execution:**

1. Flash firmware to device
2. Run ``pytest pytest_device_windowed_wdt_test.py``
3. Application runs windowed WDT tests: normal feed window, underflow detection, consecutive cycles
4. Serial output captured and validated

**Expected Behavior:**

- Normal feeds within the window pass; early feeds set underflow flag; consecutive valid feeds complete without error.

CI Test Results
^^^^^^^^^^^^^^^

**QEMU Test:**

.. xml-junit-test-results:: tests/windowed_wdt_test/build/tests/{IDF_TARGET_PATH_NAME}_qemu_report.xml
   :title: Windowed WDT QEMU Test Results

**Device Test:**

.. xml-junit-test-results:: tests/windowed_wdt_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml
   :title: Windowed WDT Device Test Results

esp_timer Test
--------------

**Purpose:** Verify that the high-resolution software timer (esp_timer) driver operates correctly for one-shot timers (creation, start, callback invocation).

**Test Application:** ``tests/esp_timer_test/``

**Execution:**

1. Build and run the esp_timer test application (QEMU or device).
2. Test ``test_BIST_ESP_TIMER_ONESHOT`` initializes esp_timer, creates a one-shot timer, starts it with a 10 ms timeout.
3. After waiting longer than the timeout, the test verifies the callback was invoked.
4. Returns PASS if callback executed as expected.

**Expected Output:**

.. code-block::

    test_BIST_ESP_TIMER_ONESHOT:PASS

**Note:** QEMU and device pytest scripts and CI jobs for esp_timer_test may be added in a future release. The test application can be run manually via ``ninja -C build qemu`` or ``ninja -C build flash`` and ``ninja -C build monitor``.

GPIO Plausibility Test
----------------------

**Purpose:** Verify that GPIO pins can be configured and that IO levels are readable and controllable.

QEMU/Emulation Validation
^^^^^^^^^^^^^^^^^^^^^^^^^^

**Note:** GPIO tests are hardware-only.

Hardware Validation
^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/digital_io_test/pytest_device_digital_io_test.py``

**Test Cases:**

1. **Invalid GPIO Test: ``test_BIST_IO_INVALID_GPIO``**

   **Execution:**

   - Attempt to configure invalid GPIO numbers (outside valid range)
   - Verify API validation catches invalid GPIO numbers
   - Returns ``BIST_ESP_IO_TEST_ERR`` for invalid GPIO

   **Expected Output:**

   .. code-block::

       test_BIST_IO_INVALID_GPIO:PASS

2. **GPIO Output Test: ``test_BIST_IO_OUTPUT_GPIO``**

   **Execution:**

   - Configure GPIO2 as output (GPIO_MODE_INPUT_OUTPUT)
   - Set level to 0, read back, verify 0
   - Set level to 1, read back, verify 1
   - Reset GPIO to default state
   - Returns ``BIST_ESP_OK`` if both levels verified

   **Expected Output:**

   .. code-block::

       test_BIST_IO_OUTPUT_GPIO:PASS

3. **GPIO Input Test: ``test_BIST_IO_INPUT_GPIO``**

   **Execution:**

   - Configure GPIO as input (GPIO_MODE_INPUT)
      - GPIO8 is used for C6, C5, C61 and P4
      - GPIO9 is used for C3 and H2
   - GPIO is tied to GND (expected level = 0)
   - Read GPIO level and verify it matches expected value
   - Reset GPIO to default state
   - Returns ``BIST_ESP_OK`` if level matches expected

   **Expected Output:**

   .. code-block::

       test_BIST_IO_INPUT_GPIO:PASS

**Complete Test Output:**

.. code-block::

    test_BIST_IO_INVALID_GPIO:PASS
    test_BIST_IO_OUTPUT_GPIO:PASS
    test_BIST_IO_INPUT_GPIO:PASS

**Expected Behavior:**

- Invalid GPIO numbers properly rejected
- GPIO2 output drive/read functionality confirmed
- GPIO input reads correct level (tied to GND)
- All GPIO configurations properly reset after test
- No GPIO configuration errors or stuck pins detected

CI Test Results
^^^^^^^^^^^^^^^

**Device Test:**

.. xml-junit-test-results:: tests/digital_io_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml
    :title: GPIO Device Test Results

ADC Plausibility Test
---------------------

**Purpose:** Verify that ADC channels can be configured and that analog readings respond plausibly to internal pull-up, pull-down, and (on ESP32-C3) VREF biasing.

QEMU/Emulation Validation
^^^^^^^^^^^^^^^^^^^^^^^^^^

**Note:** ADC tests are hardware-only.

Hardware Validation
^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/analog_io_test/pytest_device_analog_io_test.py``

**Test Cases:**

1. **Invalid ADC Test: ``test_BIST_ANALOG_IO_INVALID_ADC``**

   **Execution:**

   - Attempt to run ADC tests with invalid unit and channel numbers (outside valid range)
   - Verify API validation catches invalid parameters
   - Returns ``BIST_ESP_ADC_TEST_ERR`` for invalid unit/channel
   - On ESP32-C3, also validates ``bist_adc_reference_test`` with invalid parameters

   **Expected Output:**

   .. code-block::

       test_BIST_ANALOG_IO_INVALID_ADC:PASS

2. **ADC Low Level Test: ``test_BIST_ANALOG_IO_LOW_LEVEL``**

   **Execution:**

   - Configure ADC1 channel 2 with 12 dB attenuation
   - Enable internal pull-down on the mapped GPIO
   - Read raw ADC value after 10 ms settling time
   - Verify raw value is within ``CONFIG_ESP_BIST_ADC_PERCENT_DEVIATION`` of zero
   - Reset GPIO and delete ADC unit
   - Returns ``BIST_ESP_OK`` if reading is within tolerance

   **Expected Output:**

   .. code-block::

       test_BIST_ANALOG_IO_LOW_LEVEL:PASS

3. **ADC High Level Test: ``test_BIST_ANALOG_IO_HIGH_LEVEL``**

   **Execution:**

   - Configure ADC1 channel 2 with 12 dB attenuation
   - Enable internal pull-up on the mapped GPIO
   - Read raw ADC value after 10 ms settling time
   - Verify raw value is within tolerance of the SoC-specific high reference
   - Reset GPIO and delete ADC unit
   - Returns ``BIST_ESP_OK`` if reading is within tolerance

   **Expected Output:**

   .. code-block::

       test_BIST_ANALOG_IO_HIGH_LEVEL:PASS

4. **ADC Reference Test: ``test_BIST_ANALOG_IO_REFERENCE``** (ESP32-C3 only)

   **Execution:**

   - Configure ADC1 channel 2 with 12 dB attenuation
   - Enable internal VREF output to bias the pin near mid-scale
   - Read raw ADC value after 10 ms settling time
   - Verify raw value is within tolerance of the reference level
   - Reset GPIO and delete ADC unit
   - Returns ``BIST_ESP_OK`` if reading is within tolerance

   **Expected Output:**

   .. code-block::

       test_BIST_ANALOG_IO_REFERENCE:PASS

**Complete Test Output (ESP32-C3):**

.. code-block::

    test_BIST_ANALOG_IO_INVALID_ADC:PASS
    test_BIST_ANALOG_IO_LOW_LEVEL:PASS
    test_BIST_ANALOG_IO_HIGH_LEVEL:PASS
    test_BIST_ANALOG_IO_REFERENCE:PASS

**Complete Test Output (other supported SoCs):**

.. code-block::

    test_BIST_ANALOG_IO_INVALID_ADC:PASS
    test_BIST_ANALOG_IO_LOW_LEVEL:PASS
    test_BIST_ANALOG_IO_HIGH_LEVEL:PASS

**Expected Behavior:**

- Invalid ADC unit/channel numbers properly rejected
- ADC channel reads near zero with internal pull-down
- ADC channel reads near high reference with internal pull-up
- On ESP32-C3, VREF-biased reading is within mid-scale reference range
- All ADC and GPIO configurations properly reset after test
- No stuck-at faults or configuration errors detected

CI Test Results
^^^^^^^^^^^^^^^

**Device Test:**

.. xml-junit-test-results:: tests/analog_io_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml
    :title: ADC Device Test Results

Host Unit Tests
---------------

**Purpose:** Verify pure algorithmic helpers on the build machine, independently of target hardware or QEMU.

Host unit tests live in ``tests/unit/``. Each suite is a standalone CMake project built with the host C compiler and run under ``ctest``:

.. code-block:: bash

   cmake -S tests/unit/<suite> -B build/<suite>_test
   cmake --build build/<suite>_test
   ctest --test-dir build/<suite>_test --output-on-failure

Clock Math (``tests/unit/clock_math``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/unit/clock_math/test_bist_clock_math.c``

**Execution:**

1. Build and run on the host (no target or emulator required)
2. Exercises ``xtal_deviation_percent()`` (``bist_clock_math.h``) with exact match, ±1%, ±10%, sub-ppm drift, and ``expected == 0`` guard inputs
3. Validates the signed-subtraction path that previously relied on implementation-defined unsigned-wrap-through-``int`` conversion

**Rationale:** The device clock test (``tests/clock_test``) short-circuits on boards without a 32 kHz crystal because ``rtc_clk_cal`` returns 0 before the deviation math executes. This host test is the only regression guard for that arithmetic.

**Float-ABI caveat:** The host build uses hard-float (x86 SSE), while the device build uses soft-float on targets without an F extension (ESP32-C3/C6/H2). The chosen tolerances are loose enough to absorb the ABI difference.

**Expected Output:**

.. code-block::

    All bist_clock_math tests passed

For the Host Diagnostics protocol and companion unit tests (``tests/unit/hd_protocol``, ``tests/unit/hd_companion``), see :doc:`host_diagnostics`.

Validation Summary
------------------

- QEMU covers CPU regs, CSRs, stack, RAM, flash, PC, watchdog, windowed WDT, and esp_timer with deterministic fault injection where applicable.
- Hardware covers all modules; clock, GPIO, and ADC are hardware-only; watchdog requires two-boot sequence; windowed WDT and esp_timer have dedicated test applications.
- Host unit tests cover the clock deviation math helper (``xtal_deviation_percent``), the Host Diagnostics protocol encode/decode logic, and the LP companion state machine.
- Fault injections uniformly use temporary breakpoints and value corruption to assert FAIL paths.

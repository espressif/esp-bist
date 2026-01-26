Software Validation
===================

Validation Overview
-------------------

The ESP-BIST library is validated with two complementary approaches:

1. **QEMU-based emulation**: full system simulation with fault injection via GDB for deterministic, repeatable testing in CI pipelines

2. **Hardware testing**: On-device validation on actual {IDF_TARGET_NAME} hardware for real-world condition verification

Both approaches uses pytest and the Unity test framework for test assertions.

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

**Purpose:** Verify that CPU Control and Status Registers (MTVEC, MSCRATCH, MEPC, MCAUSE, MTVAL, PMPADDR0-PMPADDR15) maintain integrity and can be read/written reliably.

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

The script is executed 21 times, once for each CSR:

- Main CSRs: ``mtvec``, ``mscratch``, ``mepc``, ``mcause``, ``mtval``
- PMP Address CSRs: ``pmpaddr0``, ``pmpaddr1``, ``pmpaddr2``, ``pmpaddr3``, ``pmpaddr4``, ``pmpaddr5``, ``pmpaddr6``, ``pmpaddr7``, ``pmpaddr8``, ``pmpaddr9``, ``pmpaddr10``, ``pmpaddr11``, ``pmpaddr12``, ``pmpaddr13``, ``pmpaddr14``, ``pmpaddr15``

Each test replaces ``<csr_name>`` with the specific CSR name.

**Note:** GDB modifies the temporary register ``t0`` which is used by the test logic to verify CSR contents.

**Fault Injection Method:**

1. GDB sets breakpoint at CSR test label
2. When hit, modifies temporary register (t0) to incorrect value
3. Test logic uses t0 to verify CSR contents
4. Mismatch detected, test fails
5. Returns ``BIST_ESP_CPU_CSR_TEST_ERR``

**Test Coverage:** 21 test cases (5 main CSRs + 16 PMP address CSRs)

**Expected Output per CSR:**

.. code-block::

    test_BIST_Cpu_Csr_Regs:FAIL

Hardware Validation
^^^^^^^^^^^^^^^^^^^

**Test Script:** ``tests/cpu_reg_test/pytest_device_cpu_reg_test.py``

**Expected Inputs/Environment:**

- Device running with standard CSR configuration
- Machine mode privilege level (typical for bare-metal)
- PMP registers available for testing

**Execution:**

1. Flash firmware to device
2. Run ``pytest pytest_device_cpu_reg_test.py`` to capture serial output
3. Application runs ``bist_cpu_csr_regs_test()``
4. Test saves original CSR values, writes patterns, verifies, and restores

**Expected Behavior:**

- All 21 CSRs pass integrity checks on real hardware
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

1. QEMU runs ``bist_cpu_stack_overflow_test()``
2. Test initializes sentinel pattern (0xDEADBEEF) at stack bottom
3. Intentional deep recursion with 128-byte stack frames (count_max=20000)
4. Recursive calls force stack to grow downward and corrupt sentinel
5. Test detects sentinel corruption via ``bist_cpu_stack_overflow_check()``
6. Returns ``BIST_ESP_OK`` if overflow detected (stack protection working)

**Expected Output:**

.. code-block::

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

1. QEMU executes both RAM test algorithms over linker-defined RAM region
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
4. Backup/restore chunk buffer preserves RAM content on failure
5. Returns ``BIST_ESP_OK`` if all patterns verified

**Expected Output:**

.. code-block::

    test_BIST_ram_march_a:PASS
    test_BIST_ram_march_x:PASS

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
        set *start_addr=0xFF
        continue
    end

**Fault Injection Method:**

1. GDB sets breakpoint during RAM pattern verification
2. When hit, modifies a memory location to incorrect value
3. Test continues and reads memory, expecting original pattern
4. Mismatch detected, returns ``BIST_ESP_RAM_TEST_ERR``
5. Test prints "test_BIST_ram_march_x:FAIL"

**Test Coverage:** 2 test cases (March A and March X algorithms)

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
3. Application runs ``bist_ram_test_march_a()`` and ``bist_ram_test_march_x()``
4. Both algorithms verify all RAM patterns
5. Serial output captured and validated

**Expected Behavior:**

- Both March A and March X pass on real hardware
- No stuck-at faults or data path errors detected
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

**Failure Test 1: Return Address Modification (no WDT)**

**GDB Fault Injection Script:**

.. code-block::

    # Connect to QEMU debug server
    target remote :1234

    # Set breakpoint at PC verification
    tb bist_verify_pc_test
    continue

    # Watch return address and modify it
    commands
        watch returnFunctionAddress
        continue
        set returnFunctionAddress=pcTestFunction1
        disable breakpoints
        continue
    end

**Failure Test 2: Function Address Corruption (WDT Reset)**

**GDB Fault Injection Script:**

.. code-block::

    # Connect to QEMU debug server
    target remote :1234

    # Set breakpoint at PC test
    tb bist_verify_pc_test
    continue

    # Corrupt function pointer to cause invalid jump
    commands
        set pcTestFunctions[0]=(pcTestFunctions[0]-4)
        continue
    end

**Fault Injection Method:**

1. GDB sets breakpoint at function call or return verification
2. When hit, modifies return address or function pointer
3. Test executes with incorrect address
4. PC verification fails to match expected value
5. Returns ``BIST_ESP_PC_TEST_ERR``

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
5. Calculate deviation: ``|measured - expected| / expected * 100``
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
3. Initialize MWDT with timeout (``CONFIG_ESP_BIST_WDT_TIMEOUT_US``, typically 100 µs in test)
4. Delay for longer than timeout (e.g., 1000 µs)
5. Watchdog timeout expires → system reset

**Second Boot:**

1. QEMU restarts after WDT reset
2. Application checks reset reason via ``rom_get_reset_reason()``
3. Verify reset reason is ``RESET_REASON_CORE_MWDT0`` (MWDT0 reset)
4. Returns ``BIST_ESP_OK`` (watchdog working correctly)

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
2. When hit, sets timeout to very large value (10000 µs)
3. Test delay (1000 µs) completes before timeout expires
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
- Second boot: Reset reason correctly identified as ``RESET_REASON_CORE_MWDT0``
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

   - Configure GPIO9 as input (GPIO_MODE_INPUT)
   - GPIO9 is tied to GND (expected level = 0)
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
- GPIO9 input reads correct level (tied to GND)
- All GPIO configurations properly reset after test
- No GPIO configuration errors or stuck pins detected

CI Test Results
^^^^^^^^^^^^^^^

**Device Test:**

.. xml-junit-test-results:: tests/digital_io_test/build/tests/{IDF_TARGET_PATH_NAME}_device_report.xml
    :title: GPIO Device Test Results

Validation Summary
------------------

- QEMU covers CPU regs, CSRs, stack, RAM, flash, PC, watchdog with deterministic fault injection
- Hardware covers all modules; clock and GPIO are hardware-only; watchdog requires two-boot sequence
- Fault injections uniformly use temporary breakpoints and value corruption to assert FAIL paths

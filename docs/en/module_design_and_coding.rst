Module Design and Coding
========================

For each BIST test, this chapter captures implementation structure, defensive techniques, and interfaces.

.. _cpu-register-test:

CPU Register Test
-----------------

.. blockdiag::
    :scale: 50%
    :caption: CPU Register Test
    :align: center

    blockdiag {
        orientation = portrait;
        Start -> SaveOrig -> "Write 0xAAAAAAAA" -> "Read back" -> CheckA;
        CheckA -> Error [label = "No"];
        CheckA -> "Write 0x55555555" [label = "Yes"];
        "Write 0x55555555" -> "Read back 2" -> Check5;
        Check5 -> Error [label = "No"];
        Check5 -> Restore [label = "Yes"];
        Restore -> NextReg -> SaveOrig;
        SaveOrig -> Success[label = "End"];

        Start [label = "Start\nFor each register", shape = roundedbox];
        SaveOrig [label = "Save original\nif needed"];
        "Write 0xAAAAAAAA" [label = "Write\n0xAAAAAAAA"];
        "Read back" [label = "Read back\nvalue"];
        CheckA [label = "Value ==\n0xAAAAAAAA?", shape = diamond];
        Error [label = "Return\nBIST_ESP_CPU_TEST_ERR"];
        "Write 0x55555555" [label = "Write\n0x55555555"];
        "Read back 2" [label = "Read back\nvalue"];
        Check5 [label = "Value ==\n0x55555555?", shape = diamond];
        Restore [label = "Restore\noriginal"];
        NextReg [label = "Next register"];
        Success [label = "Return\nBIST_ESP_OK"];
    }

This diagram shows the generic logic for all tested CPU registers: save original value if needed, write test patterns (``0xAAAAAAAA`` and ``0x55555555``), verify, restore, and return error on any mismatch. On FPU supported devices, the same flow applies to FPU registers.

Module API
^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 30 15 15 20 20

   * - Function name
     - Exec Time
     - Cycles
     - Instruction Count
     - Code size (Bytes)
   * - ``bist_cpu_regs_test``
     - 24.8 us
     - 992
     - 362
     - 1346

Source Files
^^^^^^^^^^^^

.. list-table:: Source Files
   :header-rows: 1
   :widths: 30 20 50

   * - Source File
     - Version
     - MD5
   * - ``src/bist/core/cpu/bist_cpu_regs.c``
     - v1.0.0
     - 18ad6bcdaed51c19f26961fc067d57a0

Coding and Interfaces
^^^^^^^^^^^^^^^^^^^^^

- Macros enforce identical pattern/write/read/restore for each register
- Explicit error handling: If a register value does not match the expected pattern, the test jumps to a labeled error handler.
- No pointer dereferencing or dynamic memory is used, reducing risk of undefined behavior.
- Single public API declared in ``bist_cpu_regs.h``
- This test is self-contained and does not call other modules. It only depends on the BIST error code definitions and configuration macros.
- All operations are performed on CPU registers directly.
- The function is declared ``__attribute__((naked))`` and manages its own 16-byte stack frame explicitly (``addi sp, sp, -16`` at entry, ``addi sp, sp, 16`` at every return path). This is necessary because the function body is pure inline assembly that manually issues ``ret``; without ``naked``, the compiler may or may not generate a prologue depending on the optimization level (e.g. ``-Os`` omits it), which would cause a mismatch with the hand-written epilogue and corrupt the caller's stack.
- The main function has a single entry and, under normal conditions, a single exit. Error handling uses a label (``errorCPU``) for early exit on failure, which is documented and justified for low-level assembly.
- Branching is limited to error detection and is implemented via macro-generated assembly. There is no deep nesting or complex logic.
- No explicit C loops are used; the test iterates over registers via repeated macro invocations. All operations are statically bounded.
- Only bitwise and equality operations are performed on integer registers. For FPU register tests on FPU supported devices, no floating-point arithmetic is executed in C.
- No interrupts are used or manipulated by this test.
- No pointers are used in this test. All operations are on CPU registers.
- No recursion is used in this test.

CPU CSR Register Test
---------------------

.. blockdiag::
    :scale: 50%
    :caption: CPU CSR Register Test
    :align: center

    blockdiag {
        orientation = portrait;
        Start -> SaveOrig -> WriteA -> ReadA -> CheckA;
        CheckA -> Error [label = "No"];
        CheckA -> Write5 [label = "Yes"];
        Write5 -> Read5 -> Check5;
        Check5 -> Error [label = "No"];
        Check5 -> Restore [label = "Yes"];
        Restore -> Next -> SaveOrig;
        SaveOrig -> Success[label = "End"];

        Start [label = "Start\nFor each CSR", shape = roundedbox];
        SaveOrig [label = "Save original\nCSR value"];
        WriteA [label = "Write\n0xAAAAAAAA\n& mask"];
        ReadA [label = "Read back\nvalue & mask"];
        CheckA [label = "Value ==\n0xAAAAAAAA\n& mask?", shape = diamond];
        Error [label = "Return\nBIST_ESP_CPU_CSR_TEST_ERR"];
        Write5 [label = "Write\n0x55555555\n& mask"];
        Read5 [label = "Read back\nvalue & mask"];
        Check5 [label = "Value ==\n0x55555555\n& mask?", shape = diamond];
        Restore [label = "Restore\noriginal CSR"];
        Next [label = "Next CSR"];
        Success [label = "Return\nBIST_ESP_OK"];
    }

This diagram shows the generic logic for all tested CSRs: save original value, write test patterns (``0xAAAAAAAA`` and ``0x55555555`` masked as needed), verify, and restore. Any mismatch returns an error.

The CSR groups tested vary by SoC capability:

- **All SoCs**: Machine Trap CSRs — ``mtvec``, ``mscratch``, ``mepc``, ``mcause``, ``mtval``
- **All SoCs**: PMP — ``pmpaddr0``–``pmpaddr15`` (mask: ``0xFFFFFFFF`` on C3/C6/H2, ``0x3FFFFFE0`` on C5/C61/P4 due to 128-byte granularity), ``pmpcfg0``–``pmpcfg3`` (mask: ``0x1D1D1D1D`` on C3/C6/H2, ``0x0D0D0D0D`` on C5/C61/P4). The W bit (bit 1) is excluded because the ``0xAA`` test pattern sets R=0,W=1, a reserved RISC-V encoding that hardware WARL-clears. C5/C61/P4 additionally exclude A[1] (A=NA4 not selectable at G≥1).
- **SoCs with PMA** (C6, H2, C5, P4; guarded by ``SOC_CPU_HAS_PMA``): PMA address — ``pma_addr0``–``pma_addr11`` (CSRs ``0xBD0``–``0xBDB``, mask: ``0x3FFFFFE0``)
- **C5 only** (guarded by ``SOC_TARGET_ESP32C5``): ``mexstatus`` (CSR ``0x7E1``, mask: ``0x00102C00``), ``mhint`` (CSR ``0x7C5``, mask: ``0x00100000``). On C6/H2, ``mexstatus`` only exposes SOFT_RST bits (unsafe to test) and ``mhint`` does not exist.
- **FPU-capable SoCs** (H4, P4; guarded by ``ESP_BIST_USE_FPU``): ``fflags`` (mask ``0x1F``), ``frm`` (mask ``0x07``), ``fcsr`` (mask ``0xFF``)

PMA address entries 12–15 are skipped because the ROM bootloader may configure them as active regions whose NAPOT/TOR encoding forces the low-order address bits. PMA configuration registers (``pma_cfg``) are not tested because the ``PMA_L`` (Lock) bit is write-once.

Total CSRs tested: 25 on ESP32-C3, 37 on ESP32-C6/H2/C61, 39 on ESP32-C5, 40 on ESP32-H4/P4 (adds 12 ``pma_addr`` + ``fflags``, ``frm``, ``fcsr`` on FPU-capable SoCs).

Module API
^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 30 15 15 20 20

   * - Function name
     - Exec Time
     - Cycles
     - Instruction Count
     - Code size (Bytes)
   * - ``bist_cpu_csr_regs_test``
     - 26.15 us
     - 1046
     - 416
     - 1542

Source Files
^^^^^^^^^^^^

.. list-table:: Source Files
   :header-rows: 1
   :widths: 30 20 50

   * - Source File
     - Version
     - MD5
   * - ``src/bist/core/cpu/bist_cpu_csr_regs.c``
     - v1.0.0
     - 0d1ce5b4fef534283d438ad7c16f5c9d

Coding and Interfaces
^^^^^^^^^^^^^^^^^^^^^

- The CPU CSR test uses macros to ensure all test operations are performed in a controlled, repeatable way.
- Error code ``BIST_ESP_CPU_CSR_TEST_ERR`` on mismatch
- Single API in ``bist_cpu_csr_regs.h``
- This test is self-contained and does not call other modules. It only depends on the BIST error code definitions and configuration macros.
- No dynamic or static data structures are used. All operations are performed on CPU CSRs directly.
- The function is declared ``__attribute__((naked))`` and manages its own 16-byte stack frame explicitly, for the same reason as ``bist_cpu_regs_test`` (see above): the function body is pure inline assembly with manual ``ret``, so the compiler must not generate a prologue or epilogue.
- The main function has a single entry and, under normal conditions, a single exit. Error handling uses a label (``errorCSR``) for early exit on failure, which is documented and justified for low-level assembly.
- No explicit C loops are used; the test iterates over CSRs via repeated macro invocations. All operations are statically bounded.
- Only bitwise and equality operations are performed on integer CSRs. FPU CSR tests use ``csrr``/``csrw`` only; no floating-point arithmetic is executed in C.
- No interrupts are used or manipulated by this test.
- No recursion is used in this test.


Stack Overflow Test
-------------------

Runtime Check
^^^^^^^^^^^^^

.. blockdiag::
    :scale: 50%
    :caption: Stack Overflow Runtime Check
    :align: center

    blockdiag {
        orientation = portrait;
        Start -> ReadSentinel -> CheckPattern;
        CheckPattern -> HandleOverflow [label = "Corrupted"];
        CheckPattern -> ReturnOK [label = "Intact"];
        HandleOverflow -> ReturnOverflow;
        ReturnOK -> End;
        ReturnOverflow -> End;

        Start [label = "bist_cpu_stack_overflow_check()", shape = roundedbox];
        ReadSentinel [label = "Read *_stack_overflow_protection_start"];
        CheckPattern [label = "== 0xDEADBEEF?", shape = diamond];
        HandleOverflow [label = "handle_stack_overflow()"];
        ReturnOverflow [label = "BIST_ESP_STACK_TEST_OVERFLOW"];
        ReturnOK [label = "BIST_ESP_OK"];
        End [label = "End"];
    }

- ``bist_cpu_stack_overflow_init()`` writes sentinel pattern ``0xDEADBEEF`` to ``_stack_overflow_protection_start`` (linker-defined symbol at bottom of stack)
- ``bist_cpu_stack_overflow_check()`` reads sentinel and returns ``BIST_ESP_STACK_TEST_OVERFLOW`` if corrupted, ``BIST_ESP_OK`` otherwise
- Called periodically in main loop to detect stack overflow during normal operation

Stress Test
^^^^^^^^^^^

.. blockdiag::
    :scale: 50%
    :caption: Stack Overflow Stress Test
    :align: center

    blockdiag {
        orientation = portrait;
        Start -> InitTest -> WritePattern -> SetCount -> Loop;
        Loop -> Recurse -> AllocBuffer -> WriteBuffer -> RecurseCheck;
        RecurseCheck -> RecurseCall [label = "Yes"];
        RecurseCall -> Recurse;
        RecurseCheck -> ReturnRecurse [label = "No"];
        ReturnRecurse -> Check -> ReadSentinel -> CheckSentinel;
        CheckSentinel -> OverflowDetected [label = "No"];
        CheckSentinel -> NextIteration [label = "Yes"];
        NextIteration -> Loop;
        OverflowDetected -> Success;
        Loop -> NoOverflow;
        NoOverflow -> Failure;
        Success -> End;
        Failure -> End;

        Start [label = "Start", shape = roundedbox];
        InitTest [label = "Call bist_cpu_stack_overflow_init"];
        WritePattern [label = "Write 0xDEADBEEF to sentinel"];
        SetCount [label = "Set count_max = 20000"];
        Loop [label = "For k=0 to count_max", shape = diamond];
        Recurse [label = "Call bist_cpu_stack_recursive k"];
        AllocBuffer [label = "Allocate volatile char buffer 128 bytes"];
        WriteBuffer [label = "buffer0 = k"];
        RecurseCheck [label = "k > 0?", shape = diamond];
        RecurseCall [label = "Recursive call k-1"];
        ReturnRecurse [label = "Return"];
        Check [label = "Call bist_cpu_stack_overflow_check"];
        ReadSentinel [label = "Read sentinel"];
        CheckSentinel [label = "value == 0xDEADBEEF?", shape = diamond];
        OverflowDetected [label = "Overflow detected"];
        NextIteration [label = "k++"];
        NoOverflow [label = "Loop completed without overflow"];
        Success [label = "Return BIST_ESP_OK"];
        Failure [label = "Return BIST_ESP_STACK_TEST_ERR"];
        End [label = "End"];
    }

- ``bist_cpu_stack_overflow_test()`` intentionally causes stack overflow to verify detection works
- Uses bounded recursion (max 20000 iterations) with 128-byte local buffer per call
- Each recursive call consumes stack space; test checks sentinel after each iteration
- Returns ``BIST_ESP_OK`` if overflow detected (test passed), ``BIST_ESP_STACK_TEST_ERR`` if not (test failed)

High Watermark Calculation
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. blockdiag::
    :scale: 50%
    :caption: Stack High Watermark Calculation
    :align: center

    blockdiag {
        orientation = portrait;
        Start -> Init -> Loop;
        Loop -> Inc [label = "Yes"];
        Inc -> Loop;
        Loop -> Ret [label = "No"];
        Ret -> End;

        Start [label = "Start", shape = roundedbox];
        Init [label = "high_watermark=0 p=stack_bottom+1"];
        Loop [label = "*p == 0xBADC0FFE?", shape = diamond];
        Inc [label = "high_watermark++ p++"];
        Ret [label = "Return high_watermark * 4"];
        End [label = "End"];
    }

- ``bist_get_stack_high_watermark()`` scans stack from ``_stack_overflow_protection_start + 1`` to ``_stack_top``
- Counts consecutive words equal to fill pattern ``0xBADC0FFE`` (unused stack)
- Stops at first modified word (deepest stack usage point)
- Returns unused bytes: ``high_watermark * sizeof(uint32_t)``
- Used for runtime stack usage analysis and detection of stack pressure


Module API
^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 35 20 15 20 20

   * - Function name
     - Exec Time
     - Cycles
     - Instruction Count
     - Code size (Bytes)
   * - ``bist_cpu_stack_overflow_init``
     - 0.63 us
     - 25
     - 22
     - 42
   * - ``bist_cpu_stack_overflow_check``
     - 0.45 us
     - 18
     - 14
     - 56
   * - ``bist_cpu_stack_overflow_test``
     - 1.30 ms
     - 52031
     - 40440
     - 80
   * - ``bist_get_stack_high_watermark``
     - 15.85–575.55 us
     - 634–23022
     - 40–14978
     - 120

Source Files
^^^^^^^^^^^^

.. list-table:: Source Files
   :header-rows: 1
   :widths: 30 20 50

   * - Source File
     - Version
     - MD5
   * - ``src/bist/core/cpu/bist_cpu_stack.c``
     - v1.0.0
     - 88c79e6d96bbd53dc5c36fba82a30867

Coding and Interfaces
^^^^^^^^^^^^^^^^^^^^^

- The stack overflow test uses explicit pattern initialization and checking to detect stack overflows. The sentinel value ``0xDEADBEEF`` is written to a protected region at the bottom of the stack. The check function validates this value and calls a weak handler (``handle_stack_overflow``) if corruption is detected. All error paths return explicit error codes.
- Bounded recursion (max 20000) with 128-byte frames to force overflow during test
- High watermark scans fill pattern ``0xBADC0FFE`` to measure unused stack
- This test is self-contained but relies on linker-defined symbols for stack region boundaries.
- Uses linker symbols (``_stack_overflow_protection_start``) to locate the sentinel region.
- The sentinel is a single 32-bit value at a fixed address.
- No dynamic memory is used.
- All functions have a single entry and exit, except for the recursive test, which exits early if overflow is detected.
- Public APIs are declared in ``bist_cpu_stack.h`` and return explicit error codes. The handler is weak and can be overridden.
- Branching is limited to error detection and recursion depth checks. No deep nesting.
- The main test uses a bounded loop (``count_max = 20000``) and recursion depth is limited by this value. All loops are finite and predictable.
- Only integer comparisons and assignments are used. No floating-point or complex arithmetic.
- No interrupts are used or manipulated by this test.
- Pointers are only used to access the sentinel region via linker symbols. No pointer arithmetic beyond this.
- Recursion is used intentionally in ``bist_cpu_stack_recursive`` to stress the stack. The recursion depth is bounded and controlled.
- No goto or label-based jumps are used.

.. _ram-test:

RAM Test
--------

March A
^^^^^^^
.. blockdiag::
    :scale: 100%
    :caption: RAM March A Test
    :align: center

    blockdiag {
        orientation = portrait;
        Start -> Init -> ChunkLoop;
        ChunkLoop -> CalcSize -> Backup -> Step1 -> Step2 -> Step2Check;
        Step2Check -> Restore [label = "No"];
        Step2Check -> Step3 [label = "Yes"];
        Step3 -> Step3Check;
        Step3Check -> Restore [label = "No"];
        Step3Check -> Restore [label = "Yes"];
        Restore -> TestPassed;
        TestPassed -> Error [label = "No"];
        TestPassed -> NextChunk [label = "Yes"];
        NextChunk -> ChunkLoop;
        ChunkLoop -> Success;
        Error -> End;
        Success -> End;

        Start [label = "Start", shape = roundedbox];
        Init [label = "Get ram test\nstart and size"];
        ChunkLoop [label = "For each\nchunk", shape = diamond];
        CalcSize [label = "Calc chunk\nsize"];
        Backup [label = "Backup chunk"];
        Step1 [label = "Write 0x00000000\nascending"];
        Step2 [label = "Read 0, Write\n0xFFFFFFFF ascending"];
        Step2Check [label = "All reads\nzero?", shape = diamond];
        Restore [label = "Restore chunk"];
        Step3 [label = "Read 0xFFFFFFFF\nascending"];
        Step3Check [label = "All reads\n0xFFFFFFFF?", shape = diamond];
        TestPassed [label = "test passed?", shape = diamond];
        Error [label = "Return\nBIST_ESP_RAM_TEST_ERR"];
        NextChunk [label = "Next chunk"];
        Success [label = "Return\nBIST_ESP_OK"];
        End [label = "End"];
    }

- **Sequence:**

  1. Write 0 to all cells (ascending)
  2. Read 0, then write 1 to all cells (ascending)
  3. Read 1 from all cells (ascending)

- **Direction:** All steps are performed in ascending address order.

- **Fault Coverage:** Detects coupling and transition faults. Simpler and faster, but less comprehensive than March X.

- **Usage in ESP-BIST:** Used for a quick integrity check with lower runtime overhead.

March X
^^^^^^^
.. blockdiag::
    :scale: 50%
    :caption: RAM March X Test
    :align: center

    blockdiag {
        orientation = portrait;
        Start -> Init -> ChunkLoop;
        ChunkLoop -> CalcSize -> Backup -> Step1 -> Step2 -> Step2Check;
        Step2Check -> Restore [label = "No"];
        Step2Check -> Step3 [label = "Yes"];
        Step3 -> Step3Check;
        Step3Check -> Restore [label = "No"];
        Step3Check -> Step4 [label = "Yes"];
        Step4 -> Step4Check;
        Step4Check -> Restore [label = "No"];
        Step4Check -> Step5 [label = "Yes"];
        Step5 -> Step5Check;
        Step5Check -> Restore [label = "No"];
        Step5Check -> Restore [label = "Yes"];
        Restore -> TestPassed;
        TestPassed -> Error [label = "No"];
        TestPassed -> NextChunk [label = "Yes"];
        NextChunk -> ChunkLoop;
        ChunkLoop -> Success;
        Error -> End;
        Success -> End;

        Start [label = "Start", shape = roundedbox];
        Init [label = "Get ram test\nstart and size"];
        ChunkLoop [label = "For each\nchunk", shape = diamond];
        CalcSize [label = "Calc chunk\nsize"];
        Backup [label = "Backup chunk"];
        Step1 [label = "Write 0x00000000\nascending"];
        Step2 [label = "Read 0, Write\n0xFFFFFFFF ascending"];
        Step2Check [label = "All reads\nzero?", shape = diamond];
        Step3 [label = "Read 0xFFFFFFFF\nWrite 0 descending"];
        Step3Check [label = "All reads\n0xFFFFFFFF?", shape = diamond];
        Step4 [label = "Read 0, Write\n0xFFFFFFFF ascending"];
        Step4Check [label = "All reads\nzero?", shape = diamond];
        Step5 [label = "Read 0xFFFFFFFF\nWrite 0 descending"];
        Step5Check [label = "All reads\n0xFFFFFFFF?", shape = diamond];
        Restore [label = "Restore chunk"];
        TestPassed [label = "test passed?", shape = diamond];
        Error [label = "Return\nBIST_ESP_RAM_TEST_ERR"];
        NextChunk [label = "Next chunk"];
        Success [label = "Return\nBIST_ESP_OK"];
        End [label = "End"];
    }

- **Sequence:**

  1. Write 0 to all cells (ascending)
  2. Read 0, then write 1 to all cells (ascending)
  3. Read 1, then write 0 to all cells (descending)
  4. Read 0, then write 1 to all cells (ascending)
  5. Read 1, then write 0 to all cells (descending)

- **Direction:** Alternates between ascending and descending address order.

- **Fault Coverage:** Detects stuck-at, transition, and more coupling faults (between adjacent cells) than March A. More comprehensive, but takes longer to execute.

- **Usage in ESP-BIST:** Used for thorough RAM integrity validation.

Comparison of March A and March X
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**March A** and **March X** are both memory test algorithms used to detect stuck-at, transition, and coupling faults in RAM. Their main differences are in the sequence and direction of read/write operations, which affect fault coverage and test duration.

- March X provides higher fault coverage by alternating directions and including more read/write transitions, at the cost of increased test time.
- March A is a subset of March X, suitable for faster but less exhaustive checks.

Abraham (IEC 60730-1 H.2.19.1)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The Abraham algorithm implements the IEC 60730-1 Annex H H.2.19.1 variable-memory
test, providing Class C–level coverage for stuck-at faults (SAF), transition faults (TF),
coupling faults (CF), and address decoder faults (AF).

- **Sequence (10 elements, 30n operations):**

  | ``↕(w0)``
  | ``↓(r0,w1)  ↑(r1)``   — Sequence 1
  | ``↓(r1,w0)  ↑(r0)``   — Sequence 2
  | ``↑(r0,w1)  ↓(r1)``   — Sequence 3
  | ``↑(r1,w0)  ↓(r0)``   — Sequence 4
  | ``↓(r0,w1,w0)  ↑(r0)`` — Sequence 5
  | ``↑(r0,w1,w0)  ↑(r0)`` — Sequence 6
  | ``↕(w1)``
  | ``↑(r1,w0,w1)  ↑(r1)`` — Sequence 7
  | ``↓(r1,w0,w1)  ↑(r1)`` — Sequence 8

- **Word-Oriented Memory (WOM) variant:** Uses ``0x00000000`` (w0) and ``0xFFFFFFFF`` (w1)
  as data backgrounds for inter-word coupling detection.

- **Time-Division Partition Pairs:** The test region is divided into N partitions whose
  size is set by ``CONFIG_ESP_BIST_RAM_PARTITION_SIZE`` (default 512 words = 2 KiB).
  Each invocation of ``bist_ram_test_abraham()`` tests one pair ``(mi, mj)`` where
  ``i < j``. Over C(N,2) successive calls, all pairs are tested, providing equivalent
  coupling coverage to a full-region run without requiring a full-RAM backup buffer.
  Increasing the partition size reduces N and thus the quadratic pair count.

- **Backup model:** Two partitions are backed up simultaneously into a buffer of
  ``2 × CONFIG_ESP_BIST_RAM_PARTITION_SIZE`` words in ``.dram0.safe_ram``. On failure,
  both are restored before returning the error code.

- **Fault Coverage:** Detects SAF, TF, CF (including inter-partition coupling between
  the tested pair), and AF. More comprehensive than March X for coupling fault detection.

- **Usage in ESP-BIST:** Used for thorough post-boot RAM verification where Class C–level
  confidence is required. ``bist_ram_test_abraham_full()`` runs the complete pair schedule.
  ``bist_ram_test_abraham()`` tests one pair per call for WDT-friendly runtime usage.

Module API
^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 35 15 15 20 20

   * - Function name
     - Exec Time
     - Cycles
     - Instruction Count
     - Code size (Bytes)
   * - ``bist_ram_test_march_a``
     - 7.60 ms
     - 304178
     - 203954
     - 454
   * - ``bist_ram_test_march_x``
     - 11.44 ms
     - 457611
     - 313843
     - 664
   * - ``bist_ram_test_abraham``
     - 3.14 ms
     - 502704
     - 311512
     - 1320
   * - ``bist_ram_test_abraham_full``
     - 5260.72 ms
     - 841715631
     - 524484762
     - 1526

.. note::

   Abraham measurements were taken on ESP32-C3 at 160 MHz with
   ``CONFIG_ESP_BIST_RAM_PARTITION_SIZE`` set to 1024 words. Abraham metrics depend on
   the partition size: single-pair cost scales linearly with partition size (more cells
   per pair), while full-coverage cost depends quadratically on the number of partitions
   N = ceil(region / partition_size), since C(N,2) pairs must be tested. March A and
   March X are unaffected by partition size as they iterate the full region regardless.

Source Files
^^^^^^^^^^^^

.. list-table:: Source Files
   :header-rows: 1
   :widths: 30 20 50

   * - Source File
     - Version
     - MD5
   * - ``src/bist/core/memory/bist_ram.c``
     - v1.0.0
     - 83acc801a25f7fcea5942ba8a0129092

Stack-Pointer Relocation (Safe Stack)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Each SOC linker script defines the RAM test region via the symbol pair
``_bist_ram_test_start`` / ``_bist_ram_test_end``. The end symbol is aliased per
target to the top of the RAM region (``_dram0_end`` on ESP32-C3, ``_sram_end`` on
ESP32-C5/C6/C61/H2/P4 standalone builds, ``_image_ram_end`` on Zephyr, or
``__stack_top`` on IDF LP builds). Because that top-of-RAM address is also the
stack top, the test region includes the active call stack.

During the W0 pass the march algorithm writes zeros to every word in the current chunk;
if that chunk overlaps the function's own stack frame, local variables (``start_addr``,
loop counters, etc.) are corrupted, causing undefined behaviour whose symptom depends on
the compiler's stack-frame layout — different GCC versions place variables at different
offsets, so the defect can manifest as a crash on one toolchain and a silent early loop
exit on another.

The region size is computed at run time as
``(&_bist_ram_test_end - &_bist_ram_test_start) / 4`` rather than read from an
absolute linker symbol. Materialising an absolute value
forces a GP-relative relocation on RV32, which is limited to ±2 KiB; a large
under-test region overflows that range. Subtracting two address-taken boundary
symbols avoids the relocation entirely.

To eliminate this class of failure the public entry points ``bist_ram_test_march_a()``,
``bist_ram_test_march_x()``, and ``bist_ram_test_abraham()`` relocate the stack pointer
into a 256-byte buffer (``ram_test_stack``) placed in ``.dram0.safe_ram`` — the same
linker section that holds ``backup_chunk``.  Because ``.dram0.safe_ram`` sits **below**
``_bist_ram_test_start``,
the march algorithm never writes to the relocated stack, and the full linker-defined
region — including the normal stack — is tested.

The relocation is performed by ``run_on_safe_stack()``, a thin wrapper that:

1. Saves the original SP and RA on the safe stack using inline assembly.
2. Switches SP to the top of ``ram_test_stack``.
3. Calls the march implementation via an indirect ``jalr``.
4. Restores the original SP after the implementation returns.

Callers' stack frames (``main()``, Unity runner, etc.) **are** inside the test region,
but the chunk-based backup/restore cycle saves them to ``backup_chunk`` before each
destructive pass and restores them afterwards, so they are intact when control returns.

Coding and Interfaces
^^^^^^^^^^^^^^^^^^^^^

- The RAM test uses explicit backup and restore of memory regions to prevent data loss during testing. If a test fails, the original memory contents are restored before returning an error.
- Explicit error codes; bounded loops sized by region and chunk size
- No dynamic memory; statically allocated backup buffer and safe stack
- This test uses linker-defined symbols for the region boundaries
  (``_bist_ram_test_start`` / ``_bist_ram_test_end``) and computes the size from
  their difference at run time.
- Uses a statically allocated backup buffer (``backup_chunk``) and a 256-byte safe stack (``ram_test_stack``), both placed in a dedicated ``.dram0.safe_ram`` section that is excluded from the RAM test region.
- The stack pointer is temporarily relocated to ``ram_test_stack`` via inline assembly before calling the march implementation, ensuring the march algorithm can test the entire linker-defined region — including the normal stack — without corrupting its own frame. See *Stack-Pointer Relocation* above.
- No dynamic memory is used.
- The main function has a single entry and, under normal conditions, a single exit. The use of ``goto`` for cleanup is documented and justified for resource safety.
- Branching is limited to error detection and cleanup. No deep nesting or complex logic.
- Loops are bounded by the size of the RAM region and the backup chunk size. All loops are finite and predictable.
- Only integer comparisons and assignments are used. No floating-point or complex arithmetic.
- No interrupts are used or manipulated by this test.
- Pointers are used to access RAM regions and the backup buffer. All pointer arithmetic is explicit and bounds-checked.
- No recursion is used in this test.
- Division is only used to convert the boundary difference
  (``_bist_ram_test_end - _bist_ram_test_start``) from byte size to word count;
  the difference is guaranteed to be nonzero and 4-byte aligned by the linker.
- Inline assembly is used in ``run_on_safe_stack()`` to save/restore the stack pointer; the clobber list is explicit and covers all caller-saved registers.

.. _flash-test:

Flash CRC Test
--------------

.. blockdiag::
    :scale: 50%
    :caption: Flash CRC Test
    :align: center

    blockdiag {
        orientation = portrait;
        Start -> GetSymbols -> GetStoredCRC -> CalcRODataSize -> CalcRODataCRC -> CompareROData;
        CompareROData -> ErrorROData [label = "No"];
        CompareROData -> CalcTextSize [label = "Yes"];
        CalcTextSize -> CalcTextCRC -> CompareText;
        CompareText -> ErrorText [label = "No"];
        CompareText -> Success [label = "Yes"];
        ErrorROData -> Error;
        ErrorText -> Error;
        Success -> End;
        Error -> End;

        Start [label = "Start", shape = roundedbox];
        GetSymbols [label = "Get linker symbols"];
        GetStoredCRC [label = "Get stored CRCs"];
        CalcRODataSize [label = "Calc rodata size"];
        CalcRODataCRC [label = "CRC32 for flash.rodata"];
        CompareROData [label = "crc_data == stored rodata CRC?", shape = diamond];
        ErrorROData [label = "Log error: rodata CRC mismatch"];
        CalcTextSize [label = "Calc text size"];
        CalcTextCRC [label = "CRC32 for flash.text"];
        CompareText [label = "crc_text == stored text CRC?", shape = diamond];
        ErrorText [label = "Log error: text CRC mismatch"];
        Success [label = "Return BIST_ESP_OK"];
        Error [label = "Return BIST_ESP_FLASH_TEST_ERR"];
        End [label = "End"];
    }

This diagram shows the concrete implementation of flash CRC validation. The test reads ``.flash.rodata`` and ``.flash.text`` sections from linker-defined symbols, computes CRC32 in configurable chunk sizes, and compares against stored checksums injected post-build by ``scripts/calculate_crc32.py``.

Module API
^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 35 15 15 20 20

   * - Function name
     - Exec Time
     - Cycles
     - Instruction Count
     - Code size (Bytes)
   * - ``bist_flash_test``
     - 4.72 ms
     - 188671
     - 121249
     - 604

In this analysis CONFIG_BIST_FLASH_TEST_CHUNK_SIZE was set to 1024 bytes.

Source Files
^^^^^^^^^^^^

.. list-table:: Source Files
   :header-rows: 1
   :widths: 30 20 50

   * - Source File
     - Version
     - MD5
   * - ``src/bist/core/memory/bist_flash.c``
     - v1.0.0
     - 64feb9abb885ad1343df81b30117c58e

Coding and Interfaces
^^^^^^^^^^^^^^^^^^^^^

- The Flash CRC test uses explicit error handling: if CRC calculation or flash region access fails, the function returns an explicit error code.
- Stored CRCs injected post-build by ``scripts/calculate_crc32.py``
- This test uses linker-defined symbols for flash region boundaries and reference CRC values.
- Uses statically defined CRC32 table and linker symbols for flash regions.
- No dynamic memory is used.
- All buffers are stack-allocated or provided by the linker.
- The main function has a single entry and, under normal conditions, a single exit. Early returns are used for error handling.
- Branching is limited to error detection and chunk processing. No deep nesting or complex logic.
- Loops are bounded by the size of the flash region and the chunk size. All loops are finite and predictable.
- Only integer comparisons, CRC32 table lookups, and assignments are used. No floating-point or complex arithmetic.
- No interrupts are used or manipulated by this test.
- Pointers are used to access flash regions and buffers. All pointer arithmetic is explicit and bounds-checked.
- No recursion is used in this test.
- No goto or label-based jumps are used.
- Type conversions are only used for linker symbol addresses, which is standard and explicit.
- Division is only used to calculate chunk sizes and is always bounded by the region size.

Flash CRC Calculation Script
----------------------------

The ESP-BIST system uses a post-build script, ``scripts/calculate_crc32.py``, to compute and inject reference CRC32 checksums for the ``.flash.text`` and ``.flash.rodata`` sections of the firmware image. These pre-calculated checksums are stored in dedicated ELF sections and serve as reference values for runtime flash integrity validation performed by ``bist_flash_test()``.

Execution Flow
^^^^^^^^^^^^^^

The script executes the following steps when invoked:

1. **Extract source section**: Dumps the source ELF section (``.flash.text`` or ``.flash.rodata``) to a temporary binary file using ``objcopy --dump-section``.

2. **Calculate CRC32**: Reads the extracted binary data in 4KB chunks and calculates the CRC32 checksum using the CRC-32/ISO-HDLC algorithm (polynomial 0x04C11DB7, IEEE 802.3 standard) with a 256-entry lookup table. This is the same algorithm implemented in the runtime code (``src/bist/core/memory/bist_flash.c``), ensuring identical checksum computation.

3. **Create CRC binary**: Writes the calculated CRC32 value as a 4-byte little-endian binary file (``crc_value.bin``).

4. **Inject into ELF**: Updates the destination section (``.crc_section_text`` or ``.crc_section_data``) in the ELF file using ``objcopy --update-section``.

5. **Verify injection**: Reads back the injected section and compares it with the calculated value to ensure successful injection.

6. **Cleanup**: Removes all temporary files and exits with an error code if verification fails.

Build System Integration
^^^^^^^^^^^^^^^^^^^^^^^^^

The script is automatically invoked twice as POST_BUILD custom commands in ``cmake/project.cmake``, after the ELF executable is generated but before the flash binary is created:

.. code-block:: cmake

   # Calculate CRC32 for .flash.text section
   add_custom_command(TARGET ${APP_EXECUTABLE} POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E env OBJCOPY=${CMAKE_OBJCOPY}
       python ${BIST_ROOT_DIR}/scripts/calculate_crc32.py
       ${APP_EXECUTABLE} .flash.text .crc_section_text
   )

   # Calculate CRC32 for .flash.rodata section
   add_custom_command(TARGET ${APP_EXECUTABLE} POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E env OBJCOPY=${CMAKE_OBJCOPY}
       python ${BIST_ROOT_DIR}/scripts/calculate_crc32.py
       ${APP_EXECUTABLE} .flash.rodata .crc_section_data
   )

Linker Script Integration
^^^^^^^^^^^^^^^^^^^^^^^^^^

The linker script (``src/soc/{IDF_TARGET_PATH_NAME}/ld/linker.ld``) defines two dedicated sections for storing the injected CRC32 values:

- **``.crc_section_text``**: Stores the CRC32 checksum of the ``.flash.text`` section.

- **``.crc_section_data``**: Stores the CRC32 checksum of the ``.flash.rodata`` section.

Both sections are placed in the ``drom0_0_seg`` (DROM) memory region and are mapped to flash memory. Linker symbols ``_crc_section_text_start`` and ``_crc_section_data_start`` provide the addresses where the CRC32 values are stored.

Runtime Flash Test Integration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The runtime flash test (``src/bist/core/memory/bist_flash.c``) uses the injected CRC32 values as follows:

1. **Read reference CRCs**: Accesses the pre-calculated CRC32 values via linker symbols ``_crc_section_text_start`` and ``_crc_section_data_start``.

2. **Recompute CRCs**: Calls ``calculate_crc32()`` to recompute the CRC32 checksums of the ``.flash.text`` and ``.flash.rodata`` sections at runtime, using the same algorithm and CRC32 table as the build script.

3. **Compare values**: Compares the recomputed CRC32 values against the reference values stored in flash. If they match, flash integrity is confirmed; otherwise, ``BIST_ESP_FLASH_TEST_ERR`` is returned.

The runtime implementation processes flash sections in configurable chunks (default: ``CONFIG_BIST_FLASH_TEST_CHUNK_SIZE`` bytes) to handle large sections efficiently while maintaining bounded stack usage.

Safety Properties
^^^^^^^^^^^^^^^^^^

- **Algorithm consistency**: The CRC32 table and calculation algorithm are identical in both the build script and runtime code, guaranteeing consistent checksum computation.

- **Memory isolation**: The destination sections (``.crc_section_text`` and ``.crc_section_data``) are placed at addresses that do not overlap with the sections being checksummed, preventing circular dependencies.

- **Build-time verification**: The script verifies successful CRC injection before completing, causing build failures if injection fails.

- **Traceability**: The exact CRC32 values are recorded in the ELF file and can be inspected via linker symbols, supporting safety certification evidence.

This mechanism ensures that the flash integrity check in ESP-BIST validates the image exactly as it was built, supporting safety certification traceability and IEC 60730 compliance.

.. _program-counter-test:

Program Counter Test
--------------------

.. blockdiag::
    :scale: 50%
    :caption: Program Counter Test
    :align: center

    blockdiag {
        orientation = portrait;
        Start -> Init -> Loop;
        Loop -> Call0 -> Check0;
        Check0 -> Error [label = "No"];
        Check0 -> Call1 [label = "Yes"];
        Call1 -> Check1;
        Check1 -> Error [label = "No"];
        Check1 -> Call2 [label = "Yes"];
        Call2 -> Check2;
        Check2 -> Error [label = "No"];
        Check2 -> Call3 [label = "Yes"];
        Call3 -> Check3;
        Check3 -> Error [label = "No"];
        Check3 -> Call4 [label = "Yes"];
        Call4 -> Check4;
        Check4 -> Error [label = "No"];
        Check4 -> Success [label = "Yes"];
        Success -> End;
        Error -> End;

        Start [label = "Start", shape = roundedbox];
        Init [label = "Init array of 5 PC test functions"];
        Loop [label = "countPcTest=0..4", shape = diamond];
        Call0 [label = "Call pcTestFunction0"];
        Check0 [label = "Return == func0?", shape = diamond];
        Call1 [label = "Call pcTestFunction1"];
        Check1 [label = "Return == func1?", shape = diamond];
        Call2 [label = "Call pcTestFunction2"];
        Check2 [label = "Return == func2?", shape = diamond];
        Call3 [label = "Call pcTestFunction3"];
        Check3 [label = "Return == func3?", shape = diamond];
        Call4 [label = "Call pcTestFunction4"];
        Check4 [label = "Return == func4?", shape = diamond];
        Error [label = "Return BIST_ESP_PC_TEST_ERR"];
        Success [label = "Return BIST_ESP_OK"];
        End [label = "End"];
    }

This diagram shows the concrete implementation of the program counter integrity test. Each function is placed in a specific memory region by the linker script to exercise different PC register bits: ``pc_test_0`` and ``pc_test_1`` in IRAM (bits [2:17]), ``pc_test_2`` in Flash (bits 19/20/21/25), ``pc_test_3`` in RTC memory (bit 28), and ``pc_test_4`` in TCM (bits [2:7], ESP32-P4 only). The test verifies that each function returns its own address, detecting stuck-at faults in the PC register.

Module API
^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 35 15 15 20 20

   * - Function name
     - Exec Time
     - Cycles
     - Instruction Count
     - Code size (Bytes)
   * - ``bist_pc_test``
     - 38.60 us
     - 1544
     - 160
     - 160

Source Files
^^^^^^^^^^^^

.. list-table:: Source Files
   :header-rows: 1
   :widths: 30 20 50

   * - Source File
     - Version
     - MD5
   * - ``src/bist/core/cpu/bist_pc.c``
     - v1.0.0
     - 1f25ac855582fa7a07f51ef74a638f28

Coding and Interfaces
^^^^^^^^^^^^^^^^^^^^^

- The PC test uses explicit function placement in different memory regions to exercise specific PC bits. Error handling is explicit: if the return address does not match the expected function pointer, the function returns an explicit error code.
- This test uses linker-defined sections for function placement.
- Uses an array of function pointers to the test functions.
- No dynamic memory is used.
- The main function has a single entry and, under normal conditions, a single exit. Early returns are used for error handling.
- Branching is limited to error detection and function pointer checks. No deep nesting or complex logic.
- Loops are bounded by the number of test functions. All loops are finite and predictable.
- Only pointer comparisons and assignments are used. No floating-point or complex arithmetic.
- No interrupts are used or manipulated by this test.
- Pointers are used for function pointers only. All pointer usage is explicit and safe.
- No recursion is used in this test.
- No goto or label-based jumps are used.
- Type conversions are only used for function pointer casting, which is standard and explicit.
- No division operations are present in this test.

.. _interrupt-test:

Interrupt Handling and Execution Test
-------------------------------------

Software Interrupt Source Map Test
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. blockdiag::
    :scale: 60%
    :caption: Software Interrupt Source Map Test
    :align: center

    blockdiag {
        orientation = portrait;
        span_height = 40;
        node_width = 220;
        node_height = 55;
        default_fontsize = 11;

        Start -> Configure -> SrcLoop;
        SrcLoop -> Map [label = "Next"];
        Map -> Enable -> Trigger -> CheckEn;
        CheckEn -> Fail [label = "No"];
        CheckEn -> Teardown [label = "Yes"];
        Teardown -> CheckOk;
        CheckOk -> Fail [label = "No"];
        CheckOk -> SrcLoop [label = "Yes"];
        SrcLoop -> Success [label = "Done"];
        Fail -> End;
        Success -> End;

        Start [label = "Start", shape = roundedbox];
        Configure [label = "Set type/priority\nCPU intr 9"];
        SrcLoop [label = "For each SW source\nCPU_INTR_FROM_CPU_0..3", shape = diamond];
        Map [label = "Route source to\nCPU intr 9"];
        Enable [label = "Enable ISR"];
        Trigger [label = "Trigger interrupt source\nN times"];
        CheckEn [label = "Enable mask\nbit set?", shape = diamond];
        Teardown [label = "Disable and unmap\nCPU intr 9"];
        CheckOk [label = "Mask clear and\nISR count == N?", shape = diamond];
        Fail [label = "Return\nBIST_ESP_INTERRUPT_TEST_ERR"];
        Success [label = "Return BIST_ESP_OK"];
        End [label = "End", shape = roundedbox];
    }

.. blockdiag::
    :scale: 60%
    :caption: Software Interrupt IRQ Callback
    :align: center

    blockdiag {
        orientation = portrait;
        span_height = 40;
        node_width = 220;
        node_height = 55;
        default_fontsize = 11;

        HwAssert -> IrqClear -> IrqCount -> IrqRet;

        HwAssert [label = "IRQ callback", shape = roundedbox];
        IrqClear [label = "Clear interrupt bit"];
        IrqCount [label = "Increment ISR count"];
        IrqRet [label = "Return from ISR", shape = roundedbox];
    }

``bist_interrupt_source_map_test()`` exercises interrupt-matrix routing and ISR delivery using the four SoC software interrupt sources ``CPU_INTR_FROM_CPU_0`` through ``CPU_INTR_FROM_CPU_3``. These are dedicated software-triggered interrupt sources (not peripheral IRQs): software raises each by writing its ``CPU_INTR_FROM_CPU_n`` register, which asserts that interrupt source into the interrupt matrix. Each source is routed to free CPU interrupt line 9, triggered eight times, then unmapped. The test fails if the enable mask is wrong or the ISR count is not exactly eight. The IRQ callback runs asynchronously when hardware asserts the source; it clears the interrupt request and increments the ISR count used by the test.

Hardware Interrupt Test
^^^^^^^^^^^^^^^^^^^^^^^

.. blockdiag::
    :scale: 60%
    :caption: Hardware Interrupt Test
    :align: center

    blockdiag {
        orientation = portrait;
        span_height = 45;
        node_width = 240;
        node_height = 55;
        default_fontsize = 11;

        Start -> Init -> Route0 -> Route1 -> Poll;
        Poll -> RatioOk;
        RatioOk -> MarkFail [label = "No"];
        RatioOk -> CountsDone [label = "Yes"];
        CountsDone -> Poll [label = "No"];
        CountsDone -> Deinit [label = "Yes"];
        MarkFail -> Deinit;
        Deinit -> Failed;
        Failed -> Err [label = "No"];
        Failed -> Ok [label = "Yes"];
        Err -> End;
        Ok -> End;

        Start [label = "Start", shape = roundedbox];
        Init [label = "Init TIMG0/TIMG1\n500 us / 1000 us"];
        Route0 [label = "Route TIMG0 to\nCPU intr 9"];
        Route1 [label = "Route TIMG1 to\nCPU intr 10"];
        Poll [label = "Delay and sample\nISR counts"];
        RatioOk [label = "count1 ~= 2*count2\n(tolerance ±1)?", shape = diamond];
        MarkFail [label = "Mark test failed\n(ratio error)"];
        CountsDone [label = "Both counts\n>= 1000?", shape = diamond];
        Deinit [label = "Stop timers\nunmap interrupts"];
        Failed [label = "Counters within\ntolerance?", shape = diamond];
        Err [label = "Return\nBIST_ESP_INTERRUPT_TEST_ERR"];
        Ok [label = "Return BIST_ESP_OK"];
        End [label = "End", shape = roundedbox];
    }

``bist_hardware_interrupt_test()`` starts GPTimer alarms on TIMG0 (500 µs → CPU interrupt 9) and TIMG1 (1000 µs → CPU interrupt 10). While both ISR counts are below 1000, the ISR path periodically checks that ``|count1 - 2*count2| ≤ 1``. The allowed difference of one count is a tolerance for possible synchronization skew between the two independent timer interrupts (for example, sampling the counters while one ISR has run and the other has not yet). Timers and matrix routes are torn down before returning ``BIST_ESP_OK`` or ``BIST_ESP_INTERRUPT_TEST_ERR``. Requires two timer groups (``TIMG_LL_INST_NUM >= 2``).

Module API
^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 45 15 15 20 20

   * - Function name
     - Exec Time
     - Cycles
     - Instruction Count
     - Code size (Bytes)
   * - ``bist_interrupt_source_map_test``
     - 44.6 ms
     - 7140244
     - 2565393
     - 528
   * - ``bist_hardware_interrupt_test``
     - 500 ms
     - 80001979
     - 39957618
     - 1014

.. note::

   Interrupt test measurements were taken on ESP32-C3 at 160 MHz.
   The hardware interrupt test takes a longer time due to expecting 1000 interrupts.

Source Files
^^^^^^^^^^^^

.. list-table:: Source Files
   :header-rows: 1
   :widths: 45 15 40

   * - Source File
     - Version
     - MD5
   * - ``src/bist/core/interrupt/bist_interrupt_sw.c``
     - v1.0.0
     - 12e7e301dad87b8990c57927ac906366
   * - ``src/bist/core/interrupt/bist_interrupt.c``
     - v1.0.0
     - f9dd6bd7a1530a88757e927e3d426c5b

Coding and Interfaces
^^^^^^^^^^^^^^^^^^^^^

- Explicit error handling: any enable-mask, ISR-count, or period-ratio failure returns ``BIST_ESP_INTERRUPT_TEST_ERR``.
- Uses ``esp_cpu_intr_*`` and ``esp_rom_route_intr_matrix`` for CPU interrupt control and interrupt-matrix routing; hardware path also uses TIMG HAL/LL (``timer_hal``, ``timer_ll``, ``timg_ll``).
- Static state holds ISR counters and timer contexts; no dynamic memory allocation.
- Public APIs have a single entry; early returns are used for error paths. Hardware path always deinitializes timers before exit.
- Branching is limited to setup checks, enable-mask checks, count checks, and the period-ratio check. No deep nesting.
- Only integer comparisons and assignments are used for pass/fail decisions.
- Interrupts are intentionally installed, enabled, and torn down by this test; CPU lines 9 and 10 are used as free test lines.
- Pointers are used for ISR arguments (source register or timer context). Usage is explicit and bounded to the test lifetime.
- No recursion is used.
- No goto or label-based jumps are used.
- No division operations are present in the pass/fail logic (ratio check uses multiply-by-two and absolute difference).

.. _clock-test:

Clock Tests
-----------

External 32kHz Crystal
^^^^^^^^^^^^^^^^^^^^^^
.. blockdiag::
    :scale: 50%
    :caption: External 32kHz Crystal Test
    :align: center

    blockdiag {
        orientation = portrait;
        Start -> Register -> Configure -> Init -> InitCheck;
        InitCheck -> Error [label = "No"];
        InitCheck -> Delay [label = "Yes"];
        Delay -> CheckFailed;
        CheckFailed -> Error [label = "Yes"];
        CheckFailed -> Success [label = "No"];

        Start [label = "Start", shape = roundedbox];
        Register [label = "Register XT WDT callback"];
        Configure [label = "Timeout=200 cycles no auto backup"];
        Init [label = "esp_xt_wdt_init"];
        InitCheck [label = "Init OK?", shape = diamond];
        Error [label = "Return BIST_ESP_CLOCK_TEST_ERR"];
        Delay [label = "Wait ~2 ms"];
        CheckFailed [label = "Callback fired?", shape = diamond];
        Success [label = "Return BIST_ESP_OK"];
    }

**External 32kHz Crystal Test:** On SoCs with ``SOC_XT_WDT_SUPPORTED``, uses XT WDT with 200-cycle timeout to detect 32kHz oscillator failure. Waits 2 ms and checks if the callback was triggered (indicating clock failure). If the callback fires during this wait period, it indicates the crystal has failed and the test returns an error. On SoCs without XT WDT hardware, this test is skipped and returns ``BIST_ESP_OK``.

Main 40MHz Crystal
^^^^^^^^^^^^^^^^^^
.. blockdiag::
    :scale: 50%
    :caption: Main 40MHz Crystal Test
    :align: center

    blockdiag {
        orientation = portrait;
        Start -> GetExpected -> CalcRatio -> CheckRatio;
        CheckRatio -> ErrorRatio [label = "Yes"];
        CheckRatio -> CalcXtal [label = "No"];
        CalcXtal -> Deviation -> CheckDev;
        CheckDev -> ErrorDev [label = "Yes"];
        CheckDev -> Success [label = "No"];

        Start [label = "Start", shape = roundedbox];
        GetExpected [label = "rtc_clk_xtal _freq_get()"];
        CalcRatio [label = "rtc_clk_cal_ratio()\nRTC_CAL_32K_XTAL 500 cycles"];
        CheckRatio [label = "ratio == 0?", shape = diamond];
        ErrorRatio [label = "Return\nBIST_ESP_CLOCK_TEST_ERR"];
        CalcXtal [label = "xtal_freq\nfrom ratio"];
        Deviation [label = "measured-expected\n/expected*100"];
        CheckDev [label = "> CONFIG_ESP_BIST\n_CLOCK_PERCENT\n_FREQUENCY_DRIFT?", shape = diamond];
        ErrorDev [label = "Return BIST_ESP_CLOCK_TEST_ERR"];
        Success [label = "Return BIST_ESP_OK"];
    }

Measures the ratio between main XTAL and 32kHz reference clock over 500 cycles, calculates frequency deviation as a percentage, and compares against ``CONFIG_ESP_BIST_CLOCK_PERCENT_FREQUENCY_DRIFT`` (configurable tolerance threshold).

Module API
^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 35 15 15 20 20

   * - Function name
     - Exec Time
     - Cycles
     - Instruction Count
     - Code size (Bytes)
   * - ``bist_ext_crystal_fail_test``
     - 80.38 ms
     - 321545
     - 320384
     - 128
   * - ``bist_main_crystal_test``
     - 61.01 ms
     - 2443854
     - 2442400
     - 238

Source Files
^^^^^^^^^^^^

.. list-table:: Source Files
   :header-rows: 1
   :widths: 30 20 50

   * - Source File
     - Version
     - MD5
   * - ``src/bist/core/clock/bist_clock_fail.c``
     - v1.0.0
     - 9375bd4e39ba4049f7e38d84457a95db

Coding and Interfaces
^^^^^^^^^^^^^^^^^^^^^

- The clock test uses explicit error handling: if initialization or frequency measurement fails, the function returns an explicit error code. Callback registration is checked, and all error paths are handled.
- The external crystal fail test is conditionally compiled using ``SOC_XT_WDT_SUPPORTED`` from ``soc/soc_caps.h``. On SoCs without this capability, the function returns ``BIST_ESP_OK`` (test skipped).
- This test uses the external crystal watchdog driver (``esp_xt_wdt_*``), RTC clock functions, and linker-defined configuration.
- Uses static variables for test state and callback.
- No dynamic memory is used.
- All configuration is via local variables or linker symbols.
- All functions have a single entry and exit. Early returns are used for error handling.
- Branching is limited to error detection and callback handling. No deep nesting or complex logic.
- Loops are bounded by the number of measurement cycles or wait iterations. All loops are finite and predictable.
- Only integer and floating-point arithmetic for frequency measurement and deviation calculation.
- Division is only used for frequency and deviation calculation, and all denominators are checked for zero before use.
- Interrupts are used via the external crystal watchdog callback, which is registered and handled safely.
- Pointers are used for callback registration and configuration structures. All pointer usage is explicit and safe.
- No recursion is used in this test.
- No goto or label-based jumps are used.

.. _windowed-watchdog-operation:

Watchdog Operation Test
-----------------------

Standard WDT
^^^^^^^^^^^^
.. blockdiag::
    :scale: 50%
    :caption: Standard WDT Test
    :align: center

    blockdiag {
        orientation = portrait;
        Start -> Init -> SetupInterrupt -> InitHAL -> Stage0 -> Stage1 -> Enable -> Loop;
        Loop -> FeedCheck;
        FeedCheck -> NoFeed [label = "Yes"];
        FeedCheck -> Feed [label = "No"];
        Feed -> Loop;
        NoFeed -> Timeout;
        Timeout -> ISR -> Clear -> CallCb;
        CallCb -> UserCb [label = "Yes"];
        CallCb -> WaitReset [label = "No"];
        UserCb -> WaitReset;
        WaitReset -> Reset;

        Start [label = "Start", shape = roundedbox];
        Init [label = "wdt_init\ntimeout_us"];
        SetupInterrupt [label = "Setup\nETS_INT_WDT_INUM"];
        InitHAL [label = "wdt_hal_init\nprescaler 40000"];
        Stage0 [label = "STAGE0\ntimeout_us/500\nINT"];
        Stage1 [label = "STAGE1\ntimeout_us/500*2\nRESET"];
        Enable [label = "wdt_hal_enable"];
        Loop [label = "Main loop: wdt_feed"];
        FeedCheck [label = "stop_feed?", shape = diamond];
        NoFeed [label = "Skip feed"];
        Feed [label = "wdt_hal_feed"];
        Timeout [label = "Timeout"];
        ISR [label = "Interrupt"];
        Clear [label = "Clear status"];
        CallCb [label = "callback?", shape = diamond];
        UserCb [label = "Call user callback"];
        WaitReset [label = "Wait reset"];
        Reset [label = "System reset"];
    }

**Standard WDT:** Configured with two stages:

- STAGE0: Interrupt at ``timeout_us / 500`` ticks (where 500 is ``MWDT_DEFAULT_TICKS_PER_US``, the conversion factor from microseconds to watchdog ticks)
- STAGE1: System reset at ``timeout_us / 500 * 2`` ticks (double the STAGE0 timeout)
- Application must call ``wdt_feed()`` periodically within timeout window

Windowed WDT
^^^^^^^^^^^^
.. blockdiag::
    :scale: 50%
    :caption: Windowed WDT Test
    :align: center

    blockdiag {
        orientation = portrait;
        Start -> InitWDT -> InitWin -> CreateTimer -> StartTimer -> Flags -> Loop;
        Loop -> StopCheck;
        StopCheck -> NoFeed [label = "Yes"];
        StopCheck -> WindowCheck [label = "No"];
        WindowCheck -> Underflow [label = "No"];
        WindowCheck -> Feed [label = "Yes"];
        Underflow -> NoFeed;
        Feed -> ResetFlag -> RestartTimer -> Loop;
        NoFeed -> Timeout;
        Timeout -> ISR;

        Start [label = "Start", shape = roundedbox];
        InitWDT [label = "wdt_init\ntimeout_us"];
        InitWin [label = "wdt_init_windowed\nunderflow_timeout_us"];
        CreateTimer [label = "Create ESP\ntimer ISR"];
        StartTimer [label = "Start one-shot\nunderflow_timeout_us"];
        Flags [label = "is_windowed=true\nwindow_open_flag=false"];
        Loop [label = "Main loop: wdt_feed"];
        StopCheck [label = "stop_feed?", shape = diamond];
        NoFeed [label = "Return"];
        WindowCheck [label = "window_open _flag?", shape = diamond];
        Underflow [label = "Log underflow set stop_feed"];
        Feed [label = "wdt_hal_feed"];
        ResetFlag [label = "window_open_flag=false"];
        RestartTimer [label = "Restart timer"];
        Timeout [label = "Overflow timeout"];
        ISR [label = "Interrupt/reset"];
    }

**Windowed WDT:** Adds underflow protection:

- Uses ESP timer to track minimum feed interval (``underflow_timeout_us``)
- ``window_open_flag`` prevents feeding before underflow timeout expires
- If feed attempted too early, sets ``stop_feed=true`` and logs underflow error
- Prevents PC faults (infinite loops, unexpected jumps) from masking as valid operation

The windowed WDT behavior is validated by the ``windowed_wdt_test`` application (normal operation within the feed window, underflow detection, consecutive feed cycles). The esp_timer driver (``src/bist/drivers/esp_timer.c``) provides the high-resolution timer used for the underflow window.

Module API
^^^^^^^^^^

- ``bist_wdt_test`` (see :ref:`watchdog-operation-test` for behavior)

Source Files
^^^^^^^^^^^^

.. list-table:: Source Files
   :header-rows: 1
   :widths: 30 20 50

   * - Source File
     - Version
     - MD5
   * - ``src/bist/core/wdt/bist_wdt.c``
     - v1.0.0
     - 71da4b2eacf7335b795c8e41175e80a8

Coding and Interfaces
^^^^^^^^^^^^^^^^^^^^^

- The watchdog test checks the reset reason and only proceeds if the reset was not caused by the watchdog. All error paths return explicit error codes.
- This test uses the watchdog driver (``wdt_init``, ``wdt_deinit``) and ROM reset reason functions.
- Uses local variables for configuration and state.
- No dynamic memory is used.
- All configuration is via local variables or linker symbols.
- The main function has a single entry and, under normal conditions, a single exit. Early returns are used for error handling.
- Branching is limited to error detection and reset reason checks. No deep nesting or complex logic.
- Loops are bounded by the timeout and wait period. All loops are finite and predictable.
- Only integer comparisons and assignments are used. No floating-point or complex arithmetic.
- Interrupts are used via the watchdog driver, which is registered and handled safely.
- Pointers are used for configuration and driver calls. All pointer usage is explicit and safe.
- No recursion is used in this test.
- No goto or label-based jumps are used.
- Type conversions are only used for configuration and driver calls, which are explicit and safe.
- No division operations are present in this test.

.. _gpio-plausibility-test:

GPIO Plausibility Test
----------------------

GPIO Output
^^^^^^^^^^^
.. blockdiag::
    :scale: 100%
    :caption: GPIO Output Test
    :align: center

    blockdiag {
        orientation = portrait;
        Start -> Validate;
        Validate -> Error [label = "No"];
        Validate -> Reset1 [label = "Yes"];
        Reset1 -> Dir -> Set0 -> Read0 -> Check0;
        Check0 -> Err0 [label = "No"];
        Check0 -> Set1 [label = "Yes"];
        Set1 -> Read1 -> Check1;
        Check1 -> Err1 [label = "No"];
        Check1 -> Reset2 [label = "Yes"];
        Reset2 -> Success;

        Start [label = "Start", shape = roundedbox];
        Validate [label = "GPIO_IS_VALID_GPIO?", shape = diamond];
        Error [label = "Return BIST_ESP_IO_TEST_ERR"];
        Reset1 [label = "gpio_reset_pin"];
        Dir [label = "gpio_set_direction INPUT_OUTPUT"];
        Set0 [label = "gpio_set_level 0"];
        Read0 [label = "gpio_get_level"];
        Check0 [label = "level==0?", shape = diamond];
        Err0 [label = "Return BIST_ESP_IO_TEST_ERR"];
        Set1 [label = "gpio_set_level 1"];
        Read1 [label = "gpio_get_level"];
        Check1 [label = "level==1?", shape = diamond];
        Err1 [label = "Return BIST_ESP_IO_TEST_ERR"];
        Reset2 [label = "gpio_reset_pin"];
        Success [label = "Return BIST_ESP_OK"];
    }

**GPIO Output Test:**

- Validates GPIO pin number using ``GPIO_IS_VALID_GPIO`` macro
- Resets pin to default state with ``gpio_reset_pin``
- Configures as input/output with ``gpio_set_direction(gpio_num, GPIO_MODE_INPUT_OUTPUT)``
- Sets level to 0 (``gpio_set_level(gpio_num, 0)``) and reads back with ``gpio_get_level``
- Sets level to 1 (``gpio_set_level(gpio_num, 1)``) and reads back
- Returns ``BIST_ESP_IO_TEST_ERR`` on any mismatch or error
- Resets pin to default state after test

GPIO Input
^^^^^^^^^^
.. blockdiag::
    :scale: 50%
    :caption: GPIO Input Test
    :align: center

    blockdiag {
        orientation = portrait;
        Start -> Validate;
        Validate -> Error [label = "No"];
        Validate -> Reset1 [label = "Yes"];
        Reset1 -> Dir -> Read -> Check;
        Check -> Err [label = "No"];
        Check -> Reset2 [label = "Yes"];
        Reset2 -> Success;

        Start [label = "Start", shape = roundedbox];
        Validate [label = "GPIO_IS_VALID_GPIO?", shape = diamond];
        Error [label = "Return BIST_ESP_IO_TEST_ERR"];
        Reset1 [label = "gpio_reset_pin"];
        Dir [label = "gpio_set_direction INPUT"];
        Read [label = "gpio_get_level"];
        Check [label = "level == expected_level?", shape = diamond];
        Err [label = "Return BIST_ESP_IO_TEST_ERR"];
        Reset2 [label = "gpio_reset_pin"];
        Success [label = "Return BIST_ESP_OK"];
    }

**GPIO Input Test:**

- Validates GPIO pin number
- Resets pin to default state
- Configures as input only with ``gpio_set_direction(gpio_num, GPIO_MODE_INPUT)``
- Reads level with ``gpio_get_level`` and compares against ``expected_level`` parameter
- Returns ``BIST_ESP_IO_TEST_ERR`` if level does not match expected
- Resets pin after test

Both tests ensure GPIO pins are restored to default state after testing to avoid side effects.

Module API
^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 45 20 15 20 20

   * - Function name
     - Exec Time
     - Cycles
     - Instruction Count
     - Code size (Bytes)
   * - ``bist_gpio_output_test(gpio_num_t gpio_num)``
     - 121.33 us
     - 4853
     - 2985
     - 342
   * - ``bist_gpio_input_test(gpio_num_t gpio_num, bool expected_level)``
     - 114.43 us
     - 4577
     - 2755
     - 374

Source Files
^^^^^^^^^^^^

.. list-table:: Source Files
   :header-rows: 1
   :widths: 30 20 50

   * - Source File
     - Version
     - MD5
   * - ``src/bist/core/io/bist_gpio.c``
     - v1.0.0
     - a13966f5e6630a63b47b6eefab3bc0e5

Coding and Interfaces
^^^^^^^^^^^^^^^^^^^^^
- The GPIO test checks all input parameters for validity (e.g., valid GPIO number) and returns explicit error codes on failure.
- This test uses the GPIO driver (``gpio_set_direction``, ``gpio_set_level``, ``gpio_get_level``, ``gpio_reset_pin``) and error code definitions.
- Uses local variables for configuration and state.
- No dynamic memory is used.
- All functions have a single entry and exit. Early returns are used for error handling.
- Branching is limited to error detection and hardware access checks. No deep nesting or complex logic.
- Loops are bounded by the number of logic levels to test and the number of pins (if tested in a loop externally). All loops are finite and predictable.
- Only integer comparisons and assignments are used. No floating-point or complex arithmetic.
- No interrupts are used or manipulated by this test.
- Pointers are used for driver calls and local variables. All pointer usage is explicit and safe.
- No recursion is used in this test.
- No goto or label-based jumps are used.
- Type conversions are only used for driver calls, which are explicit and safe.
- No division operations are present in this test.

.. _adc-plausibility-test:

ADC Plausibility Test
---------------------

ADC Low Level
^^^^^^^^^^^^^
.. blockdiag::
    :scale: 100%
    :caption: ADC Low Level Test
    :align: center

    blockdiag {
        orientation = portrait;
        Start -> NewUnit;
        NewUnit -> Error [label = "Fail"];
        NewUnit -> Config [label = "OK"];
        Config -> Error [label = "Fail"];
        Config -> PullDown [label = "OK"];
        PullDown -> Error [label = "Fail"];
        PullDown -> Delay -> Read;
        Read -> Error [label = "Fail"];
        Read -> Check;
        Check -> Error [label = "raw > CONFIG_ESP_BIST\n_ADC_PERCENT_DEVIATION"];
        Check -> Cleanup [label = "OK"];
        Cleanup -> Success;

        Start [label = "Start", shape = roundedbox];
        NewUnit [label = "adc_oneshot_new_unit"];
        Config [label = "adc_oneshot_config_channel\nATTEN_DB_12"];
        PullDown [label = "gpio_set_pull_mode PULLDOWN_ONLY"];
        Delay [label = "ets_delay_us 10000"];
        Read [label = "adc_oneshot_read"];
        Check [label = "raw <= CONFIG_ESP_BIST\n_ADC_PERCENT\n_DEVIATION?", shape = diamond];
        Cleanup [label = "gpio_reset_pin\nadc_oneshot_del_unit"];
        Error [label = "Return BIST_ESP_ADC_TEST_ERR"];
        Success [label = "Return BIST_ESP_OK"];
    }

**ADC Low Level Test:**

- Creates ADC oneshot unit with ``adc_oneshot_new_unit``
- Configures channel with 12 dB attenuation and default bitwidth
- Maps ADC channel to GPIO and enables internal pull-down with ``gpio_set_pull_mode``
- Waits 10 ms for the pin to settle
- Reads raw ADC value with ``adc_oneshot_read``
- Verifies raw value is within ``CONFIG_ESP_BIST_ADC_PERCENT_DEVIATION`` of zero
- Resets GPIO and deletes ADC unit on completion or error

ADC High Level
^^^^^^^^^^^^^^
.. blockdiag::
    :scale: 100%
    :caption: ADC High Level Test
    :align: center

    blockdiag {
        orientation = portrait;
        Start -> NewUnit;
        NewUnit -> Error [label = "Fail"];
        NewUnit -> Config [label = "OK"];
        Config -> Error [label = "Fail"];
        Config -> PullUp [label = "OK"];
        PullUp -> Error [label = "Fail"];
        PullUp -> Delay -> Read;
        Read -> Error [label = "Fail"];
        Read -> Check;
        Check -> Error [label = "raw too low"];
        Check -> Cleanup [label = "OK"];
        Cleanup -> Success;

        Start [label = "Start", shape = roundedbox];
        NewUnit [label = "adc_oneshot_new_unit"];
        Config [label = "adc_oneshot_config_channel\nATTEN_DB_12"];
        PullUp [label = "gpio_set_pull_mode PULLUP_ONLY"];
        Delay [label = "ets_delay_us 10000"];
        Read [label = "adc_oneshot_read"];
        Check [label = "raw >= high - CONFIG_ESP_BIST\n_ADC_PERCENT\n_DEVIATION?", shape = diamond];
        Cleanup [label = "gpio_reset_pin\nadc_oneshot_del_unit"];
        Error [label = "Return BIST_ESP_ADC_TEST_ERR"];
        Success [label = "Return BIST_ESP_OK"];
    }

**ADC High Level Test:**

- Creates and configures ADC oneshot unit (same as low level test)
- Maps ADC channel which is attenuated ~12 dB for extending measurement range to GPIO and enables internal pull-up with ``gpio_set_pull_mode``
- Waits 10 ms for the pin to settle
- Reads raw ADC value with ``adc_oneshot_read``
- Verifies raw value is within ``CONFIG_ESP_BIST_ADC_PERCENT_DEVIATION`` (configurable tolerance) of the SoC-specific high reference (``BIST_ADC_HIGH_VAL``)
- Resets GPIO and deletes ADC unit on completion or error

ADC Reference (ESP32-C3 only)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. blockdiag::
    :scale: 100%
    :caption: ADC Reference Test (ESP32-C3)
    :align: center

    blockdiag {
        orientation = portrait;
        Start -> Validate;
        Validate -> Error [label = "No"];
        Validate -> NewUnit [label = "Yes"];
        NewUnit -> Error [label = "Fail"];
        NewUnit -> Config [label = "OK"];
        Config -> Error [label = "Fail"];
        Config -> Vref [label = "OK"];
        Vref -> Delay -> Read;
        Read -> Error [label = "Fail"];
        Read -> Check;
        Check -> Error [label = "out of range"];
        Check -> Cleanup [label = "OK"];
        Cleanup -> Success;

        Start [label = "Start", shape = roundedbox];
        Validate [label = "valid unit/channel?", shape = diamond];
        NewUnit [label = "adc_oneshot_new_unit"];
        Config [label = "adc_oneshot_config_channel\nATTEN_DB_12"];
        Vref [label = "adc_ll_vref_output enable"];
        Delay [label = "ets_delay_us 10000"];
        Read [label = "adc_oneshot_read"];
        Check [label = "raw in reference range?", shape = diamond];
        Cleanup [label = "gpio_reset_pin\nadc_oneshot_del_unit"];
        Error [label = "Return BIST_ESP_ADC_TEST_ERR"];
        Success [label = "Return BIST_ESP_OK"];
    }

**ADC Reference Test (ESP32-C3 only):**

- Validates ADC unit and channel parameters
- Creates and configures ADC oneshot unit
- Enables internal VREF output with ``adc_ll_vref_output`` to bias the pin near mid-scale
- Waits 10 ms, reads raw value, and verifies it is within ``CONFIG_ESP_BIST_ADC_PERCENT_DEVIATION`` (configurable tolerance) of ``BIST_ADC_REFERENCE`` (1500)
- Detects stuck-at-low or stuck-at-high faults that pass the pull-up/pull-down tests alone
- Resets GPIO and deletes ADC unit on completion or error

All tests ensure the ADC channel GPIO is restored to default state after testing to avoid side effects.

Module API
^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 45 20 15 20 20

   * - Function name
     - Exec Time
     - Cycles
     - Instruction Count
     - Code size (Bytes)
   * - ``bist_adc_low_level_test(adc_unit_t unit, adc_channel_t channel)``
     - 126.43 ms
     - 5057005
     - 2697513
     - 380
   * - ``bist_adc_high_level_test(adc_unit_t unit, adc_channel_t channel)``
     - 127.77 ms
     - 5110753
     - 2723904
     - 388
   * - ``bist_adc_reference_test(adc_unit_t unit, adc_channel_t channel)`` (ESP32-C3 only)
     - 129.51 ms
     - 5180240
     - 2749217
     - 748

Source Files
^^^^^^^^^^^^

.. list-table:: Source Files
   :header-rows: 1
   :widths: 30 20 50

   * - Source File
     - Version
     - MD5
   * - ``src/bist/core/io/bist_adc.c``
     - N/A
     - 5329ede84aa9604bf42a116b040d2d74

Coding and Interfaces
^^^^^^^^^^^^^^^^^^^^^
- The ADC test validates unit and channel parameters (on the reference test) and returns explicit error codes on failure.
- This test uses the ADC oneshot driver (``adc_oneshot_new_unit``, ``adc_oneshot_config_channel``, ``adc_oneshot_read``, ``adc_oneshot_del_unit``), GPIO pull configuration (``gpio_set_pull_mode``, ``gpio_reset_pin``), and on ESP32-C3 the low-level VREF output (``adc_ll_vref_output``).
- Uses local variables for configuration and state; ADC unit handle is managed through the oneshot driver API.
- All functions have a single entry and exit. Early returns and a single ``cleanup`` label are used for error handling.
- Branching is limited to error detection and reading validation. No deep nesting or complex logic.
- A fixed 10 ms delay bounds settling time before each ADC read.
- Integer comparisons and tolerance arithmetic are used; tolerance is derived from ``CONFIG_ESP_BIST_ADC_PERCENT_DEVIATION``.
- No interrupts are used or manipulated by this test.
- Pointers are used for driver calls and local variables. All pointer usage is explicit and safe.
- No recursion is used in this test.
- No goto or label-based jumps are used except for centralized cleanup.
- Type conversions are only used for driver calls, which are explicit and safe.
- Division is used only for tolerance calculation with a bounded divisor (100).

Performance Metrics Collection
------------------------------

Overview
^^^^^^^^

The BIST library provides a macro-based API for collecting performance metrics using the custom RISC-V Performance Counter CSRs. These custom CSRs have been implemented in the address space reserved by RISC-V standard for custom use. This utility enables measurement of various microarchitectural events during BIST test execution, including CPU cycles, instruction counts, and hardware hazards.

The metrics collection is designed for low-overhead instrumentation of BIST routines.

Hardware Basis
^^^^^^^^^^^^^^

The metrics system leverages three RISC-V Performance Monitor CSRs:

- **PCER:** Performance Counter Event Register - Selects which event to monitor (only one event at a time)
- **PCMR:** Performance Counter Mode Register - Enables the counter and selects saturation behavior
- **PCCR:** Performance Counter Count Register - Holds the counter value

Event Modes
^^^^^^^^^^^

The following microarchitectural events can be monitored:

.. list-table::
   :header-rows: 1
   :widths: 30 40 10

   * - Event Mode
     - Description
     - Enum Value
   * - ``BIST_METRICS_MODE_CYCLE``
     - Count CPU cycles (Cycle count does not increment during WFI mode)
     - 0
   * - ``BIST_METRICS_MODE_INST``
     - Count Instructions
     - 1
   * - ``BIST_METRICS_MODE_LD_HAZARDS``
     - Count Load Hazards
     - 2
   * - ``BIST_METRICS_MODE_JMP_HAZARDS``
     - Count Jump Hazards
     - 3
   * - ``BIST_METRICS_MODE_IDLE``
     - Count Idle cycles
     - 4
   * - ``BIST_METRICS_MODE_LOAD``
     - Count Load instructions
     - 5
   * - ``BIST_METRICS_MODE_STORE``
     - Count Store instructions
     - 6
   * - ``BIST_METRICS_MODE_JMP_UNCOND``
     - Count Unconditional jumps
     - 7
   * - ``BIST_METRICS_MODE_BRANCH``
     - Count Branches
     - 8
   * - ``BIST_METRICS_MODE_BRANCH_TAKEN``
     - Count Branches taken
     - 9
   * - ``BIST_METRICS_MODE_INST_COMP``
     - Count Compressed instructions
     - 10

Implementation
^^^^^^^^^^^^^^

The metrics collection API uses four macros that work together to measure code sections:

1. **BIST_METRICS_INIT(mode_)** - Configure the Performance Counter for the target event and reset counter to zero
2. **BIST_METRICS_BEGIN(metrics_)** - Capture starting counter value
3. **BIST_METRICS_END(metrics_)** - Capture ending counter value
4. **BIST_METRICS_PRINT(name_, metrics_)** - Output results to console in CI-parseable format

Data Structure
^^^^^^^^^^^^^^

Measurements are stored in the ``bist_metrics_t`` structure:

- ``start_value``: Counter value at measurement start
- ``end_value``: Counter value at measurement end
- ``valid``: Boolean flag indicating whether a complete measurement was captured

Usage Pattern
^^^^^^^^^^^^^

The typical usage sequence is:

.. code-block:: c

    // 1. Initialize PMU for the event to measure
    BIST_METRICS_INIT(BIST_METRICS_MODE_CYCLE);

    // 2. Create metrics structure
    bist_metrics_t my_metrics;

    // 3. Start collection
    BIST_METRICS_BEGIN(my_metrics);

    // ... code to be measured ...

    // 4. Stop collection
    BIST_METRICS_END(my_metrics);

    // 5. Print results
    BIST_METRICS_PRINT("test_name", my_metrics);

Output Format
^^^^^^^^^^^^^

The ``BIST_METRICS_PRINT`` macro outputs results in a CI-parseable format:

.. code-block:: text

    METRICS: test_name Mode: 0 delta=12345

Where:

- ``test_name`` is the label provided to the macro (or "test" if NULL)
- ``Mode`` is the numeric event mode (0-10)
- ``delta`` is the count of measured events (end_value - start_value)

If metrics are invalid (incomplete measurement), a warning is logged instead.

Design Considerations
^^^^^^^^^^^^^^^^^^^^^

- **Non-intrusive:** Minimal overhead; CSR reads/writes do not significantly impact measured code
- **Single Event at a Time:** Only one microarchitectural event can be monitored per initialization
- **Counter Overflow:** Counters are 32-bit; overflow is not handled and will cause measurement inaccuracy for very long code sections
- **No Dynamic Memory:** Uses stack-allocated structures only
- **Macro-based:** Inline implementation avoids function call overhead
- **CI Integration:** Output format is designed for easy parsing by continuous integration tools

Source Files
^^^^^^^^^^^^

.. list-table:: Source Files
   :header-rows: 1
   :widths: 30 20 50

   * - Source File
     - Version
     - MD5
   * - ``src/bist/include/bist_metrics.h``
     - v1.0.0
     - fdb9b4ef52f3c7ba0c2c9395f5b5bbee

Coding and Interfaces
^^^^^^^^^^^^^^^^^^^^^

- The metrics API is purely macro-based with no function calls, minimizing measurement overhead
- CSR access is performed via ``RV_READ_CSR`` and ``RV_WRITE_CSR`` helpers from ``riscv/csr.h``
- Logging is performed via ``ESP_LOGI`` for success and ``ESP_LOGW`` for warnings
- The delta calculation (end_value - start_value) is performed by the print macro
- All operations are statically bounded with no loops or dynamic behavior
- No pointers are dereferenced in the metrics API itself
- Bitwise operations are used only to extract the mode from PCER
- The implementation uses only inline code; no recursion or external function calls (except logging)
- Type conversions are limited to uint32_t arithmetic and logging parameter formatting

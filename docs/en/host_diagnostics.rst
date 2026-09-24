.. _host-diagnostics:

Host Diagnostics
================

**Status:** Architecture and implementation overview (not a certified safety claim)

.. _hd-overview:

Overview and Scope
------------------

ESP-BIST provides a Software Test Library (STL) of Class-B-style diagnostics
(March RAM, flash CRC, CPU register, CSR, stack, clock, watchdog, GPIO, ADC).
On SoCs with a low-power RISC-V core (ESP32-C5, ESP32-C6, ESP32-P4) the
library can run on the LP-CPU as a **safety companion** that self-tests and
then supervises the main application running on the HP-CPU under a QM
(Quality Management) host OS (ESP-IDF, Zephyr, NuttX).

**Host Diagnostics** is the feature that turns this one-way observer model into
a bidirectional supervision architecture. It adds:

- A **Host Diagnostic Agent (HDA)** on the HP side that auto-starts when the LP
  companion is present, answers Q&A challenges, and executes companion-triggered
  host audits that are enabled in the catalog.
- A shared **supervision protocol** with typed messages, sequence numbers,
  timeouts, and stale-frame rejection.
- **Companion-owned safe-state policy**: the LP companion judges every answer
  and audit result, and forces safe state (stops feeding the LP WDT, which
  resets the system) when the host fails.

.. note::

   Host Diagnostics is **not** a Safety OS. In industry that term means a
   *certified kernel*. The agent is a thin
   collaboration component supervised by the companion.

   The on-die LP core is a valid **software supervisor** but is **not** a fully
   independent safety channel. Shared die, supply, reset, and clock domains
   create common-cause risk that an external Q&A watchdog / PMIC addresses when
   stronger independence claims are required. See :ref:`hd-independence`.

.. _hd-rationale:

Rationale
---------

A self-test library that only proves the *supervisor* is healthy is not
sufficient: a stuck, corrupted, or mis-executing host can still drive hazardous
outputs while the companion has no way to detect or react. Host Diagnostics
gives the companion a bidirectional audit channel so it can continuously
verify that the host is alive, executing the expected code path, and producing
correct computational results.

The companion issues timed Q&A challenges that the host must answer with a
keyed CRC within a configurable window, proving liveness and computational
integrity in every supervision cycle. The same channel carries a catalog of
companion-triggered diagnostics (RAM March on HP DRAM, host image CRC,
CPU/CSR checks, and more -- see :ref:`hd-audit-catalog`) so the
companion can systematically cover the host fault space. If any answer is
wrong, late, or missing, the companion forces safe state without host
cooperation.

.. _hd-independence:

Independence of the On-Die LP Companion
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The LP companion schedules host audits, judges results, and owns safe-state
policy. That does not make the on-die LP a fully independent safety channel.
Independence is the same physical problem across standards:

.. list-table:: Independent channel vs on-die LP companion
   :header-rows: 1
   :widths: 30 35 35

   * - Property
     - Independent channel / external monitor
     - On-die LP companion (ESP32-C5/C6/P4)
   * - Power / supply
     - Often separate rail or supervised supply
     - Same chip; often shared supply domains
   * - Reset
     - Independent reset path
     - Shared die / chip-level reset coupling
   * - Clock
     - Independent clock supervision
     - Limited independent clock story vs HP
   * - Fault containment
     - HP hang/corruption cannot disable the monitor
     - Shared silicon, buses, and often shared power/debug events
   * - Safe-state actuation
     - Can cut power or hold outputs even if main CPU is dead
     - May still depend on chip-level resources
   * - Safety-case expectation
     - Explicit channel separation arguments (ASIL decomposition, HFT)
     - Harder to claim full independence without extra hardware

**What Host Diagnostics can claim architecturally:**

- LP is the software supervisor of the QM host.
- Companion BIST plus host audits plus companion-owned judgment improve
  diagnostic coverage of the HP OS path.
- Domain roles (companion decides; host collaborates) still apply.

**What must not be claimed without additional hardware and analysis:**

- That LP-on-die alone equals an automotive safety island or a fully
  independent industrial safety channel.
- That LP supervision alone provides the same independence as an external Q&A
  watchdog IC / PMIC or a separate safety MCU.

When a product needs stronger independence, add an external Q&A watchdog /
PMIC above the on-die companion. The LP remains useful for fine-grained host
audits; the external monitor covers common-cause and chip-level failures the LP
cannot independently survive.

.. _hd-terminology:

Terminology
-----------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Term
     - Definition
   * - Safety companion
     - LP-CPU (or external MCU) that supervises the host, schedules diagnostics,
       and owns safe-state policy
   * - QM host OS
     - Main OS on HP (ESP-IDF / Zephyr / NuttX); not ASIL/SIL-rated by itself;
       treated as a supervised collaborator
   * - Host Diagnostic Agent (HDA)
     - HP-side collaboration component that auto-enables with LP BIST, answers
       challenges, and runs triggered diagnostics
   * - Host audit
     - Temporal and logical proof that the host is alive and executing expected
       code (Q&A, checkpoints, memory/CPU diagnostics)
   * - Companion BIST
     - Companion runs Class-B tests on its own resources (LP CPU, LP RAM, LP
       image, LP WDT)
   * - FTTI
     - Fault Tolerant Time Interval; maximum allowed time from fault occurrence
       to reaching safe state
   * - FFI
     - Freedom From Interference; host faults must not corrupt companion state
       or the HP-LP protocol
   * - Safe state
     - Product-defined fail-safe reaction (reset, de-energize outputs, hold
       critical GPIOs, etc.)
   * - Supervised entity
     - Host software under checkpoint / Q&A supervision

.. _hd-domain-roles:

Domain Roles and Trust Model
-----------------------------

Two domains at different trust levels. The companion owns the safety decision;
the host runs the product and only collaborates.

.. blockdiag::
   :scale: 100%
   :caption: Host Diagnostics Domain Roles
   :align: center

   blockdiag {
       "LP BIST" -> "Host Supervisor";
       "Host Supervisor" -> "Safe State Owner";
       "Application" -> "Host Agent";
       "Host Agent" -> "Diagnostic Hooks";
       "Host Supervisor" -> "Host Agent" [label = "challenge / audit"];
       "Diagnostic Hooks" -> "Host Supervisor" [label = "answers / results"];

       "LP BIST" [shape = roundedbox];
       "Host Supervisor" [shape = box];
       "Safe State Owner" [shape = box];
       "Application" [shape = roundedbox];
       "Host Agent" [shape = box];
       "Diagnostic Hooks" [shape = box];

       group {
           label = "Safety Companion (LP)";
           "LP BIST"; "Host Supervisor"; "Safe State Owner";
       }
       group {
           label = "QM Host OS (HP)";
           "Application"; "Host Agent"; "Diagnostic Hooks";
       }
   }

Companion Responsibilities (LP)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Role
     - Responsibility
   * - BIST
     - Run Class-B tests on itself (LP CPU regs/CSR, LP RAM March, LP image
       CRC, LP WDT). Establishes "can I trust the supervisor?"
   * - Host supervision
     - Issue challenges/checkpoints; schedule host audits; check answers and
       reports
   * - Diagnostic schedule
     - Post-boot then periodic runtime cadence sized to the FTTI budget
   * - Safe-state policy
     - On wrong/late answer, failed host diagnostic, or failed companion BIST: force
       safe state. Host must not own this decision
   * - Protocol authority
     - Sequence numbers, timeouts, message validity; reject malformed host
       replies

Host Responsibilities (HP)
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Role
     - Responsibility
   * - Application
     - Product features (connectivity, UI, control loops, etc.)
   * - Host agent
     - Auto-start when LP companion is present; answer challenges inside the
       window; no manual user intervention
   * - Diagnostic collaboration
     - On companion trigger: quiesce as needed, execute the requested
       diagnostic, report results for companion judgment
   * - Reporting only
     - May log PASS/FAIL for tests; must **not** self-declare global safety or
       suppress companion safe-state

Critical Software Placement
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Placement
     - What belongs there
   * - Must run on companion
     - Supervision, audit schedule, WDT/safe-state, and any function that must
       work when HP is broken
   * - May run on host
     - Normal application, including safety-related logic that is supervised
       (checkpoints / Q&A) rather than trusted alone
   * - Companion or dual-channel
     - Any function with a hard integrity claim that a crashed QM OS must not be
       able to violate alone

.. _hd-architecture:

Architecture
------------

Layering
^^^^^^^^

.. blockdiag::
   :scale: 100%
   :caption: Host Diagnostics Layers
   :align: center

   blockdiag {
       "Companion BIST" -> "Q&A Issuer";
       "Q&A Issuer" -> "Protocol";
       "Diagnostic Scheduler" -> "Protocol";
       "Protocol" -> "Host Agent";
       "Host Agent" -> "Quiesce / Exclude";
       "Quiesce / Exclude" -> "ESP-IDF";
       "Quiesce / Exclude" -> "Zephyr";
       "Quiesce / Exclude" -> "NuttX";
       "Q&A Issuer" -> "LP WDT / Safe State";
       "Diagnostic Scheduler" -> "LP WDT / Safe State";

       "Companion BIST" [shape = roundedbox];
       "Q&A Issuer" [shape = box];
       "Diagnostic Scheduler" [shape = box];
       "Protocol" [shape = box];
       "Host Agent" [shape = box];
       "Quiesce / Exclude" [shape = box];
       "LP WDT / Safe State" [shape = box];
       "ESP-IDF" [shape = roundedbox];
       "Zephyr" [shape = roundedbox];
       "NuttX" [shape = roundedbox];

       group {
           label = "Safety Companion";
           "Companion BIST"; "Q&A Issuer"; "Diagnostic Scheduler";
           "LP WDT / Safe State";
       }
       group {
           label = "Supervision Protocol (shared)";
           "Protocol";
       }
       group {
           label = "Host Agent Package";
           "Host Agent"; "Quiesce / Exclude";
       }
       group {
           label = "QM Host OS";
           "ESP-IDF"; "Zephyr"; "NuttX";
       }
   }

.. _hd-protocol:

Supervision Protocol
--------------------

The supervision protocol uses typed messages, sequence numbers, and explicit
timeouts to carry companion-to-agent commands and agent-to-companion responses.

Frame Layout
^^^^^^^^^^^^

Two frame shapes exist on the wire:

**AGENT_READY (1 word):**

A single magic word ``0xCAFECAFE`` (``BIST_HD_AGENT_READY_MAGIC``). HP sends
this to LP once at agent start to signal that the host is ready for supervision.

**Typed multi-word frame (4 words):**

All other message types use a 4-word frame:

.. list-table:: Wire frame layout
   :header-rows: 1
   :widths: 15 85

   * - Word
     - Content
   * - word0
     - ``[TAG:8 = 0xB5][type:8][audit_id:8][nwords:8 = 4]``
   * - word1
     - ``[seq:16 in low bits]``
   * - word2
     - Payload: challenge value, answer, status bitmask, or checkpoint id
   * - word3
     - ``deadline_ticks``: window end in microseconds (0 if unused)

The tag byte ``0xB5`` distinguishes typed frames from stray data. Transports
use ``bist_hd_frame_nwords()`` on ``word0`` to determine how many words to
read: 1 for ``AGENT_READY``, 4 for tagged frames, 0 for invalid.

Message Types
^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 22 10 38 30

   * - Message
     - Direction
     - Purpose
     - Status
   * - ``AGENT_READY`` (1)
     - HP to LP
     - Host agent started; ready for supervision
     - Implemented
   * - ``CHALLENGE`` (2)
     - LP to HP
     - Challenge value + sequence + window deadline
     - Implemented
   * - ``ANSWER`` (3)
     - HP to LP
     - ``f(challenge, seq)`` + matching sequence
     - Implemented
   * - ``DIAG_REQ`` (4)
     - LP to HP
     - Request a host-catalog diagnostic (``audit_id`` selects which)
     - Implemented
   * - ``DIAG_RSP`` (5)
     - HP to LP
     - Pass/fail + diagnostic id + sequence
     - Implemented
   * - ``CHECKPOINT`` (6)
     - HP to LP
     - Alive / deadline / logical checkpoint
     - Implemented
   * - ``SAFE_STATE_NOTIFY`` (7)
     - LP to HP
     - Companion already acting; informational only
     - Companion sends; agent ignores
   * - ``LP_STATUS`` (8)
     - LP to HP
     - LP BIST result bitmask (``BIST_HD_BIT_POSTBOOT`` or
       ``BIST_HD_BIT_RUNTIME`` flag set)
     - Implemented

Sequence Number Rules
^^^^^^^^^^^^^^^^^^^^^

- 16-bit sequence in the low bits of word1.
- **Companion side:** monotonic, skips zero. ``bist_hd_seq_check()`` requires an
  exact match between the expected and actual sequence.
- **Agent side:** ``bist_hd_seq_is_stale()`` uses unsigned 16-bit wrap-aware
  distance. Incoming is stale when it is not strictly ahead of the last accepted
  sequence in the forward half of the space (``BIST_HD_SEQ_HALF_RANGE =
  0x8000``). The agent drops stale or duplicate challenges.

Checkpoint ID
^^^^^^^^^^^^^

The checkpoint ID is a private ``uint32_t`` counter inside the agent
(``bist_hd_agent.c``). ``bist_hd_checkpoint_reached()`` takes no argument;
each call increments the counter before sending, so the first payload is 1.
A failed transport send still consumes the ID, guaranteeing the companion
never sees a duplicate even after partial frame errors.

The companion validates the checkpoint ID with a wrap-safe unsigned-distance
check: the ID is rejected when ``diff == 0`` (duplicate) or ``diff > 0x80000000``
(backwards). This handles the ``UINT32_MAX`` wrap correctly.

Timeouts
^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 35 35 30

   * - Timeout
     - Kconfig / source
     - Default
   * - Agent ready wait
     - ``CONFIG_ESP_BIST_HD_AGENT_READY_TIMEOUT_US``
     - 1 000 000 us (1 s)
   * - Challenge window
     - ``CONFIG_ESP_BIST_HD_CHALLENGE_WINDOW_US``
     - 10 000 us (10 ms)
   * - Agent send
     - Hardcoded in ``bist_hd_agent_start()``
     - 10 000 ms
   * - Diagnostic response
     - ``CONFIG_ESP_BIST_HD_DIAG_TIMEOUT_US``
     - 50 000 us (50 ms)
   * - Generic tick compare
     - ``bist_hd_timeout_expired()``
     - Unsigned wrap-safe comparison

``CONFIG_ESP_BIST_HD_DIAG_TIMEOUT_US`` is used by the companion when waiting
for ``DIAG_RSP`` after issuing a ``DIAG_REQ``. During the wait, the companion
periodically feeds the LP watchdog to avoid triggering a reset while awaiting
the host response.

Challenge Algorithm
^^^^^^^^^^^^^^^^^^^

The Q&A answer function is:

.. code-block:: none

   f(challenge, seq) = CRC32(LE32(challenge) || LE16(seq) || LE32(key))

where:

- ``challenge`` is a pseudo-random value generated by the companion (xorshift32
  seeded from the LP free-running tick counter).
- ``seq`` is the current 16-bit sequence number.
- ``key`` resolves in order: compile-time ``BIST_HD_CHALLENGE_KEY`` (set via
  ``target_compile_definitions``), then ``CONFIG_ESP_BIST_HD_CHALLENGE_KEY``
  (Kconfig), then the default ``0xA5A5A5A5``. The compile-time path exists
  because the two ends of the wire do not always share one Kconfig namespace.

The CRC-32 uses polynomial ``0x04C11DB7`` reflected. The function is
``bist_hd_challenge_answer()`` in ``src/bist/bist_hd_challenge.c``.

.. _hd-lifecycle:

Supervision Lifecycle
---------------------

Boot and Init (``bist_hd_companion_init``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. blockdiag::
   :scale: 100%
   :caption: Companion Init Sequence
   :align: center

   blockdiag {
       "Mailbox Init" -> "Wait AGENT_READY";
       "Wait AGENT_READY" -> "Ready OK?";
       "Ready OK?" -> "Post-boot BIST" [label = "yes"];
       "Ready OK?" -> "Safe State" [label = "timeout / bad frame"];
       "Post-boot BIST" -> "BIST Pass?";
       "BIST Pass?" -> "Report LP_STATUS" [label = "yes"];
       "BIST Pass?" -> "Safe State" [label = "no"];
       "Report LP_STATUS" -> "Arm LP WDT";

       "Mailbox Init" [shape = roundedbox];
       "Wait AGENT_READY" [shape = box];
       "Ready OK?" [shape = diamond];
       "Post-boot BIST" [shape = box];
       "BIST Pass?" [shape = diamond];
       "Report LP_STATUS" [shape = box];
       "Arm LP WDT" [shape = roundedbox];
       "Safe State" [shape = box];
   }

Ordered steps:

#. ``bist_hd_comp_port_init()`` -- mailbox hardware.
#. (Optional) ``bist_cpu_stack_overflow_init()`` if stack test enabled.
#. Block on ``bist_hd_comp_port_recv()`` until ``AGENT_READY`` arrives or
   ``CONFIG_ESP_BIST_HD_AGENT_READY_TIMEOUT_US`` expires.
#. Run post-boot LP BIST (CPU regs, CPU CSR, RAM March-X + Abraham full, flash
   CRC -- depending on Kconfig). OR the ``BIST_HD_BIT_POSTBOOT`` flag into the
   result bitmask.
#. Send ``LP_STATUS`` with the post-boot bitmask.
#. Verify that all expected bits are set. If not: ``bist_hd_safe_state()``.
#. **Arm LP WDT** (``lp_wdt_init(CONFIG_ESP_BIST_WDT_TIMEOUT_US)``). The WDT is
   armed **only after** post-boot BIST passes; a pre-init failure never arms it.

Runtime Loop (``bist_hd_companion_loop``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. blockdiag::
   :scale: 100%
   :caption: Companion Runtime Loop Sequence
   :align: center

   blockdiag {
       "Check Safe State" -> "Feed LP WDT" [label = "healthy"];
       "Feed LP WDT" -> "Drain Checkpoints";
       "Drain Checkpoints" -> "Ckpt Valid?";
       "Ckpt Valid?" -> "Run Runtime BIST" [label = "yes"];
       "Ckpt Valid?" -> "Safe State" [label = "no / out-of-order"];
       "Run Runtime BIST" -> "Send LP_STATUS";
       "Send LP_STATUS" -> "BIST Pass?";
       "BIST Pass?" -> "Host Q&A Challenge" [label = "yes"];
       "BIST Pass?" -> "Safe State" [label = "no"];
       "Host Q&A Challenge" -> "Q&A Pass?";
       "Q&A Pass?" -> "Check Checkpoint Deadline" [label = "yes"];
       "Q&A Pass?" -> "Safe State" [label = "no / timeout / over-budget"];
       "Check Checkpoint Deadline" -> "Deadline Met?";
       "Deadline Met?" -> "Return Success" [label = "yes"];
       "Deadline Met?" -> "Safe State" [label = "no / missed"];

       "Check Safe State" [shape = diamond];
       "Feed LP WDT" [shape = box];
       "Drain Checkpoints" [shape = box];
       "Ckpt Valid?" [shape = diamond];
       "Run Runtime BIST" [shape = box];
       "Send LP_STATUS" [shape = box];
       "BIST Pass?" [shape = diamond];
       "Host Q&A Challenge" [shape = box];
       "Q&A Pass?" [shape = diamond];
       "Check Checkpoint Deadline" [shape = box];
       "Deadline Met?" [shape = diamond];
       "Return Success" [shape = roundedbox];
       "Safe State" [shape = box];
   }

Each iteration:

#. **Safe-State Check**: If already in safe state (``s_in_safe_state`` is set),
   return immediately (no WDT feed).
#. **Feed LP WDT**: ``lp_wdt_feed()`` feeds the hardware watchdog timer.
#. **Drain Checkpoints**: If ``CONFIG_ESP_BIST_HD_AUDIT_CHECKPOINT`` is enabled:
   clear the loop-received flag (``s_checkpoint_received = false``) and drain
   pending checkpoint messages via non-blocking receive (``drain_checkpoints()``).
   Validate each checkpoint ID using wrap-safe unsigned-distance comparison
   (``process_checkpoint()``). If duplicate or out-of-order: ``bist_hd_safe_state()``.
#. **Run Runtime LP BIST**: Execute runtime self-tests (CPU registers, CPU CSR,
   RAM March-A + Abraham time-division, stack overflow check) and OR the
   ``BIST_HD_BIT_RUNTIME`` flag into the result bitmask.
#. **Send LP_STATUS**: Transmit ``LP_STATUS`` message with the runtime bitmask to
   the HP agent.
#. **Verify Runtime BIST Status**: Verify that all expected runtime bits are set.
   If not: ``bist_hd_safe_state()``.
#. **Host Q&A Challenge & IRQ Latency Audit**: If ``CONFIG_ESP_BIST_HD_AUDIT_QA`` is enabled:

   - Send ``CHALLENGE`` with monotonic sequence number, random payload, and
     deadline window (``CONFIG_ESP_BIST_HD_CHALLENGE_WINDOW_US``).
   - Wait for ``ANSWER`` within the challenge window. If a ``CHECKPOINT`` frame
     arrives during this wait, consume and validate it immediately via
     ``process_checkpoint()``, then continue waiting for the ``ANSWER``.
   - Verify sequence number match, CRC-32 answer value, and that elapsed round-trip
     time does not exceed the challenge window.
   - If ``CONFIG_ESP_BIST_HD_AUDIT_IRQ_LATENCY`` is enabled: after a correct in-window
     answer, additionally check that elapsed round-trip time does not exceed
     ``CONFIG_ESP_BIST_HD_IRQ_LATENCY_BUDGET_US``.
   - If any challenge, value, sequence, timeout, or IRQ latency check fails: ``bist_hd_safe_state()``.
#. **Host Diagnostic Catalog Audits (DIAG_REQ / DIAG_RSP)**: If
   ``CONFIG_ESP_BIST_HD_AUDIT`` is enabled. The schedule executes each
   enabled catalog audit (QA stays on ``CHALLENGE`` / ``ANSWER``).
   For each enabled audit:

   - Send ``DIAG_REQ`` with monotonic sequence number, ``audit_id``, and
     timeout window (``CONFIG_ESP_BIST_HD_DIAG_TIMEOUT_US``).
   - Wait for ``DIAG_RSP`` while periodically feeding the LP WDT.
   - Consume any interleaved ``CHECKPOINT`` frames immediately via
     ``process_checkpoint()``.
   - Validate response frame type, sequence number, ``audit_id``, deadline,
     and payload status (``BIST_HD_STATUS_OK``).
   - On any timeout, bad frame, mismatched sequence, or failed payload:
     ``bist_hd_safe_state()``.
#. **Check Checkpoint Deadline**: If ``CONFIG_ESP_BIST_HD_AUDIT_CHECKPOINT`` is enabled:
   evaluate ``check_checkpoint()``. If a checkpoint was received during this loop
   (in step 3, step 7, or step 8), reset ``s_checkpoint_miss_count = 0`` and arm ID-order
   tracking. Otherwise, increment ``s_checkpoint_miss_count``; if it exceeds
   ``CONFIG_ESP_BIST_HD_CHECKPOINT_PERIOD_LOOPS`` (including a host that never
   reports from boot): ``bist_hd_safe_state()``.

The application-side LP main loop calls ``bist_hd_companion_loop()`` with a
delay between iterations (e.g. ``bist_hd_comp_port_delay_us(10000)``).

Agent Worker (``bist_hd_agent_start``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

On the HP side, ``bist_hd_agent_start()`` performs a one-shot setup:

#. ``bist_hd_platform_init()`` -- create the LP-status queue.
#. ``bist_hd_transport_init()`` -- initialize the mailbox transport and create
   the transmission mutex (``s_tx_mutex``).
#. ``bist_hd_transport_flush()`` -- discard mailbox state left over from a
   previous HP session. The LP companion keeps running across an HP reset, so
   pending words or frames that predate ``AGENT_READY`` must be dropped. IDF
   drains with a bounded non-blocking receive loop, Zephyr purges its RX
   message queue, and NuttX defers to a byte-level header resync inside
   ``bist_hd_transport_recv()`` because the driver lacks non-blocking reads.
#. Send ``AGENT_READY`` magic word (``0xCAFECAFE``) to the companion.
#. Start a high-priority worker thread (FreeRTOS task / Zephyr cooperative
   thread / NuttX ``SCHED_FIFO`` pthread).

The worker thread runs an infinite receive loop:

#. Block on ``bist_hd_transport_recv()`` (indefinite timeout).
#. If ``LP_STATUS``: push the bitmask into the platform queue so the
   application can retrieve it via ``bist_hd_agent_wait_lp_status()``.
#. If ``CHALLENGE``: check for stale or duplicate sequence numbers
   (``bist_hd_seq_is_stale()``); if fresh:

   - If ``CONFIG_ESP_BIST_HD_AUDIT_CHECKPOINT`` is enabled and a checkpoint is
     pending from ``bist_hd_checkpoint_reached()``, send the ``CHECKPOINT``
     frame first. Transmitting here guarantees the companion is blocked in its
     receive loop, avoiding mailbox contention with LP-to-HP transmissions.
     This is why checkpoint auditing depends on ``CONFIG_ESP_BIST_HD_AUDIT_QA``:
     without a challenge the host never flushes the pending checkpoint, and
     the companion would still run ``check_checkpoint()`` and enter safe state.
   - Compute the challenge answer via ``bist_hd_challenge_answer()`` and send
     ``ANSWER`` back to the companion.
#. If ``DIAG_REQ``: check for stale or duplicate sequence numbers
   (``bist_hd_seq_is_stale()``); if fresh, dispatch to
   ``bist_hd_audit_handle_diag()`` to execute the requested catalog audit
   and transmit ``DIAG_RSP``. ``BIST_HD_AUDIT_CPU`` runs
   ``bist_cpu_regs_test()``, ``BIST_HD_AUDIT_CSR`` runs
   ``bist_cpu_csr_regs_test()`` (trap CSRs only: the HP OS has already
   locked PMP/PMA, so those entries are not written). CPU and CSR run
   with the OS irq lock held. Unknown or disabled ``audit_id`` values
   return ``BIST_HD_STATUS_NOT_CONFIGURED``. Checkpoints are not flushed
   here; they ride the Q&A window only.
#. Unsupported or unknown incoming command frames (e.g.
   ``SAFE_STATE_NOTIFY``) are dropped.

On receive errors the worker yields for 1 ms to avoid spinning on a
high-priority thread.

LP BIST Status Bitmask
^^^^^^^^^^^^^^^^^^^^^^

The companion reports results as a bitmask via ``LP_STATUS`` messages:

.. list-table::
   :header-rows: 1
   :widths: 30 20 50

   * - Bit
     - Value
     - Meaning (bit set = test passed)
   * - ``BIST_HD_BIT_CPU_REG``
     - ``1 << 0``
     - CPU register test passed
   * - ``BIST_HD_BIT_CPU_CSR``
     - ``1 << 1``
     - CPU CSR test passed
   * - ``BIST_HD_BIT_RAM_A``
     - ``1 << 2``
     - RAM March-A test passed (runtime)
   * - ``BIST_HD_BIT_RAM_X``
     - ``1 << 3``
     - RAM March-X test passed (post-boot)
   * - ``BIST_HD_BIT_FLASH``
     - ``1 << 4``
     - Flash CRC test passed
   * - ``BIST_HD_BIT_STACK``
     - ``1 << 5``
     - Stack overflow check passed
   * - ``BIST_HD_BIT_ABRAHAM``
     - ``1 << 6``
     - Abraham time-division test passed
   * - ``BIST_HD_BIT_RUNTIME``
     - ``1 << 30``
     - Runtime phase flag
   * - ``BIST_HD_BIT_POSTBOOT``
     - ``1 << 31``
     - Post-boot phase flag

Safe State and LP WDT
^^^^^^^^^^^^^^^^^^^^^^

``bist_hd_safe_state()`` is a weak function. The default implementation:

#. Latches ``s_in_safe_state = 1`` (once latched, subsequent loop iterations are
   no-ops with no WDT feed).
#. Sends a best-effort ``SAFE_STATE_NOTIFY`` to HP.

Because the loop stops feeding the LP WDT, the watchdog fires
(``WDT_STAGE_ACTION_RESET_SYSTEM``) and resets the entire chip. This is the
**fail-closed** mechanism.

Triggers for safe state:

- Wrong or late Q&A answer.
- Answer arriving within the challenge window but exceeding the IRQ latency budget (``CONFIG_ESP_BIST_HD_IRQ_LATENCY_BUDGET_US``).
- Missed host checkpoint deadline (exceeding ``CONFIG_ESP_BIST_HD_CHECKPOINT_PERIOD_LOOPS`` consecutive loops without a checkpoint, including a host that never reports).
- Out-of-order or duplicate checkpoint ID (monotonicity violation or non-forward step).
- Post-boot or runtime LP BIST failure.
- Protocol / FFI violation (bad sequence, corrupted frame).
- Transport failure during init or runtime.
- ``AGENT_READY`` timeout or wrong frame type during init.

Applications may override the weak ``bist_hd_safe_state()`` to drive
product-specific cut-offs (GPIO, relay, power) before the WDT reset.

.. _hd-audit-catalog:

Host Audit Catalog and Implementation Status
---------------------------------------------

The companion schedules host audits from the catalog below. Product integration
enables the required subset via Kconfig.

The **Status** column distinguishes delivered mechanisms from declared
interfaces: a ``Declared`` audit has a protocol enum value. Its Kconfig
selector and companion schedule entry are added when that audit is implemented.

.. list-table::
   :header-rows: 1
   :widths: 20 20 35 25

   * - Audit
     - What it detects
     - Who executes
     - Status
   * - Q&A challenge / window
     - Stuck host, wrong path, compute faults
     - Companion issues; host answers
     - **Implemented** (``BIST_HD_AUDIT_QA``)
   * - Alive / deadline / logical checkpoints
     - Missed tasks, late work, wrong sequence
     - Companion checks; host reports checkpoints
     - **Implemented** (``BIST_HD_AUDIT_CHECKPOINT``)
   * - Host RAM March
     - DRAM bit faults
     - Host (quiesce) under companion trigger
     - Declared (``BIST_HD_AUDIT_RAM``); STL exists, HP region not wired
   * - Host flash / image CRC
     - Corrupted or wrong firmware
     - Host computes CRC; companion checks golden
     - Declared (``BIST_HD_AUDIT_FLASH``); golden injection not wired
   * - Host CPU register test
     - HP reg stuck-at / coupling
     - Host under companion trigger
     - **Implemented** (``BIST_HD_AUDIT_CPU``)
   * - Host CSR / PMP-PMA config audit
     - Bad privilege / memory protection config
     - Host under trigger
     - **Implemented** (``BIST_HD_AUDIT_CSR``) wired on HP OS
   * - Clock / crystal drift
     - Wrong time base (breaks FTTI)
     - Companion time-slot and/or host measure
     - Declared (``BIST_HD_AUDIT_CLOCK``)
   * - Host WDT path check
     - Soft WDT not armed / not fed correctly
     - Companion observes feed/ack pattern
     - Declared (``BIST_HD_AUDIT_WDT``)
   * - PC / program-flow sample
     - PC in unexpected region
     - Host under trigger
     - Declared (``BIST_HD_AUDIT_PC``)
   * - Interrupt latency / storm bound
     - Host not servicing within budget
     - Companion timestamps challenge IRQ response
     - **Implemented** (``BIST_HD_AUDIT_IRQ_LATENCY``)
   * - Peripheral / GPIO plausibility
     - Hazard outputs in illegal state
     - Companion reads or host reports via callback
     - Declared (``BIST_HD_AUDIT_GPIO``); requires app callback
   * - ADC / supply plausibility
     - Brownout, sensor nonsense
     - Host or companion ADC
     - Declared (``BIST_HD_AUDIT_ADC``); requires app callback
   * - Config / NVM / partition integrity
     - Bad calibration, wrong partition table
     - Host CRC under schedule
     - Declared (``BIST_HD_AUDIT_CONFIG_NVM``)
   * - IPC / shared-mem integrity
     - Corrupted HP-LP channel
     - Companion validates seq/CRC of protocol
     - Declared (``BIST_HD_AUDIT_IPC``)
   * - Secure-boot / image authenticity
     - Untrusted boot
     - Host reports secure-boot state
     - Declared (``BIST_HD_AUDIT_SECURE_BOOT``); requires app callback
   * - Dual-channel output compare
     - HP commanded actuate does not match safe intent
     - Companion compares or owns cut-off
     - Declared (``BIST_HD_AUDIT_DUAL_CHANNEL``); requires app callback

.. _hd-configuration:

Configuration Reference
------------------------

All Host Diagnostics options are in ``src/bist/Kconfig`` under the
``Host Diagnostics`` menu.

Master Enable
^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 40 10 10 40

   * - Option
     - Type
     - Default
     - Description
   * - ``CONFIG_ESP_BIST_HOST_DIAGNOSTICS``
     - bool
     - n
     - Master enable for companion supervision and the HP agent

Audit Selectors
^^^^^^^^^^^^^^^

Each implemented option enables one entry in the host-audit catalog.
``CONFIG_ESP_BIST_HD_AUDIT`` compiles the ``DIAG_REQ`` / ``DIAG_RSP``
schedule. Individual catalog audit selectors ``select`` that framework option
and compile the matching STL module into the HP image without
``IS_ULP_COCPU``.

Q&A, checkpoint, and IRQ latency stay independent of ``CONFIG_ESP_BIST_HD_AUDIT``.

.. list-table::
   :header-rows: 1
   :widths: 50 50

   * - Option
     - Audit
   * - ``CONFIG_ESP_BIST_HD_AUDIT_QA``
     - Q&A challenge / window
   * - ``CONFIG_ESP_BIST_HD_AUDIT_CHECKPOINT``
     - Alive / deadline / logical checkpoints (depends on ``CONFIG_ESP_BIST_HD_AUDIT_QA``)
   * - ``CONFIG_ESP_BIST_HD_AUDIT_IRQ_LATENCY``
     - Interrupt latency / storm bound (depends on ``CONFIG_ESP_BIST_HD_AUDIT_QA``)
   * - ``CONFIG_ESP_BIST_HD_AUDIT``
     - Master enable for catalog ``DIAG_REQ`` / ``DIAG_RSP`` (selected by
       individual catalog audit options)
   * - ``CONFIG_ESP_BIST_HD_AUDIT_CPU``
     - Host CPU register test (``bist_cpu_regs_test()`` on DIAG_REQ)
   * - ``CONFIG_ESP_BIST_HD_AUDIT_CSR``
     - Host CSR / PMP-PMA test (``bist_cpu_csr_regs_test()`` on DIAG_REQ)

Timing Parameters
^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 45 10 15 30

   * - Option
     - Type
     - Default
     - Description
   * - ``CONFIG_ESP_BIST_HD_CHALLENGE_WINDOW_US``
     - int
     - 10000
     - Max time for host to answer a Q&A challenge (us)
   * - ``CONFIG_ESP_BIST_HD_DIAG_TIMEOUT_US``
     - int
     - 50000
     - Max time for ``DIAG_REQ`` to ``DIAG_RSP`` (us); depends on
       ``CONFIG_ESP_BIST_HD_AUDIT``
   * - ``CONFIG_ESP_BIST_HD_IRQ_LATENCY_BUDGET_US``
     - int
     - 5000
     - Max acceptable Q&A round-trip time (us); must be less than the challenge
       window. Exceeding this with a correct answer triggers safe state
   * - ``CONFIG_ESP_BIST_HD_CHECKPOINT_PERIOD_LOOPS``
     - int
     - 1
     - Max consecutive companion loops without a host checkpoint before safe
       state. Applies from the first runtime loop, so a host that never
       reports still fails closed
   * - ``CONFIG_ESP_BIST_HD_AGENT_READY_TIMEOUT_US``
     - int
     - 1000000
     - Companion wait for ``AGENT_READY`` before safe state (us)
   * - ``CONFIG_ESP_BIST_HD_CHALLENGE_KEY``
     - hex
     - 0xA5A5A5A5
     - Secret key mixed into ``bist_hd_challenge_answer()``; override per
       product

Agent Thread / Task
^^^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 45 10 10 35

   * - Option
     - Type
     - Default
     - Description
   * - ``CONFIG_ESP_BIST_HD_AGENT_TASK_STACK``
     - int
     - 3072
     - Agent stack size (bytes); all OSes
   * - ``CONFIG_ESP_BIST_HD_AGENT_TASK_PRIO``
     - int
     - 23
     - FreeRTOS priority (ESP-IDF only)
   * - ``CONFIG_ESP_BIST_HD_AGENT_THREAD_COOP_PRIO``
     - int
     - 1
     - ``K_PRIO_COOP()`` index (Zephyr only)
   * - ``CONFIG_ESP_BIST_HD_AGENT_TASK_PRIO_NUTTX``
     - int
     - 200
     - ``SCHED_FIFO`` priority (NuttX only)

.. _hd-verification:

Verification Evidence
---------------------

Off-Target Unit Tests
^^^^^^^^^^^^^^^^^^^^^

**Protocol / challenge unit test** (``tests/unit/hd_protocol/``):

Validates encode/decode round-trip, CRC-32, and the challenge answer function.
Runs host-native via ``cmake`` + ``ctest``; no device required.

**Companion verdict matrix** (``tests/unit/hd_companion/``):

Drives the real ``bist_hd_companion.c`` state machine against a scripted fake
agent. 25 scenarios cover the judgment logic without hardware:

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Scenario
     - What it verifies
   * - ``ready``
     - Normal ``AGENT_READY`` handshake accepted
   * - ``no_ready``
     - Timeout when no ``AGENT_READY`` arrives
   * - ``ready_wrong_type``
     - Companion rejects non-``AGENT_READY`` frame during init
   * - ``postboot_fail``
     - Post-boot BIST failure triggers safe state
   * - ``qa_pass``
     - Correct Q&A answer accepted
   * - ``qa_wrong_value``
     - Wrong answer value triggers safe state
   * - ``qa_wrong_seq``
     - Wrong sequence number triggers safe state
   * - ``qa_wrong_type``
     - Non-``ANSWER`` response triggers safe state
   * - ``qa_silent``
     - No response (timeout) triggers safe state
   * - ``qa_late``
     - Answer arrives after window expires
   * - ``safe_state_latched``
     - Once in safe state, no further WDT feeds or challenges
   * - ``runtime_fail``
     - Runtime BIST failure triggers safe state
   * - ``checkpoint_pass``
     - Checkpoints arriving every loop keep the companion running
   * - ``checkpoint_missing``
     - Consecutive loops without a checkpoint trigger safe state
   * - ``checkpoint_never``
     - Host that never sends checkpoints triggers safe state on deadline
   * - ``checkpoint_out_of_order``
     - Decreasing checkpoint ID triggers safe state
   * - ``checkpoint_wrap``
     - Checkpoint ID wrapping past ``UINT32_MAX`` is accepted (not a false
       out-of-order)
   * - ``irq_latency_over_budget``
     - Correct answer exceeding the IRQ latency budget triggers safe state
   * - ``diag_ok``
     - Passing ``DIAG_RSP`` accepted across multiple runtime rounds
   * - ``diag_fail``
     - ``DIAG_RSP`` returning ``STATUS_FAIL`` triggers safe state
   * - ``diag_silent``
     - Missing host response to ``DIAG_REQ`` (timeout) triggers safe state
   * - ``diag_bad_seq``
     - Mismatched sequence number in ``DIAG_RSP`` triggers safe state
   * - ``diag_wrong_type``
     - Non-``DIAG_RSP`` frame on diagnostic request triggers safe state
   * - ``diag_not_configured``
     - ``STATUS_NOT_CONFIGURED`` response triggers safe state
   * - ``diag_late``
     - ``DIAG_RSP`` exceeding the diagnostic timeout window triggers safe state

Agent Checkpoint and Audit Dispatch (``tests/unit/hd_agent/``)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Host-native tests for the HP agent's private checkpoint counter and command
dispatch. Transport and platform adapters are stubbed; only the agent logic is
verified.

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Scenario
     - What it proves
   * - ``checkpoint_first_id``
     - First call emits payload 1 (never 0, avoiding confusion with zeroed memory)
   * - ``checkpoint_step_is_one``
     - Five consecutive calls produce payloads whose unsigned step is exactly 1
   * - ``checkpoint_send_failure_consumes_id``
     - A failed transport send still consumes the ID (gap of 2), so no duplicate
       reaches the companion after a partial frame error
   * - ``diag_req_handled``
     - Incoming ``DIAG_REQ`` is dispatched to ``bist_hd_audit_handle_diag()``

On-Target Fail-Closed Tests
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**ESP-IDF** (``tests/integration/hd_idf/``):

The test application makes the host misbehave; the companion enters safe state;
the LP WDT resets the chip. Three configurations:

- ``hd_key_mismatch``: HP is built with ``BIST_HD_CHALLENGE_KEY=0x5A5A5A5A``
  (a key the LP companion does not share), so the production answer path
  computes a value the companion rejects.
- ``hd_starved_agent``: HP withholds its agent past the challenge window
  (``CONFIG_BIST_HD_TEST_STARVE_AGENT``). The library creates the agent
  unpinned, so the app holds every HP core above the agent priority; on a
  multi-core target, starving one core alone would let the agent answer from
  another.
- ``hd_skip_checkpoint``: HP answers Q&A challenges correctly but never calls
  ``bist_hd_checkpoint_reached()`` (``CONFIG_BIST_HD_TEST_SKIP_CHECKPOINT``).
  The companion's checkpoint deadline is the only path to safe state, proving
  the checkpoint audit alone triggers the fail-closed reaction.

The pytest (``pytest_device_hd_idf.py``) asserts:

#. ``test_HD_agent_ready:PASS``
#. ``test_BIST_postboot:PASS``
#. ``test_HD_fail_closed_armed:PASS``
#. ``CPU has been reset by WDT``

Targets: ESP32-C5, ESP32-C6, ESP32-P4.

**Zephyr** (``tests/integration/hd_zephyr/``):

Twister test cases:

- ``bist.hd.selftest`` -- happy path via ztest.
- ``bist.hd.reset_cause`` -- starvation triggers reboot; second boot checks
  ``hwinfo`` for ``RESET_WATCHDOG``.
- ``bist.hd.key_mismatch`` -- HP key mismatch causes WDT reset.
- ``bist.hd.starved_agent`` -- starvation causes WDT reset (console harness).
- ``bist.hd.skip_checkpoint`` -- HP answers Q&A correctly but never sends
  checkpoints; checkpoint deadline alone causes WDT reset (console harness).

Platforms: ESP32-C5, ESP32-C6, ESP32-P4.

**NuttX** (``tests/integration/hd_nuttx/``):

The same three faults as ESP-IDF (``hd_key_mismatch``, ``hd_starved_agent``,
``hd_skip_checkpoint``), built as a NuttX custom-apps tree and flashed
as ``nuttx.merged.bin``. Pytest asserts:

#. ``test_HD_agent_ready:PASS``
#. ``test_BIST_postboot:PASS``
#. ``test_HD_fail_closed_armed:PASS``
#. ROM reset-reason line containing ``WDT``

Targets: ESP32-C6, ESP32-P4.

**Fail-closed stimulus lives in the test application, not in the library.** The
shipped library contains no test-only branches: a fail-closed build and a
product build differ only in the test app's code and Kconfig. If a
``CONFIG_*_INJECT_*`` option or an ``inject_fault`` hook reappears under
``src/bist/``, the gate no longer proves anything about the shipped library.

The LP companion image is identical in the samples and in the test apps: it is
the element under test, so it is always built exactly as a product would build
it. Every difference is on the HP side.

Per-Platform Verification Depth
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 20 40 40

   * - Platform
     - Device-tested evidence
     - Coverage scope
   * - ESP-IDF
     - pytest happy-path + fail-closed (key mismatch, starved agent, skip
       checkpoint); targets ESP32-C5, C6, P4
     - Q&A, checkpoint, IRQ latency, CPU/CSR — pass; fail-closed via
       starve (DIAG timeout) and unit ``diag_fail`` (FAIL payload)
   * - Zephyr
     - Twister ztest + console harness (selftest, reset_cause, key_mismatch,
       starved_agent, skip_checkpoint); targets ESP32-C5, C6, P4
     - Q&A, checkpoint, IRQ latency, CPU/CSR — pass; fail-closed via
       starve (DIAG timeout) and unit ``diag_fail`` (FAIL payload)
   * - NuttX
     - pytest happy-path + fail-closed (key mismatch, starved agent, skip
       checkpoint); targets ESP32-C6, P4
     - Q&A, checkpoint, IRQ latency, CPU/CSR — pass; fail-closed via
       starve (DIAG timeout) and unit ``diag_fail`` (FAIL payload)

Healthy silicon does not produce a CPU/CSR ``STATUS_FAIL`` payload, so
device fail-closed for those audits is a missing ``DIAG_RSP`` (the existing
starve configuration). Late answers, wrong sequence, wrong type, and FAIL
payload remain unit-only on every platform.

.. _hd-references:

References
----------

Internal
^^^^^^^^

- :doc:`software_architecture` -- standalone-centric architecture overview
- :doc:`module_design_and_coding` -- STL algorithm design (CPU, RAM, flash,
  stack, clock, etc.)
- :doc:`software_safety_requirements` -- IEC 60730 mapping for the library
- ``src/bist/include/bist_hd_protocol.h`` -- shared protocol header
- ``src/bist/bist_hd_challenge.c`` -- Q&A answer function
- ``src/bist/companion/bist_hd_companion.c`` -- companion state machine
- ``src/bist/host/bist_hd_agent.c`` -- agent worker loop
- ``src/bist/Kconfig`` -- Host Diagnostics configuration
- ``samples/idf/`` -- IDF sample
- ``samples/zephyr/`` -- Zephyr sample
- ``samples/nuttx/nuttx_bist/`` -- NuttX sample
- ``tests/integration/hd_idf/`` -- IDF fail-closed validation
- ``tests/integration/hd_zephyr/`` -- Zephyr fail-closed validation
- ``tests/integration/hd_nuttx/`` -- NuttX fail-closed validation
- ``tests/unit/hd_agent/`` -- off-target agent checkpoint counter tests
- ``tests/unit/hd_companion/`` -- off-target companion verdict matrix
- ``tests/unit/hd_protocol/`` -- protocol and challenge unit tests

External
^^^^^^^^

- IEC 60730-1 Annex H -- Class B control themes (program sequence, time-slot,
  variable/invariable memory)
- IEC 61508 -- independent and diverse channels, diagnostic coverage (when
  industrial SIL claims are in scope; used for independence vocabulary)
- ISO 26262 -- FFI and mixed-criticality framing (when automotive claims are in
  scope; used for independence vocabulary)

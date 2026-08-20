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
  companion is present, answers Q&A challenges, and will in future phases
  execute companion-triggered host audits (RAM March on HP DRAM, host image
  CRC, CPU/CSR/stack/clock checks, and more).
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
CPU/CSR/stack checks, and more -- see :ref:`hd-audit-catalog`) so the
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
     - Defined, not handled
   * - ``DIAG_RSP`` (5)
     - HP to LP
     - Pass/fail + diagnostic id + sequence
     - Defined, not handled
   * - ``CHECKPOINT`` (6)
     - HP to LP
     - Alive / deadline / logical checkpoint
     - Defined, not handled
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
     - 50 000 us (defined, not referenced in code)
   * - Generic tick compare
     - ``bist_hd_timeout_expired()``
     - Unsigned wrap-safe comparison

``CONFIG_ESP_BIST_HD_DIAG_TIMEOUT_US`` is defined in Kconfig but not yet
referenced by the companion or agent. It will be used once ``DIAG_REQ`` /
``DIAG_RSP`` handling is implemented.

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

Each iteration:

#. If already in safe state: return immediately (no WDT feed).
#. ``lp_wdt_feed()`` -- feed the watchdog.
#. Run runtime LP BIST (CPU regs, CPU CSR, RAM March-A + Abraham, stack check).
   OR the ``BIST_HD_BIT_RUNTIME`` flag.
#. Send ``LP_STATUS`` with the runtime bitmask.
#. Verify expected runtime bits. If not: ``bist_hd_safe_state()``.
#. If ``CONFIG_ESP_BIST_HD_AUDIT_QA``: run Q&A challenge -- send ``CHALLENGE``,
   recv ``ANSWER`` within ``CONFIG_ESP_BIST_HD_CHALLENGE_WINDOW_US``, verify seq
   + value + elapsed time. If any check fails: ``bist_hd_safe_state()``.

The application-side LP main loop calls ``bist_hd_companion_loop()`` with a
delay between iterations (e.g. ``bist_hd_comp_port_delay_us(10000)``).

Agent Worker (``bist_hd_agent_start``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

On the HP side, ``bist_hd_agent_start()`` performs a one-shot setup:

#. ``bist_hd_platform_init()`` -- create the LP-status queue.
#. ``bist_hd_transport_init()`` -- open the mailbox (IDF / Zephyr mbox /
   NuttX ``/dev/lp_mailbox``).
#. ``bist_hd_transport_flush()`` -- discard mailbox state left over from a
   previous HP session. The LP companion keeps running across an HP reset, so
   pending words or frames that predate ``AGENT_READY`` must be dropped. IDF
   drains with a bounded non-blocking receive loop, Zephyr purges its RX
   message queue, and NuttX defers to a byte-level header resync inside
   ``bist_hd_transport_recv()`` because the driver lacks non-blocking reads.
#. Send ``AGENT_READY`` to the companion.
#. Start a high-priority worker thread (FreeRTOS task / Zephyr cooperative
   thread / NuttX ``SCHED_FIFO`` pthread).

The worker thread runs an infinite receive loop:

#. Block on ``bist_hd_transport_recv()`` (indefinite timeout).
#. If ``LP_STATUS``: push the bitmask into the platform queue so the
   application can retrieve it via ``bist_hd_agent_wait_lp_status()``.
#. If ``CHALLENGE``: check for stale or duplicate sequence numbers
   (``bist_hd_seq_is_stale()``); if fresh, compute the answer via
   ``bist_hd_challenge_answer()`` and send ``ANSWER`` back to the companion.
#. All other message types are silently dropped in this revision
   (``DIAG_REQ``, ``CHECKPOINT``, and ``SAFE_STATE_NOTIFY`` handling is
   deferred).

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
- Post-boot or runtime LP BIST failure.
- Protocol / FFI violation (bad sequence, corrupted frame).
- Transport failure during init or runtime.
- ``AGENT_READY`` timeout or wrong frame type during init.

Applications may override the weak ``bist_hd_safe_state()`` to drive
product-specific cut-offs (GPIO, relay, power) before the WDT reset. See
:ref:`hd-safe-state-override`.

.. _hd-audit-catalog:

Host Audit Catalog and Implementation Status
---------------------------------------------

The companion schedules host audits from the catalog below. Product integration
enables the required subset via Kconfig.

The **Status** column distinguishes delivered mechanisms from declared
interfaces: a ``Declared`` audit has a protocol enum value and a Kconfig symbol
but no wired runtime path in this revision.

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
     - Declared (``BIST_HD_AUDIT_CHECKPOINT``); stub returns -1
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
     - Declared (``BIST_HD_AUDIT_CPU``)
   * - Host CSR / PMP-PMA config audit
     - Bad privilege / memory protection config
     - Host under trigger; companion checks config
     - Declared (``BIST_HD_AUDIT_CSR``)
   * - Stack overflow / canary / watermark
     - Stack smash, stack pressure
     - Host periodic + companion demand
     - Declared (``BIST_HD_AUDIT_STACK``)
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
     - Declared (``BIST_HD_AUDIT_IRQ_LATENCY``)
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

Each option enables one entry in the host-audit catalog. Enabling an audit that
depends on an STL test (e.g. ``ESP_BIST_HD_AUDIT_RAM`` selects
``ESP_BIST_MEMORY_RAM_TEST``) automatically pulls in the corresponding STL
module.

.. list-table::
   :header-rows: 1
   :widths: 50 50

   * - Option
     - Audit
   * - ``CONFIG_ESP_BIST_HD_AUDIT_QA``
     - Q&A challenge / window
   * - ``CONFIG_ESP_BIST_HD_AUDIT_CHECKPOINT``
     - Alive / deadline / logical checkpoints
   * - ``CONFIG_ESP_BIST_HD_AUDIT_RAM``
     - Host RAM March (selects ``ESP_BIST_MEMORY_RAM_TEST``)
   * - ``CONFIG_ESP_BIST_HD_AUDIT_FLASH``
     - Host flash CRC (selects ``ESP_BIST_MEMORY_FLASH_TEST``)
   * - ``CONFIG_ESP_BIST_HD_AUDIT_CPU``
     - Host CPU register test (selects ``ESP_BIST_CPU_REG_TEST``)
   * - ``CONFIG_ESP_BIST_HD_AUDIT_CSR``
     - Host CSR audit (selects ``ESP_BIST_CPU_CSR_REG_TEST``)
   * - ``CONFIG_ESP_BIST_HD_AUDIT_STACK``
     - Stack overflow / canary (selects ``ESP_BIST_STACK_TEST``)
   * - ``CONFIG_ESP_BIST_HD_AUDIT_CLOCK``
     - Clock / crystal drift
   * - ``CONFIG_ESP_BIST_HD_AUDIT_WDT``
     - Host WDT path check
   * - ``CONFIG_ESP_BIST_HD_AUDIT_PC``
     - PC / program-flow sample
   * - ``CONFIG_ESP_BIST_HD_AUDIT_IRQ_LATENCY``
     - Interrupt latency / storm bound
   * - ``CONFIG_ESP_BIST_HD_AUDIT_GPIO``
     - GPIO plausibility (requires app callback)
   * - ``CONFIG_ESP_BIST_HD_AUDIT_ADC``
     - ADC plausibility (requires app callback)
   * - ``CONFIG_ESP_BIST_HD_AUDIT_CONFIG_NVM``
     - Config / NVM integrity
   * - ``CONFIG_ESP_BIST_HD_AUDIT_IPC``
     - IPC / shared-mem integrity
   * - ``CONFIG_ESP_BIST_HD_AUDIT_SECURE_BOOT``
     - Secure-boot status (requires app callback)
   * - ``CONFIG_ESP_BIST_HD_AUDIT_DUAL_CHANNEL``
     - Dual-channel output compare (requires app callback)

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
     - Max time for ``DIAG_REQ`` to ``DIAG_RSP`` (us); defined, not referenced
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

.. _hd-integration-guide:

Integration Guide
-----------------

ESP-IDF
^^^^^^^

**Required Kconfig** (``sdkconfig.defaults``):

.. code-block:: none

   CONFIG_ULP_COPROC_ENABLED=y
   CONFIG_ULP_COPROC_TYPE_LP_CORE=y
   CONFIG_ULP_COPROC_RESERVE_MEM=16240
   CONFIG_ESP_BIST_HOST_DIAGNOSTICS=y
   CONFIG_ESP_BIST_HD_AUDIT_QA=y

Plus the LP BIST test options (``CONFIG_ESP_BIST_CPU_REG_TEST``,
``CONFIG_ESP_BIST_CPU_CSR_REG_TEST``, ``CONFIG_ESP_BIST_MEMORY_RAM_TEST``,
``CONFIG_ESP_BIST_MEMORY_FLASH_TEST``, ``CONFIG_ESP_BIST_STACK_TEST``).

**Build wiring:**

The HP application registers ``esp-bist`` as a component (it is the repo root
``CMakeLists.txt``). The LP sub-project is added via ``ulp_add_project()`` in
the application ``main/CMakeLists.txt``:

.. code-block:: cmake

   idf_component_register(SRCS ${app_sources}
                          REQUIRES ulp esp-bist
                          WHOLE_ARCHIVE)
   ulp_add_project("ulp_bist_sample" "${CMAKE_CURRENT_LIST_DIR}/ulp/")

Inside ``ulp/CMakeLists.txt``, the LP main links the BIST library:

.. code-block:: cmake

   add_subdirectory(${BIST_PATH}/src/bist ${CMAKE_CURRENT_BINARY_DIR}/bist)
   target_link_libraries(${ULP_APP_NAME} PRIVATE bist_esp)

**LP entry point** (``ulp/main.c``):

.. code-block:: c

   #include "bist_hd_companion.h"
   #include "bist_hd_comp_port.h"

   #define RUNTIME_INTERVAL_US 10000

   int main(void)
   {
       if (bist_hd_companion_init() != 0) {
           return 0;
       }
       while (1) {
           bist_hd_companion_loop();
           bist_hd_comp_port_delay_us(RUNTIME_INTERVAL_US);
       }
   }

**HP integration** (``main.c``):

.. code-block:: c

   #include "bist_hd_agent.h"
   #include "bist_hd_protocol.h"

   /* After loading and starting the LP core: */
   vTaskDelay(pdMS_TO_TICKS(100));   /* mailbox settle */

   if (bist_hd_agent_start() != 0) {
       /* handle error */
   }

   uint32_t status;
   /* Wait for post-boot LP BIST status */
   bist_hd_agent_wait_lp_status(&status, BIST_HD_BIT_POSTBOOT, 10000);
   /* Wait for runtime LP BIST status (repeat in a loop) */
   bist_hd_agent_wait_lp_status(&status, BIST_HD_BIT_RUNTIME, 10000);

See ``samples/idf/`` for the complete working sample.

Zephyr
^^^^^^

Zephyr uses sysbuild with a dual-image layout (hpcore + lpcore).

**Required Kconfig** (HP ``prj.conf``):

.. code-block:: none

   CONFIG_MBOX=y
   CONFIG_ESP32_ULP_COPROC_ENABLED=y
   CONFIG_ESP_BIST_HOST_DIAGNOSTICS=y
   CONFIG_ESP_BIST_HD_AUDIT_QA=y

LP ``remote/prj.conf`` additionally sets the BIST test options and timing:

.. code-block:: none

   CONFIG_ESP_BIST_HOST_DIAGNOSTICS=y
   CONFIG_ESP_BIST_HD_AUDIT_QA=y
   CONFIG_ESP_BIST_HD_AGENT_READY_TIMEOUT_US=5000000

**Sysbuild:**

``sysbuild.cmake`` adds the LP remote project:

.. code-block:: cmake

   ExternalZephyrProject_Add(
       APPLICATION zephyr_bist_remote
       SOURCE_DIR ${APP_DIR}/remote
       BOARD ${SB_CONFIG_ULP_REMOTE_BOARD}
   )
   sysbuild_add_dependencies(FLASH zephyr_bist_remote ${DEFAULT_IMAGE})

``Kconfig.sysbuild`` maps boards to lpcore variants:

.. code-block:: none

   config ULP_REMOTE_BOARD
       string
       default "esp32c5_devkitc/esp32c5/lpcore" if $(BOARD) = "esp32c5_devkitc"
       default "esp32c6_devkitc/esp32c6/lpcore" if $(BOARD) = "esp32c6_devkitc"

**Devicetree overlays:**

HP overlay (e.g. ``boards/esp32c6_devkitc_esp32c6_hpcore.overlay``):

.. code-block:: dts

   / {
       chosen {
           zephyr,ipc_shm = &ipc_shm;
           zephyr,ipc = &mbox0;
       };
       mbox-consumer {
           compatible = "vnd,mbox-consumer";
           mboxes = <&mbox0 0>, <&mbox0 1>;
           mbox-names = "tx", "rx";
       };
   };
   &mbox0 {
       shared-memory-size = <0x20>;
   };

LP overlay has identical nodes with **tx/rx channels swapped**:
``mboxes = <&mbox0 1>, <&mbox0 0>``.

**HP integration** (``src/main.c``):

.. code-block:: c

   k_msleep(200);  /* LP boot settle */
   bist_hd_agent_start();
   bist_hd_agent_wait_lp_status(&status, BIST_HD_BIT_POSTBOOT, 10000);

See ``samples/zephyr/`` for the complete working sample.

NuttX
^^^^^

**Required Kconfig** (config fragment):

.. code-block:: none

   CONFIG_ESPRESSIF_LP_MAILBOX=y
   CONFIG_ESPRESSIF_LP_UART=y
   CONFIG_ESP_BIST_HOST_DIAGNOSTICS=y
   CONFIG_ESP_BIST_HD_AUDIT_QA=y

**ULP image build:**

``CMakeLists.txt`` conditionally includes the ULP build when
``CONFIG_ESPRESSIF_USE_LP_CORE`` is set, using NuttX ``esp_ulp.cmake``. The
BIST library is added to ``ULP_APP_C_SRCS`` via ``nuttx.cmake`` or
``nuttx.mk``.

**HP integration** (``nuttx_bist_main.c``):

.. code-block:: c

   /* Load LP firmware */
   int ulp_fd = open("/dev/ulp", O_WRONLY);
   write(ulp_fd, nuttx_bist_bin, nuttx_bist_bin_len);
   close(ulp_fd);

   sleep(1);  /* mailbox settle */

   bist_hd_agent_start();
   bist_hd_agent_wait_lp_status(&status, BIST_HD_BIT_POSTBOOT, 10000);

The agent transport opens ``/dev/lp_mailbox`` internally (provided by
``CONFIG_ESPRESSIF_LP_MAILBOX``).

See ``samples/nuttx/nuttx_bist/`` for the complete working sample.

.. _hd-safe-state-override:

Overriding Safe State
^^^^^^^^^^^^^^^^^^^^^

``bist_hd_safe_state()`` is declared ``__attribute__((weak))``. To drive a
product-specific cut-off before the LP WDT fires, provide a strong definition:

.. code-block:: c

   void bist_hd_safe_state(void)
   {
       /* De-energize hazard outputs, open relay, etc. */
       gpio_set_level(HAZARD_CUT_PIN, 0);
       /* Then let the LP WDT reset the system. */
   }

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
agent. 13 scenarios cover the judgment logic without hardware:

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

On-Target Fail-Closed Tests
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**ESP-IDF** (``tests/integration/hd_idf/``):

The test application makes the host misbehave; the companion enters safe state;
the LP WDT resets the chip. Two configurations:

- ``hd_key_mismatch``: HP is built with ``BIST_HD_CHALLENGE_KEY=0x5A5A5A5A``
  (a key the LP companion does not share), so the production answer path
  computes a value the companion rejects.
- ``hd_starved_agent``: HP withholds its agent past the challenge window
  (``CONFIG_BIST_HD_TEST_STARVE_AGENT``). The library creates the agent
  unpinned, so the app holds every HP core above the agent priority; on a
  multi-core target, starving one core alone would let the agent answer from
  another.

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

Platforms: ESP32-C5, ESP32-C6.

**NuttX** (``tests/integration/hd_nuttx/``):

The same two faults as ESP-IDF, built as a NuttX custom-apps tree and flashed
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
- ``src/bist/Kconfig`` -- Host Diagnostics configuration (lines 189-377)
- ``samples/idf/`` -- IDF sample (agent/companion, Q&A)
- ``samples/zephyr/`` -- Zephyr sample
- ``samples/nuttx/nuttx_bist/`` -- NuttX sample
- ``tests/integration/hd_idf/`` -- IDF fail-closed validation
- ``tests/integration/hd_zephyr/`` -- Zephyr fail-closed validation
- ``tests/integration/hd_nuttx/`` -- NuttX fail-closed validation
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

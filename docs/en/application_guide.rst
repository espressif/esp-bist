Application Guide
=================

Overview
--------

This guide explains how to integrate and use the ESP-BIST library in a safety-critical application. The standalone sample application (``samples/standalone/``) demonstrates a complete integration pattern that follows IEC 60730-1 Annex H (Class B) best practices.

The standalone application serves as a reference implementation showing:

- **Post-boot self-tests**: Comprehensive hardware integrity checks executed once at startup
- **Runtime monitoring**: Continuous CPU and stack integrity checks during normal operation
- **Watchdog integration**: Windowed watchdog timer for timing fault detection
- **Fail-safe handling**: Appropriate error response and safe state transition
- **GPIO validation**: I/O functionality verification

Application Structure
---------------------

The standalone application follows a structured execution flow designed to maximize safety:

.. blockdiag::
    :scale: 100%
    :caption: Standalone Application Execution Flow
    :align: center

    blockdiag {
        Boot -> "Post-Boot Tests" -> "Initialize Stack Sentinel";
        "Post-Boot Tests" -> "Fail-Safe Exit" [label = "Error Detected"];
        "Initialize Stack Sentinel" -> "Register WDT Callbacks";
        "Register WDT Callbacks" -> "Configure GPIO";
        "Configure GPIO" -> "Initialize WDT";
        "Initialize WDT" -> "Main Loop";
        "Main Loop" -> "Application Logic";
        "Application Logic" -> "Runtime Tests" -> "WDT Feed";
        "Runtime Tests" -> "Fail-Safe Exit" [label = "Error Detected"];
        "WDT Feed" -> "Main Loop";

        Boot [shape = roundedbox];
        "Post-Boot Tests" [shape = box];
        "Fail-Safe Exit" [shape = box];
        "Initialize Stack Sentinel" [shape = box];
        "Register WDT Callbacks" [shape = box];
        "Runtime Tests" [shape = box];
        "Configure GPIO" [shape = box];
        "Initialize WDT" [shape = box];
        "Main Loop" [shape = box];
        "Application Logic" [shape = box];
        "WDT Feed" [shape = box];
    }

Components and Rationale
------------------------

Post-Boot Self-Tests
^^^^^^^^^^^^^^^^^^^^

**Purpose**: Verify hardware integrity before entering normal operation. These tests are executed once at startup and must pass before the application proceeds.

**Rationale**: IEC 60730-1 Annex H requires that critical hardware components be validated at startup to ensure the system begins operation in a known-good state.

**Tests Executed**:

1. **Watchdog Test** (``bist_wdt_test()``)

   - **IEC 60730 Component**: 6.3 (Timing)
   - **Purpose**: Verifies watchdog timer functionality through a two-boot sequence
   - **Rationale**: Watchdog is the ultimate safety mechanism; it must be verified before relying on it for runtime monitoring
   - **Implementation**: First boot triggers WDT timeout, second boot verifies reset reason

2. **External Crystal Test** (``bist_ext_crystal_fail_test()``)

   - **IEC 60730 Component**: 3 (Clock)
   - **Purpose**: Validates 32.768 kHz external crystal oscillator via XT WDT (on SoCs with ``SOC_XT_WDT_SUPPORTED``)
   - **Rationale**: Low-frequency crystal is critical for RTC and timing functions; failure must be detected immediately
   - **Implementation**: Registers XT WDT callback; test passes if no callback fires within timeout window. On SoCs without XT WDT hardware, the test is skipped and returns ``BIST_ESP_OK``.

3. **Main Crystal Test** (``bist_main_crystal_test()``)

   - **IEC 60730 Component**: 3 (Clock)
   - **Purpose**: Validates 40 MHz main crystal oscillator frequency
   - **Rationale**: Main clock accuracy is essential for all timing-critical operations
   - **Implementation**: Measures frequency ratio against 32 kHz reference; fails if drift exceeds ``CONFIG_ESP_BIST_CLOCK_PERCENT_FREQUENCY_DRIFT`` (default: ±1%)

4. **RAM Test** (``bist_ram_test_march_x()`` / ``bist_ram_test_abraham_full()``)

   - **IEC 60730 Component**: 4.2 (Variable Memory)
   - **Purpose**: Detects RAM stuck-at, coupling, transition, and address decoder faults
   - **Rationale**: RAM integrity is critical; corruption can lead to unpredictable behavior
   - **Implementation**: March X for Class B coverage; Abraham (H.2.19.1) with time-division
     partition pairs for Class C–level coupling fault detection across the full RAM region

5. **Flash CRC Test** (``bist_flash_test()``)

   - **IEC 60730 Component**: 4.1 (Invariable Memory)
   - **Purpose**: Validates flash memory integrity via CRC32 checksums
   - **Rationale**: Flash corruption can cause code execution errors or data corruption
   - **Implementation**: Computes CRC32 over ``.flash.text`` and ``.flash.rodata`` sections; compares against post-build injected checksums

6. **Stack Overflow Stress Test** (``bist_cpu_stack_overflow_test()``)

   - **IEC 60730 Component**: 4.2 (Variable Memory)
   - **Purpose**: Verifies stack overflow detection mechanism through intentional overflow
   - **Rationale**: Stack overflow detection must be validated to ensure it works when needed
   - **Implementation**: Deep recursion corrupts stack sentinel; test verifies detection

7. **GPIO Output Test** (``bist_gpio_output_test(LED_GPIO)``)

   - **IEC 60730 Component**: 7.1 (Digital I/O)
   - **Purpose**: Verifies GPIO output functionality
   - **Rationale**: I/O functionality must be validated before use in safety-critical applications
   - **Implementation**: Sets GPIO high/low and reads back to verify state

8. **GPIO Input Test** (``bist_gpio_input_test(BTN_GPIO, 1)``)

   - **IEC 60730 Component**: 7.1 (Digital I/O)
   - **Purpose**: Verifies GPIO input functionality
   - **Rationale**: Input validation ensures correct reading of external signals
   - **Implementation**: Reads GPIO level and verifies it matches expected value (button tied to GND = 0)

9. **ADC Low Level Test** (``bist_adc_low_level_test(ADC_UNIT_1, ADC_CHANNEL_2)``)

   - **IEC 60730 Component**: 7.2 (Analog I/O)
   - **Purpose**: Verifies ADC reads near zero with internal pull-down
   - **Rationale**: Analog input functionality must be validated before use in safety-critical applications
   - **Implementation**: Configures channel, enables pull-down, reads raw value within tolerance of zero

10. **ADC High Level Test** (``bist_adc_high_level_test(ADC_UNIT_1, ADC_CHANNEL_2)``)

    - **IEC 60730 Component**: 7.2 (Analog I/O)
    - **Purpose**: Verifies ADC reads near full scale with internal pull-up
    - **Rationale**: Detects stuck-at-low faults and verifies ADC conversion path
    - **Implementation**: Configures channel, enables pull-up, reads raw value within tolerance of SoC-specific high reference

11. **ADC Reference Test** (``bist_adc_reference_test(ADC_UNIT_1, ADC_CHANNEL_2)``) — ESP32-C3 only

    - **IEC 60730 Component**: 7.2 (Analog I/O)
    - **Purpose**: Verifies mid-scale ADC reading using internal VREF biasing
    - **Rationale**: Detects faults that may pass pull-up/pull-down tests alone
    - **Implementation**: Enables VREF output, reads raw value within tolerance of reference level

12. **Software Interrupt Source Map Test** (``bist_interrupt_source_map_test()``)

    - **IEC 60730 Component**: 2 (Interrupt handling and execution)
    - **Purpose**: Verifies interrupt-matrix routing and ISR delivery for software IRQ sources ``CPU_INTR_FROM_CPU_0..3``
    - **Rationale**: Stuck or mis-routed interrupts can prevent fault handlers and safety responses from running
    - **Implementation**: Maps each software source to CPU interrupt 9, triggers eight times, checks enable mask and ISR count, then unmaps. Enabled when ``CONFIG_ESP_BIST_INTERRUPT_TEST`` is set (standalone builds).

13. **Hardware Interrupt Test** (``bist_hardware_interrupt_test()``)

    - **IEC 60730 Component**: 2 (Interrupt handling and execution)
    - **Purpose**: Verifies concurrent hardware interrupt delivery via dual timer-group alarms
    - **Rationale**: Incorrect interrupt frequency or missed peripheral IRQs can break timing-critical safety functions
    - **Implementation**: TIMG0 (500 µs) and TIMG1 (1000 µs) on CPU interrupts 9 and 10; checks 2:1 ISR count ratio with ±1 tolerance. Post-boot only; requires two Timer Groups.

**Error Handling**: All post-boot tests call ``fail_safe_exit()`` on failure, which enters an infinite loop. This prevents the application from continuing with detected hardware faults.

Stack Sentinel Initialization
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Function**: ``bist_cpu_stack_overflow_init()``

**Purpose**: Initializes stack overflow detection by filling a protection region with sentinel pattern ``0xDEADBEEF``.

**Rationale**: Stack overflow is a common cause of system failures. Early detection allows the application to respond before memory corruption affects other system components.

**Implementation**: Fills every word in the half-open region ``[_stack_overflow_protection_end, _stack_overflow_protection_start)`` with the sentinel pattern. The region size is set by ``CONFIG_ESP_BIST_STACK_PROTECTION_BLOCK_SIZE``. The word at ``_stack_overflow_protection_start`` itself is live stack and is not written. This pattern is checked periodically during runtime.

Watchdog Callback Registration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Functions**:

- ``esp_xt_wdt_register_callback((esp_xt_callback_t)fail_safe_exit, NULL)`` (on SoCs with ``SOC_XT_WDT_SUPPORTED``)
- ``wdt_register_callback(wdt_callback, NULL)``

**Purpose**: Registers callbacks for watchdog events to enable fail-safe response.

**Rationale**:

- **XT WDT Callback**: External crystal failure is a critical fault requiring immediate safe state transition (available on SoCs with XT WDT hardware, e.g. ESP32-C3)
- **MWDT Callback**: Master watchdog callback provides notification before reset (if needed for logging or state saving)

**Implementation**:

- XT WDT callback directly calls ``fail_safe_exit()`` for crystal failures (guarded by ``SOC_XT_WDT_SUPPORTED`` at compile time)
- MWDT callback logs the event (callback executes in IRAM before reset)

Runtime Tests
^^^^^^^^^^^^^

**Purpose**: Continuous monitoring of CPU and stack integrity during normal operation.

**Rationale**: Hardware faults can occur at any time during operation. Runtime tests detect transient faults, aging-related degradation, or corruption that develops during operation.

**Tests Executed**:

1. **CPU Register Test** (``bist_cpu_regs_test()``)

   - **IEC 60730 Component**: 1.1 (CPU Registers)
   - **Purpose**: Verifies all 32 general-purpose registers maintain integrity; on FPU supported devices, also verifies all 32 single-precision FPU registers (f0–f31)
   - **Rationale**: Register corruption can cause incorrect computation or control flow errors
   - **Execution Frequency**: Called in main loop (every iteration)

2. **CPU CSR Test** (``bist_cpu_csr_regs_test()``)

   - **IEC 60730 Component**: 1.1 (CPU CSRs)
   - **Purpose**: Verifies critical Control and Status Registers — trap CSRs (MTVEC, MEPC, MCAUSE, MTVAL, MSCRATCH), PMP registers (PMPADDR0–15, PMPCFG0–3), PMA address registers on C6/H2/C5/P4, MEXSTATUS/MHINT on C5 only, and FPU CSRs (``fflags``, ``frm``, ``fcsr``) on FPU supported devices (H4, P4).
   - **Rationale**: CSR corruption can cause exception handling failures or memory protection violations
   - **Execution Frequency**: Called in main loop (every iteration)

3. **Stack Overflow Check** (``bist_cpu_stack_overflow_check()``)

   - **IEC 60730 Component**: 4.2 (Variable Memory)
   - **Purpose**: Detects stack overflow by scanning the protection region for sentinel corruption
   - **Rationale**: Early detection prevents memory corruption from spreading
   - **Execution Frequency**: Called in main loop (every iteration)

4. **Program Counter Test** (``bist_pc_test()``)

   - **IEC 60730 Component**: 1.3 (Program Counter)
   - **Purpose**: Exercises PC register bits by calling functions in different memory regions
   - **Rationale**: PC corruption can cause execution outside intended code regions
   - **Execution Frequency**: Called in main loop (every iteration)

**Error Handling**: All runtime test failures trigger ``fail_safe_exit()``, preventing continued operation with detected faults.

GPIO Configuration
^^^^^^^^^^^^^^^^^^

**Functions**: ``configure_led()``, ``configure_button()``

**Purpose**: Configures GPIO pins for application functionality (LED output, button input).

**Rationale**: GPIO configuration is performed after BIST validation to ensure I/O functionality is verified before use.

**Implementation**:

- LED GPIO (GPIO 7): Configured as output for visual feedback
- Button GPIO (GPIO 9): Configured as input (tied to GND, expected level = 0)

Watchdog Initialization
^^^^^^^^^^^^^^^^^^^^^^^

**Functions**:

- ``wdt_init(CONFIG_ESP_BIST_WDT_TIMEOUT_US)``
- ``wdt_init_windowed(CONFIG_ESP_BIST_WDT_WINDOWED_UNDERFLOW_TIMEOUT_US)``

**Purpose**: Initializes windowed watchdog timer for timing fault detection.

**Rationale**: IEC 60730-1 Annex H requires timing monitoring to detect infinite loops, deadlocks, or missed deadlines. Windowed watchdog provides:

- **Upper bound**: System must feed watchdog within timeout window (prevents infinite loops)
- **Lower bound**: System must not feed watchdog too early (prevents premature feeding)

**Configuration**:

- ``CONFIG_ESP_BIST_WDT_TIMEOUT_US``: Maximum time between feeds (default: application-specific)
- ``CONFIG_ESP_BIST_WDT_WINDOWED_UNDERFLOW_TIMEOUT_US``: Minimum time between feeds (default: application-specific)

**Operation**: Application must call ``wdt_feed()`` periodically in main loop. Failure to feed within timeout window triggers system reset.

**Error Handling**: ``wdt_init_windowed()`` returns ``0`` on success or ``-1`` on failure. A ``-1`` return leaves no poisoned state (``is_windowed`` remains false, any partially created timer is deleted), so the call may be retried or the application can fall back to non-windowed mode.

Main Loop
^^^^^^^^^

**Purpose**: Application's normal operation loop with integrated safety monitoring.

**Structure**:

.. code-block:: c

    while (1) {
        set_led(get_button());      // Application logic
        runtime_tests();             // Safety monitoring
        wdt_feed();                  // Watchdog maintenance
    }

**Rationale**:

1. **Application Logic First**: Execute application functionality before safety checks
2. **Runtime Tests**: Continuous hardware integrity monitoring
3. **WDT Feed**: Maintain watchdog within timing window

**Timing Considerations**:

- Runtime tests execute deterministically (all BIST code in IRAM)
- Total loop time must be less than ``CONFIG_ESP_BIST_WDT_TIMEOUT_US``
- WDT feed must occur within configured window

Fail-Safe Handling
------------------

**Function**: ``fail_safe_exit()``

**Purpose**: Transitions system to safe state when faults are detected.

**Implementation**:

.. code-block:: c

    static void fail_safe_exit(void)
    {
        ESP_LOGE(TAG, "Fail safe exit");
        while (1)
            ;
    }

**Rationale**: When a fault is detected, the safest response is to stop execution and enter a known safe state. The infinite loop:

1. **Prevents Continued Operation**: Stops application from operating with detected faults
2. **Enables Watchdog Reset**: If WDT is still active, timeout will reset system
3. **Maintains System State**: Preserves system state for debugging (if needed)

**Alternative Implementations**: Applications may implement more sophisticated fail-safe handling:

- Log fault to non-volatile storage
- Disable critical outputs
- Transition to degraded mode
- Trigger external safety mechanisms

Build Configuration
-------------------

The standalone application uses the following build configuration:

**CMakeLists.txt**:

.. code-block:: cmake

    cmake_minimum_required(VERSION 3.22)

    set(BIST_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../)
    set(APP_NAME standalone_critical_${SOC_TARGET})
    set(APP_SOURCES main.c)

    include(${BIST_ROOT_DIR}/cmake/project.cmake)
    project(standalone)

**Key Points**:

- **BIST_ROOT_DIR**: Points to ESP-BIST library root
- **APP_NAME**: Application name
- **project.cmake**: Includes BIST library, toolchain, and build configuration

**Kconfig Configuration** (``bist.conf``):

The application uses default BIST configuration. Customize via ``bist.conf``:

- ``CONFIG_ESP_BIST_WDT_TIMEOUT_US``: Watchdog timeout
- ``CONFIG_ESP_BIST_WDT_WINDOWED_UNDERFLOW_TIMEOUT_US``: Windowed WDT underflow timeout
- ``CONFIG_ESP_BIST_CLOCK_PERCENT_FREQUENCY_DRIFT``: Clock drift tolerance
- ``CONFIG_ESP_BIST_ADC_PERCENT_DEVIATION``: ADC reading tolerance for plausibility tests

Integration Patterns
--------------------

Pattern 1: Post-Boot Validation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Execute comprehensive hardware tests at startup before normal operation:

.. code-block:: c

    int main() {
        post_boot_tests();  // Must pass before continuing
        // ... initialize application ...
    }

**Rationale**: Ensures system starts in known-good state.

Pattern 2: Runtime Monitoring
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Integrate runtime tests into main application loop:

.. code-block:: c

    while (1) {
        application_logic();
        runtime_tests();  // Continuous monitoring
        wdt_feed();
    }

**Rationale**: Detects faults that develop during operation.

Pattern 3: Watchdog Integration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Initialize watchdog after post-boot tests and feed in main loop:

.. code-block:: c

    post_boot_tests();
    wdt_init(CONFIG_ESP_BIST_WDT_TIMEOUT_US);
    wdt_init_windowed(CONFIG_ESP_BIST_WDT_WINDOWED_UNDERFLOW_TIMEOUT_US);

    while (1) {
        // ... application logic ...
        wdt_feed();
    }

**Rationale**: Provides ultimate safety mechanism for timing fault detection.

Pattern 4: Stack Overflow Protection
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Initialize stack sentinel early and check periodically:

.. code-block:: c

    bist_cpu_stack_overflow_init();

    while (1) {
        // ... application logic ...
        if (bist_cpu_stack_overflow_check() == BIST_ESP_STACK_TEST_OVERFLOW) {
            fail_safe_exit();
        }
    }

**Rationale**: Early detection prevents memory corruption.

Pattern 5: Fail-Safe Response
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Implement appropriate fail-safe handling for each test failure:

.. code-block:: c

    bist_esp_err_t err = bist_cpu_regs_test();
    if (err != BIST_ESP_OK) {
        fail_safe_exit();  // Or custom fail-safe handler
    }

**Rationale**: Prevents continued operation with detected faults.

Safety Considerations
---------------------

Test Execution Order
^^^^^^^^^^^^^^^^^^^^

The standalone application executes tests in a specific order:

1. **Post-boot tests first**: Hardware validation before any application logic
2. **Stack initialization**: Stack sentinel before any deep function calls
3. **Watchdog callbacks**: Registered before watchdog initialization
4. **Runtime tests**: Integrated into main loop for continuous monitoring

**Rationale**: Order ensures dependencies are satisfied and safety mechanisms are active before they are needed.

Timing Constraints
^^^^^^^^^^^^^^^^^^

- **Post-boot test duration**: Must complete within acceptable startup time
- **Runtime test duration**: Must complete within watchdog timeout window
- **WDT feed frequency**: Must feed within configured window (not too early, not too late)

**Rationale**: Timing constraints ensure tests don't interfere with application functionality while maintaining safety coverage.

Memory Usage
^^^^^^^^^^^^

- **Stack size**: Configured in linker script; must accommodate application needs plus BIST test stack usage
- **IRAM placement**: All BIST code in IRAM for deterministic execution
- **RAM test exclusion**: Safe RAM buffer excluded from RAM test coverage

**Rationale**: Memory configuration ensures tests don't interfere with application memory usage.

Error Recovery
^^^^^^^^^^^^^^

The standalone application uses a simple fail-safe exit. Production applications may implement:

- **Fault logging**: Record fault type and context
- **Recovery attempts**: Retry tests or reset specific subsystems
- **Degraded mode**: Continue operation with reduced functionality
- **External notification**: Alert external systems of fault condition

**Rationale**: Appropriate error recovery depends on application safety requirements and fault tolerance needs.

Customization Guide
-------------------

Adapting the Standalone Application
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To adapt the standalone application for your use case:

1. **Modify GPIO Configuration**: Update ``LED_GPIO`` and ``BTN_GPIO`` definitions for your hardware
2. **Customize Application Logic**: Replace LED/button logic with your application functionality
3. **Adjust Test Frequency**: Modify runtime test frequency based on safety requirements
4. **Implement Custom Fail-Safe**: Replace ``fail_safe_exit()`` with application-specific fail-safe handling
5. **Configure Timeouts**: Adjust watchdog timeouts based on application timing requirements

Integration Example
^^^^^^^^^^^^^^^^^^^

For maximum safety coverage (as in standalone application):

.. code-block:: c

    int main() {
        // Complete post-boot validation
        post_boot_tests();

        // Initialize all safety mechanisms
        bist_cpu_stack_overflow_init();
    #if SOC_XT_WDT_SUPPORTED
        esp_xt_wdt_register_callback(fail_safe_exit, NULL);
    #endif
        wdt_register_callback(wdt_callback, NULL);

        // Initialize application
        configure_gpio();
        wdt_init(CONFIG_ESP_BIST_WDT_TIMEOUT_US);
        wdt_init_windowed(CONFIG_ESP_BIST_WDT_WINDOWED_UNDERFLOW_TIMEOUT_US);

        // Main loop with continuous monitoring
        while (1) {
            application_logic();
            runtime_tests();
            wdt_feed();
        }
    }

**Note**: Complete integration provides maximum safety coverage per IEC 60730-1 Annex H requirements.

References
----------

- :doc:`software_safety_requirements` - IEC 60730 component mapping and safety requirements
- :doc:`module_design_and_coding` - Detailed test module documentation
- :doc:`software_validation` - Test validation and fault injection
- :doc:`api` - Complete API reference
- :doc:`get_started` - Build and development setup

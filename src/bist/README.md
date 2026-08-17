# BIST (Built-In Self Test) Library

This directory contains the Built-In Self Test (BIST) library for the Espressif SoCs. The BIST library is designed to verify the integrity and proper operation of the hardware components in the SoC, including the CPU, memory, and clock sources. The library is intended for use in safety-critical applications that require compliance with industry standards such as IEC 60730 Class B.

The Espressif's Built-in Self Test library performs the following tests:

- CPU registers
- CPU stack overflow
- Configuration and Status Registers (CSR)
- Interrupt handling and execution
- Volatile memory
- Non-volatile memory
- Program Counter (PC)
- Clock
- Digital IO

## Supported SoCs

- ESP32-C3
- ESP32-C5
- ESP32-C6
- ESP32-C61
- ESP32-H2
- ESP32-P4

## License

The BIST library is licensed under the LGPL-3.0 license. For more information, see the [LICENSE](LICENSE) file.

## CPU register test

The CPU register test procedure performs tests on register X1-X31.

| Register | ABI Name | Role                           |
|----------|----------|--------------------------------|
| x0       | zero     | Hardwired zero                 |
| x1       | ra       | Return address                 |
| x2       | sp       | Stack pointer                  |
| x3       | gp       | Global pointer                 |
| x4       | tp       | Thread pointer                 |
| x5       | t0       | Temporary/scratch              |
| x6-x7    | t1-t2    | Temporary/scratch              |
| x8       | s0/fp    | Saved frame pointer            |
| x9       | s1       | Saved register                 |
| x10-x11  | a0-a1    | Function arguments             |
| x12-x17  | a2-a7    | Function arguments/return values |
| x18-x27  | s2-s11   | Saved registers                |
| x28      | t3       | Temporary/scratch              |
| x29      | t4       | Temporary/scratch              |
| x30      | t5       | Temporary/scratch              |
| x31      | t6       | Temporary/scratch              |

The `bist_cpu_regs_test` function is part of the safety mechanisms provided by Espressif Systems, specifically designed for testing the integrity and proper functioning of CPU registers in RISC-V architecture.

The primary objective of `bist_cpu_regs_test` is to verify the operational integrity of each CPU register. The process involves altering the values of registers, comparing them against known values, and checking for any discrepancies.

### Detailed Operation

1. Initialization: The function starts by storing the return address (ra) on the stack to ensure it can return properly after testing.

1. Register Testing:

- Stacked Registers: For registers that are typically saved on the stack during a function call (like ra, gp, tp, and saved registers s0 to s11), the function saves their current value on the stack, tests them, and then restores their original values from the stack.
- Non-Stacked Registers: For registers not typically saved on the stack (like t0 to t6 and a0 to a7), the function tests their values directly.
- Stack Pointer: Specially for the stack pointer, its value is saved in a temporary register before testing, and then restored to its original value.

1. Testing Methodology: The test is performed by loading a test value (0xAAAAAAAA), comparing it with the current value in the register, and then repeating the process with a different test value (0x55555555). That way all bits in the register are tested.

1. Error Handling: If any discrepancy is found during the testing, the function jumps to the error label where it restores the ra register from the stack and returns an error code (`BIST_ESP_CPU_TEST_ERR`).

1. Successful Completion: If no discrepancies are found, the function restores the ra register from the stack and returns a success code (`BIST_ESP_OK`).

## Configuration and Status Register (CSR) test

It is designed to verify the integrity and proper operation of various CSRs, including machine trap setup, machine trap handling, Physical Memory Protection (PMP) registers, and Physical Memory Attribute (PMA) registers.

Some CSRs directly affect the application's operation and the MCU's state, and testing these can lead to unintended side effects that might compromise the system's stability, safety, or data integrity. Therefore, the test is designed to be non-intrusive and to avoid modifying the system's state. Write masks are carefully chosen to exclude write-once bits (e.g., PMP Lock, PMA Lock) that would render registers unmodifiable until the next reset.

1. Testing Method

- Non-Stacked CSRs: Tests CSRs that do not require their original values to be preserved. It writes a pattern, verifies it, then writes an inverted pattern and verifies again.

- Stacked CSRs: For CSRs whose original values need to be preserved, the test first saves the current value on the stack, performs the write-verify operations with both patterns, and then restores the original value from the stack.

1. Test Patterns

Two binary patterns (0xAAAAAAAA and 0x55555555) are used to test the read and write capabilities of the CSRs. A mask is applied to these patterns when necessary to accommodate registers that do not utilize all 32 bits.

1. Register Coverage

The following CSR groups are tested:

| Group | CSRs | SOCs | Mask | Notes |
|-------|------|------|------|-------|
| Machine Trap Setup | mtvec | All | 0xFFFFFF00 | |
| Machine Trap Handling | mscratch, mepc, mcause, mtval | All | register-specific | |
| PMP Address | pmpaddr0-15 | All | C3/C6/H2: 0xFFFFFFFF, C5/C61/P4: 0x3FFFFFE0 | C5/C61/P4 have 128-byte granularity (25 writable bits) |
| PMP Configuration | pmpcfg0-3 | All | C3/C6/H2: 0x1D1D1D1D, C5/C61/P4: 0x0D0D0D0D | Excludes Lock, reserved, and W bit (R=0,W=1 is reserved RISC-V encoding); C5/C61/P4 also exclude A[1] (NA4 not selectable at G=5) |
| PMA Address | pma_addr0-11 (0xBD0-0xBDB) | C6, H2, C5, P4 | 0x3FFFFFE0 | 25 writable bits; guarded by `SOC_CPU_HAS_PMA` |
| Machine Extension | mexstatus (0x7E1) | C5 only | 0x00102C00 | PBEXE, PPBEXE, NMFT, CLIC_INHV (C6/H2 only have SOFT_RST — unsafe to test) |
| Machine Hint | mhint (0x7C5) | C5 only | 0x00100000 | SBE bit only (does not exist on C6/H2) |
| FPU CSRs | fflags, frm, fcsr | P4 | 0x1F / 0x07 / 0xFF | Guarded by `ESP_BIST_USE_FPU` |

> **Note — PMA entries 12-15 skipped:** The ROM bootloader may configure these as active NAPOT/TOR regions. In active modes the hardware forces address low-order bits to match the region encoding, causing the stacked write/verify pattern to fail.

> **Note — PMA cfg registers not tested:** The PMA_L (Lock) bit at position 29 is write-once. Any test pattern that inadvertently sets this bit permanently locks the entry until the next power-on reset, making stacked testing unsafe.

1. Error Handling

If a CSR does not correctly retain the written value, the test jumps to an error handling routine (errorCSR) which sets a specific error code (`BIST_ESP_CPU_CSR_TEST_ERR`) and returns, indicating test failure.

## Volatile Memory Test

The volatile memory test is designed to verify the integrity and proper operation of the volatile memory (RAM) in the MCU. It leverages March A, March X, and Abraham algorithms to exhaustively test the integrity of the device's RAM. The test is non-destructive and does not modify the memory's contents.

1. Testing Method

The BIST routines work by temporarily backing up a chunk of the heap before performing the test operations. The backup is stored in a safe area that is not tested. It targets the heap area, suitable for dynamic memory integrity verification.

- March A:

`bist_ram_test_march_a` sequentially writes zeroes and then ones to each memory location, verifying the content at each step.

- March X:

`bist_ram_test_march_x` is a more comprehensive test that includes several phases of writing and reading in both ascending and descending orders, intended to uncover a wider range of potential memory faults.

- Abraham:

`bist_ram_test_abraham` implements the 10-element Abraham algorithm (30 operations per cell) for Class C–level variable-memory coverage. It detects stuck-at, transition, coupling, and address decoder faults. The test uses time-division over partition pairs: each call tests one pair of partitions (sized by `CONFIG_ESP_BIST_RAM_PARTITION_SIZE`) from the RAM region, and `bist_ram_test_abraham_full` runs the complete pair schedule for full coupling coverage across the entire region.

## Non-volatile memory Test

It is designed to verify the integrity of the non-volatile memory, it checks for memory corruption or whether there is a change in the memory content during the application execution.

### CRC32 Algorithm for NVM Integrity Verification

The algorithm used in this test is the 32-bit Cyclic Redundancy Check (CRC), defined by the polynomial `0x04C11DB7` (`x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1`), offers superior error detection capabilities. It's the standard choice for applications requiring high levels of data integrity. The chosen polynomial is significant due to its error-detecting capabilities, providing a good balance between speed and detection efficiency. By applying this algorithm, it's possible to verify the integrity of the data in NVM by comparing calculated checksums with expected values, thus ensuring data reliability.

### Post Build Process

After building the image, CRC32 of the relevant NVM section is calculated for comparison in runtime.

This process involves a script, located at [`scripts/calculate_crc32.py`](../../scripts/calculate_crc32.py), which calculates the CRC32 checksums of the `.flash.text` and `.flash.rodata` sections of the firmware. These sections contain the executable code and read-only data, respectively, which are critical for the system's operation.

After the calculation, the script injects these CRC32 checksums into a dedicated place in the Flash memory. This dedicated location is carefully chosen to ensure it does not interfere with the sections being checksummed, thereby avoiding any potential for the calculation to inadvertently alter the checksum. This step is crucial for maintaining the integrity of the firmware by ensuring that any modifications post-build are accounted for in the checksum.

### Runtime Verification

To ensure continuous integrity of the NVM during device operation, a runtime test is performed using the same CRC32 algorithm. `bist_flash_test` recalculates the CRC32 values of the `.flash.text` and `.flash.rodata` sections and compares them against the pre-calculated values stored in Flash memory. If the recalculated checksums match the stored values, it confirms the integrity of the NVM. Otherwise, a mismatch indicates potential corruption or unintended modifications to these sections, triggering error-handling procedures.

## Program Counter Testing

The `bist_pc_test` routine is designed for testing the Program Counter (PC) register, specifically targeting the detection of stuck at conditions across various bits of the PC register.

1. Overview

The testing focuses on exercising specific bits of the 32-bit PC register, taking into consideration alignment requirements and memory mapping. The PC register is aligned to 4 bytes, implying the two least significant bits are always zero. The memory mapping covers different segments including IRAM, Flash, and RTC memory, each mapped to distinct address ranges.

1. Memory Mapping

- IRAM (ESP32-C3): 0x40380000 - 0x403BEE00
- IRAM (ESP32-C6): 0x40800000 - 0x40880000
- Flash (Executed from ICache):
  - 2MB: 0x42010000 - 0x421FFFFF
  - 4MB: 0x42010000 - 0x423FFFFF
  - 8MB: 0x42010000 - 0x427FFFFF
- RTC: 0x50000000 - 0x50001FFF

1. Testing Method

The testing is divided into several parts, each aimed at validating different sets of bits within the PC register:

`pc_test_0` is placed at the end of `IRAM` to test upper address bits that differ between IRAM and Flash/RTC regions.
`pc_test_1` and `pc_test_2` are placed in `Flash` memory with a 64KB gap (offset 0xFFF8) between them, ensuring bits [3:15] are inverted. This placement avoids consuming scarce SRAM.
`pc_test_3` is allocated in `RTC` memory, focusing on bit 28.

Each test function is assigned to a specific memory section through the `__attribute__((section(".pc_test_X")))` directive, ensuring their placement in the intended memory areas.

The core function `bist_pc_test` iterates through an array of function pointers, each pointing to a test function defined to return its own address. The test validates the PC register by ensuring that the address returned by each test function matches the function's address, thereby confirming the correct operation of the tested PC register bits across IRAM, Flash, and RTC address ranges.

### Indirect time-slot monitoring

The library implements a software based windowed watchdog to strictly monitor the main loop execution time. This is done by using a private timer in conjunction with the main watchdog timer. The indirect time-slot monitoring helps detect Program Counter (PC) faults (e.g., due to corruption, infinite loops, or unintended jumps). It infers correct execution by enforcing strict timing deadlines on software checkpoints.

The Windowed Watchdog mechanism relies on two key components working together that defines the allowed reset window (minimum and maximum time limits):

- Private Timer
- Main Watchdog Timer (Hardware Watchdog)

The private timer is configured independently. It defines the bottom of the time-slot window. The main watchdog defines the top of the time-slot window. The private timer is set to a value defined by `CONFIG_ESP_BIST_WDT_UNDERFLOW_US` while the main watchdog is set to a value defined by `CONFIG_ESP_BIST_WDT_TIMEOUT_US`.

If the application tries to reset the watchdog:

- Too early (before CONFIG_WDT_UNDERFLOW_US): Private timer flags an underflow, blocking the watchdog reset.
- Too late (after the max window): Hardware watchdog will expire, triggering an interrupt that the application can register for fault handling and resetting the device.
- Within the allowed window: The reset is done correctly.

## Interrupt Handling and Execution Test

The interrupt tests cover IEC 60730 Table H.1 ID 2 (Interrupt handling and execution). They are enabled with `CONFIG_ESP_BIST_INTERRUPT_TEST` (standalone builds) and are intended as post-boot checks.

### Software interrupt source map (`bist_interrupt_source_map_test`)

Exercises the four SoC software interrupt sources `CPU_INTR_FROM_CPU_0` through `CPU_INTR_FROM_CPU_3`. These are software-triggered sources (not peripheral IRQs): writing the corresponding register asserts the source into the interrupt matrix.

For each source the test:

1. Routes the source to free CPU interrupt line 9 and installs an ISR
2. Triggers the source eight times
3. Verifies the enable mask and that the ISR count equals eight
4. Disables and unmaps the source

Returns `BIST_ESP_OK` on success, or `BIST_ESP_INTERRUPT_TEST_ERR` on failure.

### Hardware interrupt delivery (`bist_hardware_interrupt_test`)

Starts GPTimer alarms on TIMG0 (500 us → CPU interrupt 9) and TIMG1 (1000 us → CPU interrupt 10). While both ISR counts are below 1000, the test periodically checks that `|count1 - 2*count2| ≤ 1`. The ±1 tolerance accounts for possible synchronization skew between the two independent timer interrupts. Requires two Timer Groups (`TIMG_LL_INST_NUM >= 2`).

Returns `BIST_ESP_OK` on success, or `BIST_ESP_INTERRUPT_TEST_ERR` on failure. QEMU validates the software path only; the TIMG hardware path requires device testing.

## Clock Testing

The clock test is designed to verify the integrity and proper operation of the clock sources. It checks the clock frequency and stability to ensure the MCU's operation is within the specified limits.

The following clock sources are tested:

- External 32 Khz crystal oscillator
- Main 40 Mhz crystal oscillator

### External 32KHz Crystal Oscillator

On SoCs with XT WDT support (e.g., ESP32-C3), this test uses the XT WDT peripheral to verify the external 32KHz crystal oscillator stability. If the XT WDT detects a failure of 200 cycles from the 32KHz crystal, it triggers an interrupt. The test then verifies if the interrupt was triggered and returns `BIST_ESP_CLOCK_TEST_ERR`.

On SoCs without XT WDT support (e.g., ESP32-C6), this test is skipped and returns `BIST_ESP_OK`. Use `bist_main_crystal_test()` to validate the main XTAL frequency on those targets.

### Main 40 Mhz Crystal Oscillator

This test uses the External 32Khz as a reference for calculating the 40Mhz crystal oscillator frequency. It calculates the ratio between the 40Mhz and 32Khz clocks and compares it with the expected value. If the ratio is out of the specified range, the test returns `BIST_ESP_CLOCK_TEST_ERR`. The frequency drift is set by the `CONFIG_ESP_BIST_CLOCK_PERCENT_FREQUENCY_DRIFT` configuration and it represents the maximum allowed deviation from the expected value in percentage.

## CPU Stack tests

### Overflow

The stack is a crucial section of RAM used by the CPU to temporarily store information such as data and addresses.
Due to the limited number of CPU registers, the stack serves as an essential storage area.

In Espressif RISC-V SoCs, the stack is located at the end of the RAM and grows downward.
The stack pointer, which is 32 bits wide, is decremented with each PUSH instruction and incremented with each POP instruction.

The primary goal of the stack overflow test is to ensure that the stack does not overlap with the program data memory during execution.
This overlap can occur in various scenarios, such as the use of recursive functions, leading to potential system crashes or unpredictable behavior.

### Stack Overflow Detection Mechanism

To detect stack overflow, a reserved block of memory at the end of the stack is filled with a predefined pattern.
A test function is periodically invoked to verify the integrity of this block. If the stack overflows,
it will overwrite this reserved block with corrupted data, which the test function will detect as an overflow error.

### Linker Script Configuration

The linker script is a vital component in defining the memory layout of the program.
It specifies the location and size of the stack and other memory sections. Proper configuration of the linker script
ensures that the stack is correctly placed and that the reserved block for overflow detection is appropriately defined.

### Test Scenario

1. Initialization
At system startup, the reserved memory block at the end of the stack is initialized with a predefined pattern (e.g., 0xDEADBEEF).

2. Periodic Verification
A test function is periodically called, typically by a timer interrupt or within the main program loop. This function checks the integrity of the predefined pattern.

3. Detection
If the stack overflows, the predefined pattern will be overwritten. The test function will detect this change, indicating a stack overflow error.

4. Error Handling
Upon detecting a stack overflow, appropriate error handling procedures should be invoked. This might include logging the error, halting the system, or attempting a safe recovery.

### Stack Overflow Handler

The signature for the stack overflow handler is `void handle_stack_overflow(void)` and should be defined in the application code.

## IO Tests

The IO (GPIO) test routines verify the correct operation of the digital Input/Output pins and analog ADC channels.
### Digital I/O

The digital I/O (GPIO) test routines verify the correct operation of the digital Input/Output pins.

These tests are supposed to be run before the application configures the GPIO pins, as they will configure the pins to a known state and perform read/write operations to ensure the pins are functioning correctly.

#### Output Test

The output test (`bist_gpio_output_test`) checks if a GPIO pin can be reliably set to logic low and high:

1. The pin is configured as output.
2. The test sets the pin to logic low (0) and reads back the value to confirm.
3. The test sets the pin to logic high (1) and reads back the value to confirm.
4. If the pin does not reflect the expected value at any step, the test fails with error code `BIST_ESP_IO_TEST_ERR`.

This ensures the pin can be controlled as expected by the application.

#### Input Test

The input test (`bist_gpio_input_test`) checks if a GPIO pin can correctly read an external logic level:

1. The pin is configured as input.
2. The test reads the pin value and compares it to the expected logic level (0 or 1) provided by the user.
3. If the read value does not match the expected value, the test fails with error code `BIST_ESP_IO_TEST_ERR`.

This ensures the pin can reliably read external signals.

### Analog I/O

The analog I/O (ADC) test routines verify the correct operation of ADC input channels.

Tolerance is controlled by `CONFIG_ESP_BIST_ADC_PERCENT_DEVIATION` (default: 1%).

#### Low Level Test

The low level test (`bist_adc_low_level_test`) checks if an ADC channel reads near zero with internal pull-down:

1. The ADC unit and channel are configured with 12 dB attenuation.
2. Internal pull-down is enabled on the mapped GPIO.
3. After a 10 ms settling delay, the raw ADC value is read.
4. If the reading exceeds the configured tolerance, the test fails with `BIST_ESP_ADC_TEST_ERR`.

#### High Level Test

The high level test (`bist_adc_high_level_test`) checks if an ADC channel reads near full scale with internal pull-up:

1. The ADC unit and channel are configured with 12 dB attenuation.
2. Internal pull-up is enabled on the mapped GPIO.
3. After a 10 ms settling delay, the raw ADC value is read.
4. The reading is compared against a SoC-specific high reference minus tolerance.
5. If the reading is too low, the test fails with `BIST_ESP_ADC_TEST_ERR`.

#### Reference Test (ESP32-C3 only)

The reference test (`bist_adc_reference_test`) checks mid-scale ADC behavior using internal VREF output:

1. The ADC unit and channel are configured with 12 dB attenuation.
2. Internal VREF output is enabled to bias the pin near mid-scale.
3. After a 10 ms settling delay, the raw ADC value is read.
4. The reading must be within tolerance of the reference level.
5. If the reading is stuck at low or high, the test fails with `BIST_ESP_ADC_TEST_ERR`.

## Watchdog

The Main System Watchdog Timer (MWDT) of Timer 1 is enabled by default. The MWDT is a hardware watchdog timer that can be used to monitor the system's operation and detect potential failures. The MWDT is configured to trigger a system reset if the system fails to clear the watchdog within a specified time frame. The MWDT is enabled by default to ensure the system can recover from potential failures and maintain operational integrity.

Before resetting the system, the watchdog can trigger an interrupt to allow the application to perform any necessary operation. The interrupt handler can be defined in the application and registered using `wdt_register_callback(void (*callback)(void *), void *arg)`.

The `CONFIG_ESP_BIST_WDT_TIMEOUT_US` configuration defines the watchdog timeout in microseconds. The watchdog timeout should be set according to the system's requirements, ensuring it provides sufficient time for the application to complete its operations. The interrupt will be triggered when the watchdog timer reaches its timeout value. The system will be reset after double the timeout value.

### Windowed Watchdog

The system implements a Windowed Watchdog using a private timer in conjunction with the main watchdog timer. This mechanism provides indirect time-slot monitoring of the application execution flow.

A Program Counter (PC) Fault is detected if the watchdog timer is not reset within a specified time window. If an underflow occurs (early reset), subsequent attempts to reset the watchdog within the same time window will fail, ensuring fault detection.

The Underflow value is defined in microseconds by `CONFIG_ESP_BIST_WDT_UNDERFLOW_US` in the `sdkconfig.h` file. The overflow value is the same as the main watchdog timer timeout, defined by `CONFIG_ESP_BIST_WDT_TIMEOUT_US`.

The windowed watchdog can be enabled with `void wdt_init_windowed(uint32_t underflow_timeout_us)`.

The windowed WDT behavior is validated by the `windowed_wdt_test` application, which covers normal operation within the time window, underflow detection (feed before the underflow window), and consecutive feed cycles. CI runs both QEMU and device tests for this application.

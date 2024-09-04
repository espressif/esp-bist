# BIST (Built-In Self Test) Library

This repository contains the Built-In Self Test (BIST) library for the Espressif SoCs. The BIST library is designed to verify the integrity and proper operation of the hardware components in the SoC, including the CPU, memory, and clock sources. The library is intended for use in safety-critical applications that require compliance with industry standards such as IEC 60730 Class B.

The Espressif's Built-in Self Test library performs the following tests:

- CPU registers
- CPU stack overflow
- Configuration and Status Registers (CSR)
- Volatile memory
- Non-volatile memory
- Program Counter (PC)
- Clock

## Supported SoCs

- ESP32-C3

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

It is designed to verify the integrity and proper operation of various CSRs, including machine trap setup, machine trap handling, and Physical Memory Protection (PMP) registers.

Some CSRs directly affect the application's operation and the MCU's state, and testing these can lead to unintended side effects that might compromise the system's stability, safety, or data integrity. Therefore, the test is designed to be non-intrusive and to avoid modifying the system's state.

1. Testing Method

- Non-Stacked CSRs: Tests CSRs that do not require their original values to be preserved. It writes a pattern, verifies it, then writes an inverted pattern and verifies again.

- Stacked CSRs: For CSRs whose original values need to be preserved, the test first saves the current value on the stack, performs the write-verify operations with both patterns, and then restores the original value from the stack.

1. Test Patterns

Two binary patterns (0xAAAAAAAA and 0x55555555) are used to test the read and write capabilities of the CSRs. A mask is applied to these patterns when necessary to accommodate registers that do not utilize all 32 bits.

1. Error Handling

If a CSR does not correctly retain the written value, the test jumps to an error handling routine (errorCSR) which sets a specific error code (`BIST_ESP_CPU_CSR_TEST_ERR`) and returns, indicating test failure.

## Volatile Memory Test

The volatile memory test is designed to verify the integrity and proper operation of the volatile memory (RAM) in the MCU. It leverages March A and March X algorithms to exhaustively test the integrity of the device's RAM. The test is non-destructive and does not modify the memory's contents.

1. Testing Method

The BIST routines work by temporarily backing up a chunk of the heap before performing the test operations. The backup is stored in a safe area that is not tested. It targets the heap area, suitable for dynamic memory integrity verification.

- March A:

`bist_ram_test_march_a` sequentially writes zeroes and then ones to each memory location, verifying the content at each step.

- March X:

`bist_ram_test_march_a` is a more comprehensive test that includes several phases of writing and reading in both ascending and descending orders, intended to uncover a wider range of potential memory faults.

## Non-volatile memory Test

It is designed to verify the integrity of the non-volatile memory, it checks for memory corruption or whether there is a change in the memory content during the application execution.

### CRC32 Algorithm for NVM Integrity Verification

The algorithm used in this test is the 32-bit Cyclic Redundancy Check (CRC), defined by the polynomial `0x04C11DB7` (`x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1`), offers superior error detection capabilities. It's the standard choice for applications requiring high levels of data integrity. The chosen polynomial is significant due to its error-detecting capabilities, providing a good balance between speed and detection efficiency. By applying this algorithm, it's possible to verify the integrity of the data in NVM by comparing calculated checksums with expected values, thus ensuring data reliability.

### Post Build Process

After building the image, CRC32 of the relevant NVM section is calculated for comparison in runtime.

This process involves a script, located at `scripts/calculate_crc32.py`, not located in this repository, which calculates the CRC32 checksums of the `.flash.text` and `.flash.rodata` sections of the firmware. These sections contain the executable code and read-only data, respectively, which are critical for the system's operation.

After the calculation, the script injects these CRC32 checksums into a dedicated place in the Flash memory. This dedicated location is carefully chosen to ensure it does not interfere with the sections being checksummed, thereby avoiding any potential for the calculation to inadvertently alter the checksum. This step is crucial for maintaining the integrity of the firmware by ensuring that any modifications post-build are accounted for in the checksum.

### Runtime Verification

To ensure continuous integrity of the NVM during device operation, a runtime test is performed using the same CRC32 algorithm. `bist_flash_test` recalculates the CRC32 values of the `.flash.text` and `.flash.rodata` sections and compares them against the pre-calculated values stored in Flash memory. If the recalculated checksums match the stored values, it confirms the integrity of the NVM. Otherwise, a mismatch indicates potential corruption or unintended modifications to these sections, triggering error-handling procedures.

## Program Counter Testing

The `bist_pc_test` routine is designed for testing the Program Counter (PC) register, specifically targeting the detection of stuck at conditions across various bits of the PC register.

1. Overview

The testing focuses on exercising specific bits of the 32-bit PC register, taking into consideration alignment requirements and memory mapping. The PC register is aligned to 4 bytes, implying the two least significant bits are always zero. The memory mapping covers different segments including IRAM, ROM, and RTC memory, each mapped to distinct address ranges.

1. Memory Mapping

- IRAM: 0x40380000 - 0x403BEE00
- ROM (Executed from ICache):
  - 2MB: 0x42010000 - 0x421FFFFF
  - 4MB: 0x42010000 - 0x423FFFFF
  - 8MB: 0x42010000 - 0x427FFFFF
- RTC: 0x50000000 - 0x50001FFF

1. Testing Method

The testing is divided into several parts, each aimed at validating different sets of bits within the PC register:

`pc_test_0` and `pc_test_1` are designed to test bits 2-17. Both are placed in `IRAM`, with specific address placements ensuring coverage of these bits.
`pc_test_2` is placed in `Flash` memory to test bits 19, 20, 21, and 25.
`pc_test_3` is allocated in `RTC` memory, focusing on bit 28.

Each test function is assigned to a specific memory section through the `__attribute__((section(".pc_test_X")))` directive, ensuring their placement in the intended memory areas.

The core function `bist_pc_test` iterates through an array of function pointers, each pointing to a test function defined to return its own address. The test validates the PC register by ensuring that the address returned by each test function matches the function's address, thereby confirming the correct operation of bits 2-17, 19-21, 25, and 28 of the PC register.

## Clock Testing

The clock test is designed to verify the integrity and proper operation of the clock sources. It checks the clock frequency and stability to ensure the MCU's operation is within the specified limits.

The following clock sources are tested:

- External 32 Khz crystal oscillator
- Main 40 Mhz crystal oscillator

### External 32KHz Crystal Oscillator

This test uses the XT WDT peripheral to verify the external 32KHz crystal oscillator stability.

If the XT WDT detects a failure of 200 cycles from the 32KHz crystal, it triggers an interrupt. The test then verify if the interrupt was triggered and returns `BIST_ESP_CLOCK_TEST_ERR`.

### Main 40 Mhz Crystal Oscillator

This test uses the External 32Khz as a reference for calculating the 40Mhz crystal oscillator frequency. It calculates the ratio between the 40Mhz and 32Khz clocks and compares it with the expected value. If the ratio is out of the specified range, the test returns `BIST_ESP_CLOCK_TEST_ERR`. The frequency drift is set by the `CONFIG_BIST_CLOCK_PERCENT_FREQUENCY_DRIFT` configuration in `sdkconfig.h` and it represents the maximum allowed deviation from the expected value in percentage.

## CPU Stack tests

### Overflow

The stack is a crucial section of RAM used by the CPU to temporarily store information such as data and addresses.
Due to the limited number of CPU registers, the stack serves as an essential storage area.

In the ESP32-C3, like in many other SoCs, the stack is located at the end of the RAM and grows downward.
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

## How to use the BIST library

The BIST is a cmake library, to use it in your project, you can follow the instructions in the [Critical Safety]() repository. The library is designed to be used in a bare-metal context. It requires that the [ESP-IDF](https://github.com/espressif/esp-idf) is installed in your system.

# ESP-BIST IDF Sample

This sample demonstrates running ESP-BIST tests on the LP core (ULP coprocessor) within an ESP-IDF application. The HP CPU loads and starts the LP-core firmware, which executes BIST tests in a loop.

HP and LP cores communicate over the LP mailbox (`lp_core_mailbox_*`). On ESP32-C5/C6 this uses the software mailbox (shared memory + PMU interrupt). The HP core sends a `BIST_MSG_READY` handshake; the LP core replies with post-boot and periodic runtime result bitmasks.

## Supported SoCs

- ESP32-C5
- ESP32-C6

## Prerequisites

- ESP-IDF v6.0
- Apply the ULP custom linker patch to your IDF installation:

```sh
cd $IDF_PATH
git apply /path/to/esp-bist/samples/idf/patches/idf_ulp_linker.patch
```

This patch adds `ulp_apply_custom_linker_script()` which allows the ULP sub-project to use a custom linker script instead of the stock `lp_core_riscv.ld`.

## Build

```sh
. $IDF_PATH/export.sh
cd samples/idf
idf.py set-target <target_soc>
idf.py build
```

## Flash and Monitor

```sh
idf.py -p PORT flash monitor
```

On success, the LP core runs the enabled BIST tests in a loop and prints results over LP UART.

## Configuration

```sh
idf.py menuconfig
```

Relevant options:

- **Component config > Ultra Low Power (ULP) Co-processor** — LP core enable and reserved memory size.
- **Component config > ESP-BIST** — enable individual BIST tests.

## BIST Tests Available on LP Core

| Test | Kconfig option |
|------|----------------|
| CPU registers | `CONFIG_ESP_BIST_CPU_REG_TEST` |
| CPU CSRs | `CONFIG_ESP_BIST_CPU_CSR_REG_TEST` |
| RAM (March A/X / Abraham) | `CONFIG_ESP_BIST_MEMORY_RAM_TEST` |
| Stack overflow | `CONFIG_ESP_BIST_STACK_TEST` |
| Flash CRC | `CONFIG_ESP_BIST_MEMORY_FLASH_TEST` |
| LP watchdog | `CONFIG_ESP_BIST_WDT_TIMEOUT_US` |

### RAM test notes

- **Post-boot:** March-X followed by `bist_ram_test_abraham_full()` (complete pair schedule).
- **Runtime:** March-A followed by `bist_ram_test_abraham()` (one partition pair per round).
- LP builds default `CONFIG_ESP_BIST_RAM_PARTITION_SIZE` to **256 words** (1 KiB partitions, 2 KiB backup buffer) to fit within the ~16 KiB LP SRAM.

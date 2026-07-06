# ESP-BIST Zephyr LP Core Self-Test Sample

This sample demonstrates running [ESP-BIST](../../README.md) safety tests on the
ESP32 Low-Power (LP) core under Zephyr RTOS, with results validated on the
High-Performance (HP) core using the `ztest` framework.

## Overview

The sample is a **sysbuild** multi-image application with two firmware images:

| Image | Core | Role |
|-------|------|------|
| `zephyr_bist` (main) | HP core | Receives test results via mailbox, runs `ztest` assertions |
| `zephyr_bist_remote` | LP core | Executes ESP-BIST tests, reports results via mailbox |

### Intercore handshake

The HP and LP cores synchronise through the hardware mailbox (`mbox`):

1. Both cores boot independently after reset.
2. The LP core registers its mailbox RX callback and **waits** for a
   `BIST_MSG_READY` message from the HP core.
3. The HP core registers its mailbox RX callback and sends `BIST_MSG_READY`.
4. The LP core receives the ready signal, executes tests, and sends results back.

This handshake ensures no results are lost regardless of boot timing or
`CONFIG_BOOT_DELAY` values used by CI tooling.

### Test phases

The LP core runs tests in two phases:

**Post-boot** (runs once after handshake):
- CPU register test
- RAM March-X test
- Flash CRC integrity test

**Runtime** (runs periodically every 500 ms):
- CPU register test
- RAM March-A test

Results are encoded as a `uint32_t` bitmask:

| Bit | Meaning |
|-----|---------|
| 0 | CPU register test passed |
| 1 | CPU CSR register test passed |
| 2 | RAM March-A test passed |
| 3 | RAM March-X test passed |
| 4 | Flash CRC test passed |
| 30 | Runtime flag (distinguishes runtime from post-boot) |
| 31 | Done sentinel |

## Supported boards

- `esp32c5_devkitc/esp32c5/hpcore`
- `esp32c6_devkitc/esp32c6/hpcore`

## Requirements

- Zephyr SDK with the `riscv64-zephyr-elf` toolchain
- ESP-BIST registered as an extra module:

```bash
export ZEPHYR_EXTRA_MODULES="/path/to/esp-bist"
```

## Building

```bash
west build -p -b esp32c6_devkitc/esp32c6/hpcore \
    /path/to/esp-bist/samples/zephyr \
    --sysbuild \
    -D ZEPHYR_EXTRA_MODULES="/path/to/esp-bist"
```

## Flashing

```bash
west flash
```

This flashes three images: MCUboot bootloader, the HP core application, and the
LP core binary.

## Running

After flashing, the device resets and the test suite runs automatically. Connect
to the HP core serial port (typically `/dev/ttyUSB0` at 115200 baud) to see the
`ztest` output:

```
*** Booting Zephyr OS build v4.4.0 ***
Running TESTSUITE bist_lp
===================================================================
START - test_postboot_cpu_reg
 PASS - test_postboot_cpu_reg in 0.001 seconds
===================================================================
START - test_postboot_flash_crc
 PASS - test_postboot_flash_crc in 0.001 seconds
===================================================================
START - test_postboot_ram_march_x
 PASS - test_postboot_ram_march_x in 0.001 seconds
===================================================================
START - test_runtime_cpu_reg
 PASS - test_runtime_cpu_reg in 0.001 seconds
===================================================================
START - test_runtime_ram_march_a
 PASS - test_runtime_ram_march_a in 0.001 seconds
===================================================================
TESTSUITE bist_lp succeeded
```

The LP core serial port (typically `/dev/ttyUSB1`) shows the BIST execution log:

```
[LP BIST] Waiting for HP core ready signal...
[LP BIST] HP core ready, starting tests
[LP BIST] === Post-boot tests ===
[LP BIST] CPU reg test... PASS (0)
[LP BIST] RAM March-X test... PASS (0)
[LP BIST] Flash CRC test... PASS (0)
[LP BIST] === Runtime tests (periodic) ===
[LP BIST] CPU reg test... PASS (0)
[LP BIST] RAM March-A test... PASS (0)
```

## Running with Twister

```bash
west twister -p esp32c6_devkitc/esp32c6/hpcore \
    -T /path/to/esp-bist/samples/zephyr \
    --device-testing --flash-before \
    --device-serial /dev/ttyUSB0 --west-flash --west-runner esp32 \
```

## Project structure

```
samples/zephyr/
├── CMakeLists.txt            # HP core build
├── Kconfig.sysbuild          # Auto-selects LP core board
├── prj.conf                  # HP core config (ztest, mbox, ULP)
├── sysbuild.cmake            # Adds LP core as external project
├── testcase.yaml             # Twister test definition
├── src/
│   └── main.c                # HP core: mbox RX + ztest assertions
├── boards/
│   ├── esp32c5_devkitc_esp32c5_hpcore.overlay
│   └── esp32c6_devkitc_esp32c6_hpcore.overlay
└── remote/                   # LP core application
    ├── CMakeLists.txt
    ├── prj.conf              # LP core config (ESP-BIST tests)
    ├── src/
    │   └── main.c            # LP core: handshake + BIST tests
    └── boards/
        ├── esp32c5_devkitc_esp32c5_lpcore.overlay
        └── esp32c6_devkitc_esp32c6_lpcore.overlay
```

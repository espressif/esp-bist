# ESP-BIST Zephyr Host Diagnostics Sample

This sample demonstrates [ESP-BIST](../../README.md) Host Diagnostics on Zephyr:
the ESP32 Low-Power (LP) core runs the safety companion, and the
High-Performance (HP) core runs the host agent and prints what the companion
reports. It mirrors [`samples/idf`](../idf) so both platforms read the same way.

It is deliberately the happy path only, so it works as a starting point for your
own application. The assertions and the fail-closed validation live in
[`tests/integration/hd_zephyr`](../../tests/integration/hd_zephyr).

Host Diagnostics is companion supervision of a Quality-Managed host OS, not a
Safety OS. The LP core is on the same die as the HP core, so it is **not** a
safety island or a fully independent industrial safety channel; see
[`HOST_DIAGNOSTICS_ARCHITECTURE.md`](../../HOST_DIAGNOSTICS_ARCHITECTURE.md) §8.3.

## Overview

The sample is a **sysbuild** multi-image application with two firmware images:

| Image | Core | Role |
|-------|------|------|
| `zephyr_bist` (main) | HP core | Host agent: answers audits, prints companion status |
| `zephyr_bist_remote` | LP core | Safety companion: LP self-BIST, host audit schedule, safe state |

### Supervision loop

1. Both cores boot independently after reset.
2. The LP companion initialises the mailbox and **waits** for `AGENT_READY`.
3. The HP agent starts, sends `AGENT_READY`, and starts its high-priority
   worker thread.
4. The companion runs post-boot LP BIST, reports `LP_STATUS`, and arms the LP
   watchdog.
5. Each runtime round the companion re-runs LP BIST, reports `LP_STATUS`, and
   (with `CONFIG_ESP_BIST_HD_AUDIT_QA`) issues one Q&A challenge that the agent
   must answer correctly inside `CONFIG_ESP_BIST_HD_CHALLENGE_WINDOW_US`.
6. Any failed audit puts the companion in safe state: it stops feeding the LP
   watchdog, which resets the chip.

Frames travel over the Espressif `mbox` shared-memory window plus doorbell
interrupt, so the overlays reserve one 4-word frame per direction
(`shared-memory-size = <0x20>`).

### Test phases

**Post-boot** (runs once, before the LP watchdog is armed):
- CPU register test
- RAM March-X test
- RAM Abraham full test
- Flash CRC integrity test

**Runtime** (every `CONFIG_ESP_BIST_RUNTIME_TEST_INTERVAL_MS`):
- CPU register test
- RAM March-A test
- RAM Abraham test (one partition pair per round)
- Stack overflow check
- Host Q&A challenge

Results are encoded as a `uint32_t` bitmask (see `bist_hd_protocol.h`):

| Bit | Meaning |
|-----|---------|
| 0 | CPU register test passed |
| 1 | CPU CSR register test passed |
| 2 | RAM March-A test passed |
| 3 | RAM March-X test passed |
| 4 | Flash CRC test passed |
| 5 | Stack overflow check passed |
| 6 | RAM Abraham test passed |
| 30 | Runtime round |
| 31 | Post-boot round |

The LP image sets `CONFIG_ESP_BIST_RAM_PARTITION_SIZE=64` (512 B partitions,
512 B backup buffer) so the companion fits in the ~16 KiB LP SRAM.

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

After flashing, the device resets and the sample runs automatically. Connect to
the HP core serial port (typically `/dev/ttyUSB0` at 115200 baud):

```
*** Booting Zephyr OS build v4.4.0 ***
Host Diagnostic Agent started
test_HD_agent_ready:PASS
=== Post-boot BIST results ===
test_BIST_cpu_reg:PASS
test_BIST_ram_march_x:PASS
test_BIST_ram_abraham:PASS
test_BIST_flash_crc:PASS
--- Runtime loop 1/5 ---
=== Runtime BIST results ===
test_BIST_runtime_cpu_reg:PASS
test_BIST_runtime_ram_march_a:PASS
test_BIST_runtime_ram_abraham:PASS
test_BIST_runtime_stack_check:PASS
...
test_HD_challenge:PASS
BIST_RESULT:PASS
```

Completing all five loops is itself the Q&A evidence: the companion issues one
challenge per round and stops reporting once it enters safe state, so a wrong or
late answer would end the output early.

`sample.yaml` wraps this in a twister console test so CI keeps the sample honest:

```bash
west twister -p esp32c6_devkitc/esp32c6/hpcore \
    -T /path/to/esp-bist/samples/zephyr \
    --device-testing --flash-before \
    --device-serial /dev/ttyUSB0 --west-flash --west-runner esp32
```

## Validating fail-closed behaviour

Making the host misbehave — answering with the wrong key, or missing the
challenge window — belongs to
[`tests/integration/hd_zephyr`](../../tests/integration/hd_zephyr), together with
the ztest assertions on the companion's verdict. Keeping it out of here means a
fail-closed build and a product build differ only in that test app; nothing in
`src/bist/` or in this sample has a test branch.

## Project structure

```
samples/zephyr/
├── CMakeLists.txt            # HP core build
├── Kconfig.sysbuild          # Auto-selects LP core board
├── prj.conf                  # HP core config (mbox, ULP, Host Diagnostics)
├── sysbuild.cmake            # Adds LP core as external project
├── sample.yaml               # Twister console smoke test
├── src/
│   └── main.c                # HP core: host agent + status reporting
├── boards/
│   ├── esp32c5_devkitc_esp32c5_hpcore.overlay
│   └── esp32c6_devkitc_esp32c6_hpcore.overlay
└── remote/                   # LP core application
    ├── CMakeLists.txt
    ├── prj.conf              # LP core config (ESP-BIST + companion)
    ├── src/
    │   └── main.c            # LP core: companion init + runtime loop
    └── boards/
        ├── esp32c5_devkitc_esp32c5_lpcore.overlay
        └── esp32c6_devkitc_esp32c6_lpcore.overlay
```

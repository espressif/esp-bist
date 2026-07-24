# ESP-BIST NuttX Sample

This sample demonstrates running ESP-BIST tests on the LP core (ULP coprocessor)
under NuttX. The HP CPU loads and starts the LP-core firmware, then exchanges
handshake and result bitmasks with the LP core over `/dev/lp_mailbox`.

## What this sample covers

| Phase     | Tests                                      |
|-----------|--------------------------------------------|
| Post-boot | CPU reg, CSR, RAM March-X, Flash CRC       |
| Runtime   | CPU reg, CSR, RAM March-A, stack check, LP WDT feed |

Results are encoded as a `uint32_t` bitmask:

| Bit | Meaning |
|-----|---------|
| 0 | CPU register test passed |
| 1 | CPU CSR register test passed |
| 2 | RAM March-A test passed |
| 3 | RAM March-X test passed |
| 4 | Flash CRC test passed |
| 5 | Stack overflow check passed |
| 30 | Runtime flag (distinguishes runtime from post-boot) |
| 31 | Post-boot complete flag |

## Supported SoCs

- ESP32-C6
- ESP32-P4

## Prerequisites

- [NuttX build environment](https://developer.espressif.com/blog/2020/11/nuttx-getting-started/) for RISC-V
- NuttX with LP-core / ULP support for a [SoC listed above](#Supported-SoCs)
- [Custom Apps](https://nuttx.apache.org/docs/latest/guides/customapps.html) wiring (symlink below)
- `kconfig-merge` from [kconfig-frontends](https://bitbucket.org/nuttx/tools/src/master/kconfig-frontends/)

## Link sample into NuttX apps

```bash
-ln -s /path/to/esp-bist/samples/nuttx /path/to/apps/external
```

## Configure, build and flash (CMake)

Base board: **`esp32c6-devkitc:ulp`**.
Extra options: [`nuttx_bist/configs/espressif`](nuttx_bist/configs/espressif).

```bash
cd /path/to/nuttx
cmake -B build-bist -DBOARD_CONFIG=esp32c6-devkitc:ulp -GNinja
kconfig-merge -m -O build-bist build-bist/.config \
    /path/to/esp-bist/samples/nuttx/nuttx_bist/configs/espressif
cmake --build build-bist -t olddefconfig
kconfig-merge -m -O build-bist build-bist/.config \
    /path/to/esp-bist/samples/nuttx/nuttx_bist/configs/espressif
cmake -B build-bist -GNinja
cmake --build build-bist -j$(nproc)
ESPTOOL_PORT=<PORT_NAME> cmake --build build-bist -t flash
```

## Configure, build and flash (Make)

Base board: **`esp32c6-devkitc:ulp`**.
Extra options: [`nuttx_bist/configs/espressif`](nuttx_bist/configs/espressif).

```bash
cd /path/to/nuttx
./tools/configure.sh esp32c6-devkitc:ulp
kconfig-merge -m .config /path/to/esp-bist/samples/nuttx/nuttx_bist/configs/espressif
make olddefconfig
kconfig-merge -m .config /path/to/esp-bist/samples/nuttx/nuttx_bist/configs/espressif
make
make flash ESPTOOL_PORT=<PORT_NAME>
```

## Monitor

```bash
picocom -b 115200 <PORT_NAME>
```

## Run

```text
nsh> nuttx_bist
```

### Automated device test

```bash
cd /path/to/esp-bist/samples/nuttx
pytest pytest_device* --port=<PORT_NAME> --baud=115200
```

The test uses pytest-embedded serial at 115200 baud, waits for `nsh>`, runs
`nuttx_bist`, and checks the same PASS markers as below.

Expected markers:

```
=== Post-boot BIST results ===
test_BIST_cpu_reg:PASS
test_BIST_cpu_csr:PASS
test_BIST_ram_march_x:PASS
test_BIST_flash_crc:PASS
--- Runtime loop 1/10 ---
=== Runtime BIST results ===
test_BIST_runtime_cpu_reg:PASS
test_BIST_runtime_cpu_csr:PASS
test_BIST_runtime_ram_march_a:PASS
test_BIST_runtime_stack_check:PASS
...
BIST_RESULT:PASS
```

## Project structure

```
samples/nuttx/
├── CMakeLists.txt              # Category: nuttx_add_subdirectory + Kconfig menu
├── Make.defs                   # Includes nuttx_bist/Make.defs
├── Makefile
├── README.md
└── nuttx_bist/
    ├── CMakeLists.txt          # HP app + ULP via esp_ulp.cmake / src/bist
    ├── Kconfig                 # ESP_NUTTX_BIST + BIST symbols
    ├── Make.defs
    ├── Makefile
    ├── configs/
    │   └── espressif           # Fragment merged with kconfig-merge
    ├── bist_protocol.h         # HP/LP shared protocol
    ├── nuttx_bist_main.c       # HP: load LP firmware, print results
    └── ulp/
        ├── Makefile
        └── main.c              # LP: post-boot + runtime BIST
```

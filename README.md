# ESP-BIST

This repository holds the tests, samples and SoC files for the Espressif's Built-In Self Test (BIST) library.

The BIST library is a cmake library with a set of routines designed to verify the integrity and proper operation of the hardware components in Espressif's SoCs. The library includes tests for the CPU registers, configuration and status registers (CSRs), volatile memory, non-volatile memory, cpu stack, program counter (PC), and clock sources. The tests are intended for use in safety-critical applications that require compliance with the IEC 60730 Class B standard.

This repository includes the necessary files to build an application that runs the BIST library tests.

## Supported SoCs

- ESP32-C3
- ESP32-C5
- ESP32-C6
- ESP32-C61
- ESP32-H2

## License

This repository is licensed under the LGPL-3.0 license. For more information, see the [LICENSE](LICENSE) file.

## Download

To download the repository, execute the following command:

```sh
git clone https://github.com/espressif/esp-bist.git
```

## Dev Container

This repository includes a dev container configuration to ease the development process. The dev container includes all the necessary tools and libraries to build and test the BIST library.

1. Install the Dev Container CLI:

```sh
npm install -g @devcontainers/cli
```

2. Build the dev container image:

```sh
devcontainer build --workspace-folder .
```

3. Start a dev container in your workspace folder:

```sh
devcontainer up --workspace-folder .
```

4. You can run commands in this dev container, for example:

```sh
devcontainer exec --workspace-folder . bash
```
## Set up IDF environment variables

Before building the project, make sure to set the IDF_PATH environment variable to access all the necessary tools.

To set up the IDF environment variables, execute the following command:

```sh
. $IDF_PATH/export.sh
```

## Bootloader

The Critical Firmware is designed to run on top of the MCUboot bootloader.

The current supported version of MCUboot is 2.2.0.

MCUboot is located at opt/mcuboot.

```sh
cd /opt/mcuboot/boot/espressif
cmake -DCMAKE_TOOLCHAIN_FILE=tools/toolchain-<SOC_TARGET>.cmake -DMCUBOOT_TARGET=<SOC_TARGET> -DESP_HAL_PATH=$IDF_PATH -B build -GNinja
ninja -C build
```

Replace `<SOC_TARGET>` with the target SoC.

For more information about MCUBoot bootloader, please refer to [documentation](https://docs.mcuboot.com/readme-espressif.html)

## Build

Inside any of the tests or samples directories:

```sh
cmake -DSOC_TARGET=<SOC_TARGET> -B build -GNinja
ninja -C build
```

Replace `<SOC_TARGET>` with the target SoC.

## Project Configuration

The project uses Kconfig to configure the BIST library. Application developers can open a terminal-based project configuration menu using the following command:

```sh
ninja -C build menuconfig
```

The file `bist.conf` in the root directory of any application is mandatory and can be used to set the default configuration the project. The configuration file is automatically included in the build process. If empty, the BIST library will use the default configuration.

## QEMU

To run on the QEMU emulator, execute the following command:

```sh
ninja -C build qemu
```

### QEMU Debug

To run tests using the QEMU emulator in debug mode, execute the following command:

```sh
ninja -C build qemu_debug
```

In another terminal, run gdb with the following command:

```sh
riscv32-esp-elf-gdb build/<app_name>.elf -ex "target remote :1234" -ex "tb main" -ex "c"
```

## Flash to Device

By default, the flashing process assumes the board is connected to `/dev/ttyUSB0`.

```sh
ninja -C build flash
```

If the board does not have MCUboot, use this command to flash the bootloader:

```sh
ninja -C build flash_boot
```

### Changing Serial Port

The serial port is taken from the `ESPPORT` environment variable during the configuration stage.
If `ESPPORT` is unset during configuration, the default is `/dev/ttyUSB0`.

To use a different port, set `ESPPORT` during configuration, or re-run the `cmake` command with
`ESPPORT` set.

Example:

```sh
export ESPPORT=<DEVICE_PATH>
cmake -DSOC_TARGET=<SOC_TARGET> -B build -GNinja
```

Alternatively:

```sh
ESPPORT=<DEVICE_PATH> cmake -DSOC_TARGET=<SOC_TARGET> -B build -GNinja
```

## Monitor

To monitor the device output, execute the following command:

```sh
ninja -C build monitor
```

The `monitor` target uses the same serial port as `flash`: the value of `ESPPORT` from the last
`cmake` run.

To close the monitor, press `Ctrl+]`.

## Testing

The tests are located in the `tests` directory. The tests are divided into two categories: QEMU and device testing.

### Test applications

Available test applications (each in `tests/<name>/`):

| Application         | Description |
|---------------------|-------------|
| `cpu_reg_test`      | CPU register integrity |
| `cpu_stack_test`    | CPU stack overflow detection |
| `ram_test`          | Volatile memory (March A / March X) |
| `clock_test`        | Clock sources (32 kHz and 40 MHz oscillators) |
| `flash_test`        | Non-volatile memory (CRC32) |
| `pc_test`           | Program counter and indirect time-slot monitoring |
| `digital_io_test`   | Digital I/O (GPIO) |
| `analog_io_test`    | Analog I/O (ADC) |
| `wdt_test`          | Main system watchdog timer |
| `windowed_wdt_test` | Windowed watchdog: normal operation, underflow detection, and consecutive feed cycles |
| `esp_timer_test`    | High-resolution software timers (e.g. one-shot) |

### Qemu testing

We use Pytest in conjunction with Unity, GDB scripting and QEMU to run the test suite. The tests are meant to verify the correct execution of the BIST library and to introduce faults deliberately to verify that the system can detect and recover from such situations, either by restoring the correct data from backup or entering a safe state. To run the tests, execute the following command:

```sh
pytest pytest_qemu_* --executable=<app_name> --target=<SOC_TARGET> --junitxml=build/tests/report.xml
```

The test will output a report in the `build/tests` directory.

### Device testing

To run the tests on the device, execute the following command:

```sh
pytest pytest_device_* --junitxml=build/tests/report.xml
```

### Kconfig Test Configuration

Each test directory contains a `bist.conf` file that controls which BIST modules are compiled. This file uses Kconfig syntax to enable or disable individual tests:

```
CONFIG_ESP_BIST_CPU_REG_TEST=y
CONFIG_ESP_BIST_CPU_CSR_REG_TEST=y
CONFIG_ESP_BIST_MEMORY_RAM_TEST=n
...
```

## Samples

The samples are located in the `samples` directory. The samples demonstrate how to use the BIST library to verify the integrity and proper operation of the hardware components.

## BIST Library

The proper documentation for the BIST library can be found in the [`src/bist`](src/bist) directory.

## MCP Server (AI Assistant Integration)

This repository ships with a [Model Context Protocol](https://modelcontextprotocol.io/) server in [`mcp-server/`](mcp-server/) that exposes ESP-BIST's documentation, API reference, Kconfig options, and source code to AI coding assistants in Cursor and VS Code (Copilot Chat).

One-time setup:

```sh
cd mcp-server
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

Opening this repository in either Cursor or VS Code automatically registers the server via the committed [`.cursor/mcp.json`](.cursor/mcp.json) and [`.vscode/mcp.json`](.vscode/mcp.json) files. See [`mcp-server/README.md`](mcp-server/README.md) for details.

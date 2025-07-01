# ESP-BIST

This repository holds the tests, samples and SoC files for the Espressif's Built-In Self Test (BIST) library.

The BIST library is a cmake library with a set of routines designed to verify the integrity and proper operation of the hardware components in Espressif's SoCs. The library includes tests for the CPU registers, configuration and status registers (CSRs), volatile memory, non-volatile memory, cpu stack, program counter (PC), and clock sources. The tests are intended for use in safety-critical applications that require compliance with the IEC 60730 Class B standard.

This repository includes the necessary files to build an application that runs the BIST library tests.

## Supported SoCs

- ESP32-C3

## License

This repository is licensed under the LGPL-3.0 license. For more information, see the [LICENSE](LICENSE) file.

## Download

To download the repository, execute the following command:

```sh
git clone --recursive https://github.com/espressif/esp-bist.git
```

Fetch the submodules with the following command:

```sh
git submodule update --init --recursive
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

MCUboot is a located at opt/mcuboot.

```sh
cd /opt/mcuboot/boot/espressif
cmake -DCMAKE_TOOLCHAIN_FILE=tools/toolchain-esp32c3.cmake -DMCUBOOT_TARGET=esp32c3 -DESP_HAL_PATH=$IDF_PATH -B build -GNinja
ninja -C build
```

## Build

Inside any of the tests or samples directories:

```sh
cmake -DSOC_TARGET=esp32c3 -B build -GNinja
ninja -C build
```

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
riscv32-esp-elf-gdb build/critical_fw_esp32c3.elf -ex "target remote :1234" -ex "tb main" -ex "c"
```

## Flash to Device

By default, the flashing process assumes the board is connected to /dev/ttyUSB0. The port can be set with `-DESP_PORT`.

```sh
ninja -C build flash -DESP_PORT=/dev/ttyUSBx
```

## Monitor

To monitor the device output, execute the following command:

```sh
ninja -C build monitor
```

To close the monitor, press `Ctrl+]`.

## Watchdog

The Main System Watchdog Timer (MWDT) of Timer 1 is enabled by default. The MWDT is a hardware watchdog timer that can be used to monitor the system's operation and detect potential failures. The MWDT is configured to trigger a system reset if the system fails to clear the watchdog within a specified time frame. The MWDT is enabled by default to ensure the system can recover from potential failures and maintain operational integrity.

Before resetting the system, the watchdog can trigger an interrupt to allow the application to perform any necessary operation. The interrupt handler can be defined in the application and registered using `wdt_register_callback(void (*callback)(void *), void *arg)`.

The `CONFIG_WDT_TIMEOUT_US` configuration defines the watchdog timeout in microseconds. The watchdog timeout should be set according to the system's requirements, ensuring it provides sufficient time for the application to complete its operations. The interrupt will be triggered when the watchdog timer reaches its timeout value. The system will be reset after double the timeout value.

### Windowed Watchdog

The system implements a Windowed Watchdog using a private timer in conjunction with the main watchdog timer. This mechanism provides indirect time-slot monitoring of the application execution flow.

A Program Counter (PC) Fault is detected if the watchdog timer is not reset within a specified time window. If an underflow occurs (early reset), subsequent attempts to reset the watchdog within the same time window will fail, ensuring fault detection.

The Underflow value is defined in microseconds by `CONFIG_WDT_UNDERFLOW_US` in the `sdkconfig.h` file. The overflow value is the same as the main watchdog timer timeout, defined by `CONFIG_WDT_TIMEOUT_US`.

The windowed watchdog can be enabled with `void wdt_init_windowed(uint32_t underflow_timeout_us)`.

## Testing

The tests are located in the `tests` directory. The tests are divided into two categories: QEMU and device testing.

### Qemu testing

We use Pytest in conjunction with Unity, GDB scripting and QEMU to run the test suite. The tests are meant to verify the correct execution of the BIST library and to introduce faults deliberately to verify that the system can detect and recover from such situations, either by restoring the correct data from backup or entering a safe state. To run the tests, execute the following command:

```sh
pytest pytest_qemu_* --junitxml=build/tests/report.xml
```

The test will output a report in the `build/tests` directory.

### Device testing

To run the tests on the device, execute the following command:

```sh
pytest pytest_device_* --junitxml=build/tests/report.xml

```

## Samples

The samples are located in the `samples` directory. The samples demonstrate how to use the BIST library to verify the integrity and proper operation of the hardware components.

## BIST Library

The proper documentation for the BIST library can be found in the [`src/bist`](src/bist) directory.

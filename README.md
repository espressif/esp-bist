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

## Bootloader

The Critical Firmware is designed to run on top of the MCUboot bootloader.

MCUboot is a submodule located at modules/mcuboot.

To build and flash MCUboot, follow the instructions in https://github.com/mcu-tools/mcuboot/blob/main/docs/readme-espressif.md

## Build

Before building the project, make sure to set the IDF_PATH environment variable to access all the necessary tools.

Inside any of the tests or samples directories:

```sh
cmake -DSOC_TARGET=esp32c3 -B build -GNinja
ninja -C build
```

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
ninja -C build flash -DESP_PORT=/dev/ttyUSB0
```

## Watchdog

The Main System Watchdog Timer (MWDT) of Timer 1 is enabled by default. The MWDT is a hardware watchdog timer that can be used to monitor the system's operation and detect potential failures. The MWDT is configured to trigger a system reset if the system fails to clear the watchdog within a specified time frame. The MWDT is enabled by default to ensure the system can recover from potential failures and maintain operational integrity.

Before resetting the system, the watchdog can trigger an interrupt to allow the application to perform any necessary operation. The signature for the interrupt handler is `void mwdt_callback(void *args)` and should be defined in the application code.

The `CONFIG_WDT_TIMEOUT_MS` macro in the `sdkconfig.h` file defines the watchdog timeout in milliseconds. The default value is 2000 ms. The watchdog timeout should be set according to the system's requirements, ensuring it provides sufficient time for the application to complete its operations. The interrupt will be triggered when the watchdog timer reaches its timeout value. The system will be reset after double the timeout value.

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

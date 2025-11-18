# Zephyr and BIST Example Application

## Overview

This sample demonstrates how to integrate the Zephyr OS on HP (High-Performance) and LP (Low-Power) cores of the ESP32-C6, along with the ESP-BIST library for built-in self-test functionality.

Before running this sample, ensure you have the Zephyr environment set up correctly. You can find the setup instructions in the [Zephyr's Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html).

### Supported Boards

- `esp32c6_devkitc`

## Build

You can build and run the sample application using the `west` tool.

There are two main ways to build the application: using a local repository or adding `ESP-BIST` to the Zephyr manifest.

### Local Repository

To build the application, run the following command:

```sh
west build -p -b esp32c6_devkitc/esp32c6/hpcore <path/to/esp-bist/samples/zephyr> -D ZEPHYR_EXTRA_MODULES=<path/to/esp-bist/> --sysbuild
```

### Zephyr Manifest

You can add the `ESP-BIST` library to the Zephyr manifest by modifying the `west.yml` file in your Zephyr workspace. Add the following entry under `remotes`:

```yaml
- name: espressif
  url-base: https://github.com/espressif
```

Then, add the `esp-bist` module under `projects`:

```yaml
- name: esp-bist
  remote: espressif
  revision: main
  path: modules/lib/esp-bist
```

Then, run the following command to update the manifest:

```sh
west update
```
The ESP-BIST library will be cloned into the `modules/lib/esp-bist` directory of your Zephyr workspace.

After updating the manifest, you can build the application using:

```sh
west build -p -b esp32c6_devkitc/esp32c6/hpcore modules/lib/esp-bist/samples/zephyr --sysbuild
```

## Flash and Monitor

Once you have built the application, run the following command to flash it and monitor the output:

```shell
west flash && west espressif monitor
```

## LP Core Console

Connect an UART to USB adapter to the LP Core's LP UART pins (TX: GPIO 5, RX: GPIO 4, GND: GND) and open a terminal emulator.

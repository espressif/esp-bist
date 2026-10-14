Getting Started
===============

ESP-BIST is a library with routines designed to verify the integrity and proper operation of hardware components in Espressif's SoCs. The library provides the following self-tests:

- CPU Register Test
- Configuration and Status Registers (CSR) Test
- Interrupt Handling and Execution Test
- Volatile Memory Test (RAM)
- Non-Volatile Memory Test (Flash)
- CPU Stack Test
- Program Counter Test
- Clock Source Test (main crystal and External 32kHz Crystal)
- Watchdog Timer Test
- Digital I/O Test
- Analog I/O Test

These tests are intended for use in safety-critical applications that require compliance with the IEC 60730 Class B standard.

License
-------

This repository is licensed under the `GNU Lesser General Public License version 3.0 (LGPL-3.0) <https://www.gnu.org/licenses/lgpl-3.0.html>`_.

The LGPL-3.0 is a copyleft license that allows you to:

- **Use** the library in your applications (including proprietary applications)
- **Modify** the library source code
- **Distribute** the library and your applications
- **Link** the library with proprietary code

Key obligations when distributing:

- If you modify the library itself, you must release those modifications under LGPL-3.0
- You must provide the source code of the library (and any modifications) when distributing
- You must include a copy of the LGPL-3.0 license with distributions

Your proprietary application code that uses the library (without modifying the library itself) can remain under your own license terms.

For the complete license text and detailed terms, see the `LICENSE <../LICENSE>`_ file in the repository root, or visit the `official LGPL-3.0 page <https://www.gnu.org/licenses/lgpl-3.0.html>`_.

Supported SoCs
--------------

The BIST library is supported on the following SoCs:

- ESP32-C3
- ESP32-C5
- ESP32-C6
- ESP32-C61
- ESP32-H2
- ESP32-H4
- ESP32-P4

.. list-table:: SoC Comparison
   :header-rows: 1
   :widths: 9 12 7 10 6 6 30

   * - SoC
     - CPU / Arch
     - Default CPU (MHz)
     - SRAM (KB)
     - PSRAM
     - GPIO
     - Specific Test Cases & Differences
   * - ESP32-C3
     - Single-core RISC-V
     - 160
     - 400 + 8 RTC
     - No
     - 22 / 16
     - - Has ADC voltage reference test
       - Has 32 kHz crystal XT WDT test
       - Custom CSR performance monitor CSRs (PCER, PCMR, PCCR)
       - Tests 25 CSRs (no PMA address CSRs)
       - No hardware FPU
   * - ESP32-C5
     - Single-core RISC-V + LP RISC-V
     - 80
     - 384 HP + 16 LP
     - Yes
     - 29
     - - Tests 39 CSRs: adds 12 PMA address CSRs plus target-specific
       - No hardware FPU
       - No ADC reference test
       - Lacks XT WDT hardware
   * - ESP32-C6
     - Single-core RISC-V + LP RISC-V
     - 160
     - 512 HP + 16 LP
     - No
     - 30 / 22
     - - Tests 37 CSRs: includes 12 PMA address CSRs
       - Custom CSR performance monitor CSRs (PCER, PCMR, PCCR)
       - No hardware FPU
       - No ADC reference test
       - Lacks XT WDT hardware
   * - ESP32-C61
     - Single-core RISC-V
     - 160
     - 320
     - Yes
     - 30
     - - Tests 37 CSRs: includes 12 PMA address CSRs
       - No hardware FPU
       - No ADC reference test
       - Lacks XT WDT hardware
   * - ESP32-H2
     - Single-core RISC-V
     - 96
     - 320 + 4 RTC
     - No
     - 19
     - - Tests 37 CSRs: includes 12 PMA address CSRs
       - Custom CSR_PC performance monitor CSRs (PCER, PCMR, PCCR)
       - No hardware FPU
       - No ADC reference test
       - Lacks XT WDT hardware
   * - ESP32-H4
     - Dual-core RISC-V
     - 32
     - 320
     - Yes
     - 35
     - - Has hardware FPU: tests 32 single-precision FPU registers (f0–f31) and 3 FPU CSRs
       - Lacks ADC peripheral in hardware
       - Lacks XT WDT hardware
   * - ESP32-P4
     - Dual-core RISC-V + LP RISC-V
     - 400
     - 768 KB HP L2MEM + 32 KB LP + 8 KB TCM
     - Yes
     - 55
     - - Has hardware FPU: tests 32 single-precision FPU registers (f0–f31) and 3 FPU CSRs
       - 5-region Program Counter test including TCM memory
       - No ADC reference test
       - Lacks XT WDT hardware

Default CPU is ``CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ`` in ``src/soc/<soc>/include/sdkconfig.h``.

Download
--------

To download the repository, execute the following command:

.. code-block:: bash

   git clone https://github.com/espressif/esp-bist.git

Development Environment
-----------------------

Linux System Requirements
^^^^^^^^^^^^^^^^^^^^^^^^^

This library has been developed and tested on Linux systems. The following Linux environment is recommended:

**Tested Linux Distribution:**
- **Ubuntu 22.04** (or compatible Debian-based distributions)

Dev Container (Recommended)
^^^^^^^^^^^^^^^^^^^^^^^^^^^

This repository includes a dev container configuration to ease the development process. The dev container includes all the necessary tools and libraries to build and test the BIST library.

1. Install the Dev Container CLI:

   .. code-block:: bash

      npm install -g @devcontainers/cli

2. Build the dev container image:

   .. code-block:: bash

      devcontainer build --workspace-folder .

3. Start a dev container in your workspace folder:

   .. code-block:: bash

      devcontainer up --workspace-folder .

4. You can run commands in this dev container, for example:

   .. code-block:: bash

      devcontainer exec --workspace-folder . bash

Set Up IDF Environment Variables
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Before building the project, make sure to set the ``IDF_PATH`` environment variable to access all the necessary tools.

To set up the IDF environment variables, execute the following command:

.. code-block:: bash

   . $IDF_PATH/export.sh

Bootloader
----------

The Critical Firmware is designed to run on top of the MCUboot bootloader.

The current supported version of MCUboot is 2.2.0.

In the dev container, MCUboot is located at ``/opt/mcuboot``. To build the bootloader for your target SoC, execute:

.. code-block:: bash

   cd /opt/mcuboot/boot/espressif
   cmake -DCMAKE_TOOLCHAIN_FILE=tools/toolchain-{IDF_TARGET_PATH_NAME}.cmake -DMCUBOOT_TARGET={IDF_TARGET_PATH_NAME} -DESP_HAL_PATH=$IDF_PATH -B build -GNinja
   ninja -C build

Build
-----

Inside any of the tests or samples directories:

.. code-block:: bash

   cmake -DSOC_TARGET={IDF_TARGET_PATH_NAME} -B build -GNinja
   ninja -C build

Project Configuration
---------------------

The project uses Kconfig to configure the BIST library. Application developers can open a terminal-based project configuration menu using the following command:

.. code-block:: bash

   ninja -C build menuconfig

The file ``bist.conf`` in the root directory of any application is mandatory and can be used to set the default configuration for the project. The configuration file is automatically included in the build process. If empty, the BIST library will use the default configuration.

QEMU Emulation
--------------

Run on QEMU
^^^^^^^^^^^

To run on the QEMU emulator, execute the following command:

.. code-block:: bash

   ninja -C build qemu

QEMU Debug Mode
^^^^^^^^^^^^^^^

To run tests using the QEMU emulator in debug mode, execute the following command:

.. code-block:: bash

   ninja -C build qemu_debug

In another terminal, run GDB with the following command:

.. code-block:: bash

   riscv32-esp-elf-gdb build/<app_name>.elf -ex "target remote :1234" -ex "tb main" -ex "c"

Flash to Device
---------------

By default, the flashing process assumes the board is connected to ``/dev/ttyUSB0``.

Flashing the Bootloader
^^^^^^^^^^^^^^^^^^^^^^^

If the board does not have MCUboot flashed, flash the bootloader to the device. This needs to be done only once:

.. code-block:: bash

   ninja -C build flash_boot

Flashing the Application
^^^^^^^^^^^^^^^^^^^^^^^^

To flash the application, execute:

.. code-block:: bash

   ninja -C build flash

Changing Serial Port
^^^^^^^^^^^^^^^^^^^^

The serial port is taken from the ``ESPPORT`` environment variable during the CMake configuration stage. If ``ESPPORT`` is unset during configuration, the default port is ``/dev/ttyUSB0``.

To specify a different port, set ``ESPPORT`` before running CMake or pass it inline:

.. code-block:: bash

   export ESPPORT=/dev/ttyUSBx
   cmake -DSOC_TARGET={IDF_TARGET_PATH_NAME} -B build -GNinja

Alternatively:

.. code-block:: bash

   ESPPORT=/dev/ttyUSBx cmake -DSOC_TARGET={IDF_TARGET_PATH_NAME} -B build -GNinja

Monitor Device Output
---------------------

To monitor the device output, execute the following command:

.. code-block:: bash

   ninja -C build monitor

The ``monitor`` target uses the same serial port as ``flash``: the value of ``ESPPORT`` established during the last CMake configuration run.

To close the monitor, press ``Ctrl+]``.

Testing
-------

The standalone tests are located in the ``tests`` directory. The tests are divided into three categories: QEMU testing, device testing, and host unit testing.

Test Applications
^^^^^^^^^^^^^^^^^

The following standalone test applications are available in ``tests/<name>/``:

.. list-table:: Standalone Test Applications
   :header-rows: 1
   :widths: 25 75

   * - Application
     - Description
   * - ``cpu_reg_test``
     - CPU general-purpose and CSR register integrity
   * - ``cpu_stack_test``
     - CPU stack overflow detection and watermark analysis
   * - ``ram_test``
     - Volatile memory testing (March A, March X, and Abraham algorithms)
   * - ``clock_test``
     - Clock sources (main crystal drift and external 32 kHz crystal watchdog)
   * - ``flash_test``
     - Non-volatile memory integrity via CRC32 validation
   * - ``pc_test``
     - Program counter test and indirect time-slot monitoring
   * - ``interrupt_test``
     - Hardware interrupt handling and execution timing verification
   * - ``digital_io_test``
     - Digital I/O GPIO input/output loopback verification
   * - ``analog_io_test``
     - Analog I/O ADC voltage plausibility and reference checks
   * - ``wdt_test``
     - Main watchdog timer reset and timeout verification
   * - ``windowed_wdt_test``
     - Windowed watchdog underflow and overflow protection

QEMU Testing
^^^^^^^^^^^^

We use Pytest in conjunction with Unity, GDB scripting, and QEMU to run the test suite. The tests verify correct execution of the BIST routines and introduce faults deliberately to verify that the system detects and recovers from faults, restoring valid state or entering a safe stop.

To run tests on QEMU, execute inside the corresponding test directory:

.. code-block:: bash

   pytest pytest_qemu_* --executable=<app_name> --target={IDF_TARGET_PATH_NAME} --junitxml=build/tests/report.xml

The test will output a report in the ``build/tests`` directory.

Device Testing
^^^^^^^^^^^^^^

To run tests directly on target hardware, execute inside the test directory:

.. code-block:: bash

   pytest pytest_device_* --executable=<app_name> --target={IDF_TARGET_PATH_NAME} --junitxml=build/tests/report.xml

Host Unit Testing
^^^^^^^^^^^^^^^^^

``tests/unit/`` contains host-native unit tests that run on the build machine without target hardware or an emulator. Each suite is a self-contained CMake project built and run with ``ctest``:

.. code-block:: bash

   cmake -S tests/unit/<suite> -B build/<suite>_test
   cmake --build build/<suite>_test
   ctest --test-dir build/<suite>_test --output-on-failure

Available standalone suites:

- ``clock_math`` — frequency deviation arithmetic in ``bist_clock_math.h`` (guards against regressions independently of target hardware)

Samples
-------

The standalone sample is ``samples/standalone``. It demonstrates how to use the BIST library to verify the integrity and proper operation of the hardware components.

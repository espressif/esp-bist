# Host Diagnostics fail-closed validation (NuttX)

Proves that a misbehaving HP host is caught by the LP companion and that the
chip resets. This is a test app, not an example — start from
[`samples/nuttx`](../../../samples/nuttx) if you want to see how Host Diagnostics is
integrated.

## What makes it fail

The ESP-BIST library is built exactly as a product would build it. There are no
test hooks in `src/bist/`; the fault comes from this app.

| Config | Fault |
|--------|-------|
| `configs/hd_key_mismatch` | `hd_nuttx_test/CMakeLists.txt` compiles the HP-side sources with `BIST_HD_CHALLENGE_KEY=0x5A5A5A5A`. The ULP image keeps `CONFIG_ESP_BIST_HD_CHALLENGE_KEY`, so the production answer path computes a value the companion rejects on every round. |
| `configs/hd_starved_agent` | The agent is demoted to priority 100. After the first runtime `LP_STATUS`, the app raises itself to `SCHED_PRIORITY_MAX` (255) and busy-waits for 500 ms, so the agent is never scheduled and no `ANSWER` reaches the companion before the window closes. |

Both reach the same end state: the companion stops feeding the LP watchdog and
the watchdog resets the chip. One of the two options must be selected — the
build fails with an `#error` otherwise, since an app that behaves correctly
proves nothing here.

## Building and running

Link this tree as the NuttX custom apps directory (instead of the sample):

```bash
ln -s /path/to/esp-bist/tests/integration/hd_nuttx /path/to/apps/external
```

Then, from the NuttX OS tree, build one fault config at a time. Base board:
**`esp32c6-devkitc:ulp`**. Extra options: `hd_nuttx_test/configs/espressif`
plus either `hd_key_mismatch` or `hd_starved_agent`.

```bash
cd /path/to/nuttx
cmake -B build-hd_key_mismatch -DBOARD_CONFIG=esp32c6-devkitc:ulp -GNinja
kconfig-merge -m -O build-hd_key_mismatch build-hd_key_mismatch/.config \
    /path/to/esp-bist/tests/integration/hd_nuttx/hd_nuttx_test/configs/espressif \
    /path/to/esp-bist/tests/integration/hd_nuttx/hd_nuttx_test/configs/hd_key_mismatch
cmake --build build-hd_key_mismatch -t olddefconfig
kconfig-merge -m -O build-hd_key_mismatch build-hd_key_mismatch/.config \
    /path/to/esp-bist/tests/integration/hd_nuttx/hd_nuttx_test/configs/espressif \
    /path/to/esp-bist/tests/integration/hd_nuttx/hd_nuttx_test/configs/hd_key_mismatch
cmake --build build-hd_key_mismatch -j$(nproc)
ESPTOOL_PORT=<PORT_NAME> cmake --build build-hd_key_mismatch -t flash
```

Repeat with `build-hd_starved_agent` and `configs/hd_starved_agent`.

## Automated device test

After flashing one image:

```bash
cd /path/to/esp-bist/tests/integration/hd_nuttx
pytest pytest_device* --port=<PORT_NAME> --baud=115200 -k hd_key_mismatch
```

Expected UART sequence, ending in the reset:

```
nsh> hd_nuttx_test
test_HD_agent_ready:PASS
test_BIST_postboot:PASS
test_HD_fail_closed_armed:PASS
...
rst:0x10 (RTCWDT_RTC_RESET),boot:0xc (SPI_FAST_FLASH_BOOT)
```

`test_HD_fail_closed_armed` matters: the companion reports one runtime
`LP_STATUS` before its first challenge, so this marker confirms supervision was
live *before* the fault landed. Without it, a reset from an unrelated boot
failure would look like a pass.

NuttX does not print the ESP-IDF bootloader string `CPU has been reset by WDT`.
The pytest matches the ROM reset-reason line (a `rst:0x… (…WDT…)` record) that
the chip emits after an LP WDT system reset.

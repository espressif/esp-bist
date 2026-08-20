# Host Diagnostics fail-closed validation (ESP-IDF)

Proves that a misbehaving HP host is caught by the LP companion and that the
chip resets. This is a test app, not an example — start from
[`samples/idf`](../../../samples/idf) if you want to see how Host Diagnostics is
integrated.

## What makes it fail

The ESP-BIST library is built exactly as a product would build it. There are no
test hooks in `src/bist/`; the fault comes from this app.

| Config | Fault |
|--------|-------|
| `sdkconfig.ci.hd_key_mismatch` | `main/CMakeLists.txt` compiles the HP-side library with `BIST_HD_CHALLENGE_KEY=0x5A5A5A5A`. The ULP sub-project keeps `CONFIG_ESP_BIST_HD_CHALLENGE_KEY`, so the production answer path computes a value the companion rejects on every round. |
| `sdkconfig.ci.hd_starved_agent` | The agent is demoted to priority 23 (below the product default of 24) so that `app_main()`, which raises itself to `configMAX_PRIORITIES - 1` (24), strictly outranks it and busy-waits for 500 ms. The agent is never scheduled and no `ANSWER` reaches the companion before the window closes. The agent task is unpinned, so on a multi-core target every other HP core is held by an equally high-priority task for the same 500 ms. |

Both reach the same end state: the companion stops feeding the LP watchdog and
the watchdog resets the chip. One of the two options must be selected — the
build fails with an `#error` otherwise, since an app that behaves correctly
proves nothing here.

## Building and running

The ULP linker patch is the same one the sample needs:

```bash
cd $IDF_PATH && git apply <esp-bist>/samples/idf/patches/idf_ulp_linker.patch
```

Then, from this directory:

```bash
idf.py -B build_hd_key_mismatch \
    -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.ci.hd_key_mismatch" \
    set-target esp32c6
idf.py -B build_hd_key_mismatch build

pytest pytest_device_hd_idf.py --target=esp32c6
```

Expected UART sequence, ending in the reset:

```
test_HD_agent_ready:PASS
test_BIST_postboot:PASS
test_HD_fail_closed_armed:PASS
...
Guru Meditation Error: Core 0 panic'ed (...)   # CPU has been reset by WDT
```

`test_HD_fail_closed_armed` matters: the companion reports one runtime
`LP_STATUS` before its first challenge, so this marker confirms supervision was
live *before* the fault landed. Without it, a reset from an unrelated boot
failure would look like a pass.

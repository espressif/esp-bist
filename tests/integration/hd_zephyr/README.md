# Host Diagnostics validation (Zephyr)

Twister test app for the LP companion's supervision of a Zephyr HP host. This is
where the assertions live — start from
[`samples/zephyr`](../../../samples/zephyr) if you want to see how Host
Supervision is integrated.

Like the sample, it is a sysbuild project: the HP image under `src/` is the
default image and the LP companion under `remote/` is added by `sysbuild.cmake`.
The LP image is identical to the sample's, because the companion is the element
under test and must be built exactly as a product would build it.

## Scenarios

| Twister test | Harness | What it establishes |
|---|---|---|
| `bist.hd.selftest` | ztest | A well-behaved host passes: post-boot and runtime bitmasks are complete and the companion keeps issuing challenges for every round |
| `bist.hd.reset_cause` | ztest, `expect_reboot` | Starving the agent resets the host, and `hwinfo` names the cause as a watchdog rather than a panic |
| `bist.hd.key_mismatch` | console | A host built with `CONFIG_ESP_BIST_HD_CHALLENGE_KEY=0x5A5A5A5A` (HP image only) answers wrongly every round and is reset |
| `bist.hd.starved_agent` | console | Same starvation as `reset_cause`, observed black-box with no help from the firmware under test |

There are no test hooks in `src/bist/`; a fail-closed build and a product build
differ only in this app's code and Kconfig.

`bist.hd.reset_cause` uses a `__noinit` boot marker to tell the armed boot from
the post-reset boot, and clears it on any non-watchdog reset so a stale marker
cannot fake a pass. That is also why it cannot use the key mismatch: a
build-time key is wrong on every boot, so there would be no "armed once" state
to observe.

## Running

```bash
west twister -p esp32c6_devkitc/esp32c6/hpcore \
    -T tests/integration/hd_zephyr \
    --device-testing --device-serial /dev/ttyUSB0 --west-runner esp32
```

## Related coverage

- [`tests/unit/hd_companion`](../../unit/hd_companion) — the full companion
  verdict matrix (wrong value, replayed sequence, wrong frame type, silence,
  late answer) off-target, no hardware needed.
- [`tests/integration/hd_idf`](../hd_idf) — the same two faults on ESP-IDF.

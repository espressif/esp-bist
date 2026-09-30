"""Flash the metrics app, capture METRICS_DONE, and write the docs JSON.

This is not a qualification suite. scripts/run_device_tests.py skips it.
``bist_ram_test_abraham_full`` runs last. It is quadratic in the
partition count, so its expect timeout is much longer than the others.
"""

from __future__ import annotations

import json
import logging
import re
import subprocess
from pathlib import Path

import pytest

from tests.idf_targets import pytestmark  # noqa: F401

_LOG = logging.getLogger(__name__)

_FAST_S = 60
_SLOW_S = 180

# Returns a byte count, not BIST_ESP_OK.
_NON_STATUS = frozenset({"bist_get_stack_high_watermark"})
# Both need a 32 kHz crystal (bist_clock_fail.h). A runner without one
# returns BIST_ESP_CLOCK_TEST_ERR. C6 skips the external test (no XT WDT).
# C3 runs it and the XT WDT fires. That timing is the failure path, so the
# row stays "not measured" and does not fail the capture.
_OPTIONAL = frozenset(
    {
        "bist_ext_crystal_fail_test",
        "bist_main_crystal_test",
    }
)

_FUNCTIONS = (
    ("bist_cpu_regs_test", _FAST_S),
    ("bist_cpu_csr_regs_test", _FAST_S),
    ("bist_cpu_stack_overflow_init", _FAST_S),
    ("bist_cpu_stack_overflow_check", _FAST_S),
    ("bist_cpu_stack_overflow_test", _FAST_S),
    ("bist_get_stack_high_watermark", _FAST_S),
    ("bist_pc_test", _FAST_S),
    ("bist_ram_test_march_a", _SLOW_S),
    ("bist_ram_test_march_x", _SLOW_S),
    ("bist_ram_test_abraham", _FAST_S),
    ("bist_flash_test", _SLOW_S),
    ("bist_interrupt_source_map_test", _FAST_S),
    ("bist_hardware_interrupt_test", _SLOW_S),
    ("bist_ext_crystal_fail_test", _FAST_S),
    ("bist_main_crystal_test", _SLOW_S),
    ("bist_gpio_output_test", _FAST_S),
    ("bist_gpio_input_test", _FAST_S),
)

_ADC = (
    ("bist_adc_low_level_test", _FAST_S),
    ("bist_adc_high_level_test", _FAST_S),
)
_ADC_REFERENCE = (("bist_adc_reference_test", _FAST_S),)
_WDT = (("wdt_init_windowed", _FAST_S),)
_ABRAHAM_FULL = (("bist_ram_test_abraham_full", 7200),)


def expected_functions(target: str) -> tuple[tuple[str, int], ...]:
    """Functions the firmware prints for this SoC, in capture order."""
    names = list(_FUNCTIONS)
    if target != "esp32h4":
        names.extend(_ADC)
    if target == "esp32c3":
        names.extend(_ADC_REFERENCE)
    names.extend(_WDT)
    names.extend(_ABRAHAM_FULL)
    return tuple(names)


def exec_time_ms(cycles: int, cpu_mhz: int) -> float:
    """cycles / (freq_MHz * 1000)."""
    return cycles / (cpu_mhz * 1000)


def code_sizes(elf: Path) -> dict[str, int]:
    """Symbol sizes for bist_* and wdt_init_windowed from ``nm --print-size``."""
    nm = "riscv32-esp-elf-nm"
    proc = subprocess.run(
        [nm, "--print-size", str(elf)],
        check=True,
        capture_output=True,
        text=True,
    )
    sizes: dict[str, int] = {}
    for line in proc.stdout.splitlines():
        parts = line.split()
        if len(parts) < 4:
            continue
        _addr, size, _type, name = parts[0], parts[1], parts[2], parts[3]
        if name.startswith("bist_") or name == "wdt_init_windowed":
            sizes[name] = int(size, 16)
    return sizes


def _as_text(value) -> str:
    if isinstance(value, bytes):
        return value.decode()
    return str(value)


def _capture_one(dut, name: str, timeout: int) -> tuple[int, int, int, int]:
    cycles_match = dut.expect(
        rf"METRICS: {re.escape(name)} Mode: 0 delta=(\d+)",
        timeout=timeout,
    )
    ret_cycle = dut.expect(
        rf"RESULT {re.escape(name)} mode=0 ret=(-?\d+)",
        timeout=30,
    )
    inst_match = dut.expect(
        rf"METRICS: {re.escape(name)} Mode: 1 delta=(\d+)",
        timeout=timeout,
    )
    ret_inst = dut.expect(
        rf"RESULT {re.escape(name)} mode=1 ret=(-?\d+)",
        timeout=30,
    )
    return (
        int(_as_text(cycles_match.group(1))),
        int(_as_text(ret_cycle.group(1))),
        int(_as_text(inst_match.group(1))),
        int(_as_text(ret_inst.group(1))),
    )


def build_record(
    cycles: int,
    instructions: int,
    cpu_mhz: int,
    size: int | None,
) -> dict:
    record = {
        "cycles": cycles,
        "instructions": instructions,
        "exec_time_ms": exec_time_ms(cycles, cpu_mhz),
    }
    if size is not None:
        record["code_size"] = size
    return record


def test_metrics_capture(dut, target):
    out_dir = Path("build/tests")
    out_dir.mkdir(parents=True, exist_ok=True)
    json_path = out_dir / f"{target}_metrics.json"
    log_path = out_dir / f"{target}_metrics.log"

    class _SerialLog:
        def __init__(self, path: Path):
            self._file = path.open("w", encoding="utf-8")

        def write(self, data):
            if isinstance(data, bytes):
                data = data.decode("utf-8", errors="replace")
            self._file.write(data)
            self._file.flush()

        def flush(self):
            self._file.flush()

        def close(self):
            self._file.close()

    log_file = _SerialLog(log_path)
    proc = getattr(dut, "pexpect_proc", None)
    if proc is not None:
        proc.logfile_read = log_file

    report = {
        "soc": target,
        "cpu_mhz": None,
        "ram_partition_words": None,
        "functions": {},
    }
    failures: list[str] = []
    try:
        banner = dut.expect(
            r"METRICS_BANNER soc=(\S+) cpu_mhz=(\d+) ram_partition_words=(\d+)",
            timeout=120,
        )
        report["soc"] = _as_text(banner.group(1))
        report["cpu_mhz"] = int(_as_text(banner.group(2)))
        report["ram_partition_words"] = int(_as_text(banner.group(3)))
        if report["soc"] != target:
            failures.append(f"banner soc {report['soc']} != {target}")

        elf = Path("build/metrics.elf")
        sizes = code_sizes(elf) if elf.is_file() else {}

        for name, timeout in expected_functions(target):
            cycles, ret_cycle, instructions, ret_inst = _capture_one(dut, name, timeout)
            if name not in _NON_STATUS and (ret_cycle != 0 or ret_inst != 0):
                if name in _OPTIONAL:
                    _LOG.warning(
                        "%s not measured: ret cycle=%s inst=%s",
                        name,
                        ret_cycle,
                        ret_inst,
                    )
                else:
                    failures.append(f"{name} ret cycle={ret_cycle} inst={ret_inst}")
                continue
            report["functions"][name] = build_record(
                cycles,
                instructions,
                report["cpu_mhz"],
                sizes.get(name),
            )

        dut.expect(rf"METRICS_DONE soc={re.escape(report['soc'])}", timeout=30)
    finally:
        json_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        log_file.close()

    if failures:
        pytest.fail("; ".join(failures))

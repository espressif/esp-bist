#!/usr/bin/env python3
"""Run every BIST device or QEMU test for one SoC, then print a pass/fail summary.

Mirrors esp-nuttx-ci's HardwareTestLoop / CIQemuLoop:

- One chip per invocation.
- One pytest.main() per test app so a failure does not abort the rest.
- Continue after failures; retry once when CI=true.
- Print a scoreboard at the end and exit 1 if anything failed.

Usage:
  ./scripts/run_device_tests.py --target esp32c3
  ./scripts/run_device_tests.py --target esp32c3 --skip-flash
  ESPPORT=/dev/ttyUSB0 ./scripts/run_device_tests.py --target esp32c6
  ./scripts/run_device_tests.py --target esp32c6 --qemu

Requires a built tree at tests/<app>/build (from build_all_tests.sh or CI artifacts).
QEMU mode needs qemu-system-riscv32 on PATH and tests/<app>/build/<app>_qemu_image.bin.
"""

from __future__ import annotations

import argparse
import logging
import os
import subprocess
import sys
import time
from contextlib import contextmanager
from pathlib import Path

import pytest

logger = logging.getLogger(__name__)

ROOT = Path(__file__).resolve().parent.parent
TESTS_DIR = ROOT / "tests"

MODE_DEVICE = "device"
MODE_QEMU = "qemu"


def discover_test_apps(mode: str) -> tuple[str, ...]:
    """tests/<name>_test directories that have pytest_<mode>*.py."""
    pattern = f"pytest_{mode}*.py"
    apps = []
    for path in sorted(TESTS_DIR.glob("*_test")):
        if path.is_dir() and next(path.glob(pattern), None):
            apps.append(path.name)
    return tuple(apps)


@contextmanager
def pushd(path: Path):
    prev = Path.cwd()
    os.chdir(path)
    try:
        yield
    finally:
        os.chdir(prev)


class AppTestExecutor:
    """Run pytest for one test app. Device mode also flashes MCUboot."""

    def __init__(self, target: str, app: str, mode: str, skip_flash: bool = False):
        self.target = target
        self.app = app
        self.mode = mode
        self.skip_flash = skip_flash
        self.app_dir = TESTS_DIR / app
        self.build_dir = self.app_dir / "build"
        self.pytest_file = next(self.app_dir.glob(f"pytest_{mode}*.py"), None)

    def execute(self) -> int:
        if self.pytest_file is None:
            logger.error("No pytest_%s*.py in %s", self.mode, self.app_dir)
            return 1

        with pushd(self.app_dir):
            if not Path("build").is_dir():
                logger.error(
                    "Missing build dir %s — build this app first", self.build_dir
                )
                return 1

            if self.mode == MODE_DEVICE and not self.skip_flash:
                flash_ret = self._flash_boot()
                if flash_ret != 0:
                    return flash_ret

            return self._run_pytest()

    def _flash_boot(self) -> int:
        logger.info("Flashing MCUboot for %s (%s)", self.app, self.target)
        cmake_ret = subprocess.run(
            [
                "cmake",
                f"-DSOC_TARGET={self.target}",
                "-B",
                "build",
                "-GNinja",
            ]
        ).returncode
        if cmake_ret != 0:
            logger.error("cmake failed for %s", self.app)
            return cmake_ret

        ret = subprocess.run(["ninja", "-C", "build", "flash_boot"]).returncode
        if ret != 0:
            logger.error("flash_boot failed for %s", self.app)
        return ret

    def _run_pytest(self) -> int:
        Path("build/tests").mkdir(parents=True, exist_ok=True)
        report = f"build/tests/{self.target}_{self.mode}_report.xml"
        args = [
            self.pytest_file.name,
            "--executable",
            self.app,
            "--target",
            self.target,
            "--junitxml",
            report,
            "--reruns",
            "2",
        ]
        logger.info("pytest %s", " ".join(args))
        ret = pytest.main(args)
        sys.stdout.flush()
        return int(ret)


class AppTestLoop:
    """Walk every matching test app on one chip; do not stop on the first failure."""

    def __init__(self, target: str, mode: str, skip_flash: bool = False):
        self.target = target
        self.mode = mode
        self.skip_flash = skip_flash
        self.apps = discover_test_apps(mode)
        self.in_ci = bool(os.getenv("CI") or os.getenv("GITLAB_CI"))
        self.failed: list[str] = []
        self.passed: list[str] = []
        self.run_time = 0.0

    def test_loop(self) -> None:
        if not self.apps:
            raise RuntimeError(
                f"No tests/*_test apps with pytest_{self.mode}*.py under {TESTS_DIR}"
            )
        logger.info("%s test apps: %s", self.mode.capitalize(), ", ".join(self.apps))
        start = time.time()
        total = len(self.apps)
        for count, app in enumerate(self.apps, start=1):
            logger.info("=" * 80)
            logger.info(
                "%s testing %s/%s: %s (%s)",
                self.mode.capitalize(),
                count,
                total,
                app,
                self.target,
            )
            logger.info("=" * 80)

            executor = AppTestExecutor(self.target, app, self.mode, self.skip_flash)
            ret = executor.execute()

            if ret != 0 and self.in_ci:
                logger.error("1st try failed for %s:%s, retrying...", self.target, app)
                ret = executor.execute()

            if ret != 0:
                self.failed.append(app)
                logger.error(
                    "%s test failed for %s:%s",
                    self.mode.capitalize(),
                    self.target,
                    app,
                )
            else:
                self.passed.append(app)
                logger.info(
                    "%s test passed for %s:%s",
                    self.mode.capitalize(),
                    self.target,
                    app,
                )

        self.run_time = time.time() - start

    def results_summary(self) -> None:
        logger.info("=" * 80)
        logger.info(
            "%s test summary (%s): %.1fs (%.1f min)",
            self.mode.capitalize(),
            self.target,
            self.run_time,
            self.run_time / 60,
        )
        logger.info("Passed (%s):", len(self.passed))
        for app in self.passed:
            logger.info("  PASS  %s", app)
        if self.failed:
            logger.info("Failed (%s):", len(self.failed))
            for app in self.failed:
                logger.info("  FAIL  %s", app)
        else:
            logger.info("All %s tests passed.", self.mode)
        logger.info("=" * 80)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run every BIST device or QEMU test for one SoC and print a summary."
    )
    parser.add_argument("--target", required=True, help="SoC, e.g. esp32c3")
    parser.add_argument(
        "--qemu",
        action="store_true",
        help="Run pytest_qemu* tests instead of pytest_device* (no DUT flash).",
    )
    parser.add_argument(
        "--skip-flash",
        action="store_true",
        help="Skip ninja flash_boot (device mode only).",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(levelname)s %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
    )

    mode = MODE_QEMU if args.qemu else MODE_DEVICE
    if mode == MODE_QEMU and args.skip_flash:
        logger.info("--skip-flash is ignored in QEMU mode")

    loop = AppTestLoop(args.target, mode, args.skip_flash)
    loop.test_loop()
    loop.results_summary()
    return 1 if loop.failed else 0


if __name__ == "__main__":
    sys.exit(main())

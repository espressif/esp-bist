#!/usr/bin/env python3
"""Fail if a published ESP-BIST manual page mentions an OS-integration feature.

The published set is the toctree in docs/en/index.rst. ESP-IDF citations that
name the standalone toolchain or HAL are allowed. OS product modes are not.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
INDEX = REPO / "docs" / "en" / "index.rst"

# Terms that mean an OS-integration feature, not the standalone toolchain.
FORBIDDEN = (
    re.compile(r"Zephyr", re.IGNORECASE),
    re.compile(r"FreeRTOS"),
    re.compile(r"NuttX", re.IGNORECASE),
    re.compile(r"Host Diagnostics"),
    re.compile(r"samples/idf"),
    re.compile(r"samples/zephyr"),
    re.compile(r"samples/nuttx"),
    re.compile(r"hd_protocol"),
    re.compile(r"hd_companion"),
    re.compile(r"hd_agent"),
    re.compile(r"LP-companion"),
    re.compile(r"LP companion"),
)

# A line that only cites the standalone toolchain or HAL may contain "ESP-IDF".
# The forbidden patterns above do not include that phrase. This allowlist is
# the record of the citations the release keeps on purpose.
ALLOWLIST_FILES = {
    "docs/en/tool_qualification.rst",
    "docs/en/software_safety_requirements.rst",
}


def toctree_pages(index: Path) -> list[Path]:
    text = index.read_text(encoding="utf-8")
    match = re.search(r"\.\. toctree::(.*?)(?:\n\S|\Z)", text, re.DOTALL)
    if match is None:
        raise SystemExit(f"no toctree in {index}")
    pages = []
    for line in match.group(1).splitlines():
        line = line.strip()
        if not line or line.startswith(":"):
            continue
        name = line.split("<")[-1].rstrip(">").strip()
        pages.append(index.parent / f"{name}.rst")
    return pages


def main() -> int:
    failures = []
    pages = toctree_pages(INDEX)
    if not pages:
        print("toctree is empty", file=sys.stderr)
        return 1
    for page in pages:
        rel = page.relative_to(REPO).as_posix()
        if not page.is_file():
            failures.append(f"{rel}: missing")
            continue
        for lineno, line in enumerate(page.read_text(encoding="utf-8").splitlines(), 1):
            for pattern in FORBIDDEN:
                if pattern.search(line):
                    failures.append(
                        f"{rel}:{lineno}: {pattern.pattern}: {line.strip()}"
                    )
    if failures:
        print("official manual mentions OS integration:", file=sys.stderr)
        for item in failures:
            print(item, file=sys.stderr)
        return 1
    allowed = ", ".join(sorted(ALLOWLIST_FILES))
    print(f"official manual: {len(pages)} pages, no OS-integration terms")
    print(f"ESP-IDF toolchain and HAL citations may remain in: {allowed}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

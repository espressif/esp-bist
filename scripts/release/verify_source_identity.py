#!/usr/bin/env python3
"""Recompute MD5 digests in the Module Design source-file tables.

Fails when a published digest does not match the file bytes.
"""

from __future__ import annotations

import hashlib
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
MODULE = REPO / "docs" / "en" / "module_design_and_coding.rst"
ROW = re.compile(
    r"^\s+\* - ``(?P<path>src/[^`]+)``\s*$\n"
    r"\s+- (?P<version>\S+)\s*$\n"
    r"\s+- (?P<digest>[0-9a-fA-F]{32}|N/A)\s*$",
    re.MULTILINE,
)


def main() -> int:
    text = MODULE.read_text(encoding="utf-8")
    rows = list(ROW.finditer(text))
    if not rows:
        print(f"no source-file rows in {MODULE}", file=sys.stderr)
        return 1
    failures = []
    for row in rows:
        rel = row.group("path")
        published = row.group("digest").lower()
        path = REPO / rel
        if not path.is_file():
            failures.append(f"{rel}: file missing")
            continue
        actual = hashlib.md5(path.read_bytes()).hexdigest()
        if published == "n/a":
            failures.append(f"{rel}: digest is N/A, file is {actual}")
            continue
        if published != actual:
            failures.append(f"{rel}: published {published}, file {actual}")
    if failures:
        print("source identity mismatch:", file=sys.stderr)
        for item in failures:
            print(item, file=sys.stderr)
        return 1
    print(f"source identity: {len(rows)} digests match")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

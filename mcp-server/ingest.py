#!/usr/bin/env python3
"""Offline ingestion script for ESP-BIST MCP server.

Reads ESP-BIST repo content (RST docs, C headers, Kconfig, source code, READMEs),
parses and chunks it, and writes data/*.json files to be shipped with the server.

Usage:
    python ingest.py /path/to/esp-bist
"""

import json
import os
import re
import sys
from pathlib import Path


def chunk_rst(filepath: Path) -> list[dict]:
    """Parse an RST file and split it into chunks by section headings."""
    text = filepath.read_text(encoding="utf-8", errors="replace")
    filename = filepath.name

    section_re = re.compile(
        r"^(.+)\n([=\-\^~\"]+)$", re.MULTILINE
    )

    sections: list[dict] = []
    matches = list(section_re.finditer(text))

    if not matches:
        return [
            {
                "id": f"doc:{filename}:full",
                "title": filename.replace(".rst", "").replace("_", " ").title(),
                "section": "",
                "body": text.strip(),
                "source_file": str(filepath),
                "content_type": "doc",
            }
        ]

    if matches[0].start() > 0:
        preamble = text[: matches[0].start()].strip()
        if preamble:
            sections.append(
                {
                    "id": f"doc:{filename}:preamble",
                    "title": filename.replace(".rst", "").replace("_", " ").title(),
                    "section": "preamble",
                    "body": preamble,
                    "source_file": str(filepath),
                    "content_type": "doc",
                }
            )

    for i, match in enumerate(matches):
        title = match.group(1).strip()
        start = match.end()
        end = matches[i + 1].start() if i + 1 < len(matches) else len(text)
        body = text[start:end].strip()
        if not body:
            continue

        section_id = re.sub(r"[^a-z0-9]+", "_", title.lower()).strip("_")
        sections.append(
            {
                "id": f"doc:{filename}:{section_id}",
                "title": title,
                "section": section_id,
                "body": body,
                "source_file": str(filepath),
                "content_type": "doc",
            }
        )

    return sections


def chunk_markdown(filepath: Path) -> list[dict]:
    """Parse a Markdown file and split by headings."""
    text = filepath.read_text(encoding="utf-8", errors="replace")
    filename = filepath.name

    heading_re = re.compile(r"^(#{1,4})\s+(.+)$", re.MULTILINE)
    matches = list(heading_re.finditer(text))

    if not matches:
        return [
            {
                "id": f"doc:{filename}:full",
                "title": filename,
                "section": "",
                "body": text.strip(),
                "source_file": str(filepath),
                "content_type": "doc",
            }
        ]

    sections: list[dict] = []

    if matches[0].start() > 0:
        preamble = text[: matches[0].start()].strip()
        if preamble:
            sections.append(
                {
                    "id": f"doc:{filename}:preamble",
                    "title": filename,
                    "section": "preamble",
                    "body": preamble,
                    "source_file": str(filepath),
                    "content_type": "doc",
                }
            )

    for i, match in enumerate(matches):
        title = match.group(2).strip()
        start = match.end()
        end = matches[i + 1].start() if i + 1 < len(matches) else len(text)
        body = text[start:end].strip()
        if not body:
            continue

        section_id = re.sub(r"[^a-z0-9]+", "_", title.lower()).strip("_")
        sections.append(
            {
                "id": f"doc:{filename}:{section_id}",
                "title": title,
                "section": section_id,
                "body": body,
                "source_file": str(filepath),
                "content_type": "doc",
            }
        )

    return sections


def parse_c_header(filepath: Path) -> list[dict]:
    """Extract function signatures and Doxygen comments from a C header."""
    text = filepath.read_text(encoding="utf-8", errors="replace")
    filename = filepath.name

    module = "unknown"
    if "core/cpu" in str(filepath):
        module = "cpu"
    elif "core/interrupt" in str(filepath):
        module = "interrupt"
    elif "core/memory" in str(filepath):
        module = "memory"
    elif "core/clock" in str(filepath):
        module = "clock"
    elif "core/wdt" in str(filepath):
        module = "wdt"
    elif "core/io" in str(filepath):
        module = "io"
    elif "drivers" in str(filepath):
        module = "drivers"
    elif "include/bist_esp" in str(filepath) or "include/bist_metrics" in str(filepath):
        module = "core"

    file_brief = ""
    file_brief_match = re.search(r"@file\s+\S+\s*\n\s*\*\s*@brief\s+(.+?)(?:\n\s*\*\s*\n|\n\s*\*/)", text, re.DOTALL)
    if file_brief_match:
        file_brief = re.sub(r"\n\s*\*\s*", " ", file_brief_match.group(1)).strip()

    func_pattern = re.compile(
        r"(/\*\*.*?\*/)\s*\n\s*"
        r"((?:(?:static\s+|inline\s+|extern\s+|__attribute__\s*\([^)]*\)\s+)*"
        r"(?:(?:unsigned\s+|signed\s+|const\s+|volatile\s+|struct\s+|enum\s+)*"
        r"[a-zA-Z_]\w*(?:\s*\*)*)\s+"
        r"([a-zA-Z_]\w*)\s*\((?:[^()]*|\([^()]*\))*\)))\s*;",
        re.DOTALL,
    )

    entries = []
    for match in func_pattern.finditer(text):
        doxygen_block = match.group(1)
        signature = match.group(2).strip()
        func_name = match.group(3)

        brief = ""
        brief_match = re.search(r"@brief\s+(.+?)(?:\n\s*\*\s*\n|\n\s*\*\s*@|\n\s*\*/)", doxygen_block, re.DOTALL)
        if brief_match:
            brief = re.sub(r"\n\s*\*\s*", " ", brief_match.group(1)).strip()

        params = []
        for pm in re.finditer(r"@param\s+(\w+)\s+(.+?)(?=\n\s*\*\s*@|\n\s*\*/)", doxygen_block, re.DOTALL):
            param_desc = re.sub(r"\n\s*\*\s*", " ", pm.group(2)).strip()
            params.append({"name": pm.group(1), "description": param_desc})

        returns = ""
        return_matches = re.findall(r"@return\s+(.+?)(?=\n\s*\*\s*@|\n\s*\*/|\n\s*\*\s*\n)", doxygen_block, re.DOTALL)
        if return_matches:
            returns = "; ".join(
                re.sub(r"\n\s*\*\s*", " ", r).strip() for r in return_matches
            )

        note = ""
        note_match = re.search(r"@note\s+(.+?)(?=\n\s*\*\s*@|\n\s*\*/)", doxygen_block, re.DOTALL)
        if note_match:
            note = re.sub(r"\n\s*\*\s*", " ", note_match.group(1)).strip()

        kconfig_guards = re.findall(r"CONFIG_\w+", doxygen_block + signature)

        entries.append(
            {
                "function": func_name,
                "signature": signature,
                "module": module,
                "description": brief,
                "params": params,
                "returns": returns,
                "note": note,
                "header_file": str(filepath),
                "kconfig_guards": sorted(set(kconfig_guards)),
            }
        )

    if not entries and file_brief:
        entries.append(
            {
                "function": filename.replace(".h", ""),
                "signature": "",
                "module": module,
                "description": file_brief,
                "params": [],
                "returns": "",
                "note": "",
                "header_file": str(filepath),
                "kconfig_guards": [],
            }
        )

    return entries


def parse_enums_and_typedefs(filepath: Path) -> list[dict]:
    """Extract enums and typedefs from a C header for API reference."""
    text = filepath.read_text(encoding="utf-8", errors="replace")
    filename = filepath.name

    entries = []

    enum_re = re.compile(
        r"(/\*\*.*?\*/)?\s*typedef\s+enum\s*\{([^}]*)\}\s*(\w+)\s*;",
        re.DOTALL,
    )

    for match in enum_re.finditer(text):
        doxygen = match.group(1) or ""
        body = match.group(2)
        name = match.group(3)

        brief = ""
        brief_match = re.search(r"@brief\s+(.+?)(?:\n\s*\*\s*\n|\n\s*\*\s*@|\n\s*\*/)", doxygen, re.DOTALL)
        if brief_match:
            brief = re.sub(r"\n\s*\*\s*", " ", brief_match.group(1)).strip()

        values = []
        for vm in re.finditer(
            r"(\w+)"
            r"(?:\s*=\s*((?:0[xX][0-9a-fA-F]+|\d+)(?:\s*(?:<<|>>|\||\&)\s*(?:0[xX][0-9a-fA-F]+|\d+|\w+))*|\([^)]+\)|\w+))?"
            r"\s*,?\s*(?:/\*\*<\s*(.+?)\s*\*/|//[!<]*\s*(.+))?",
            body,
        ):
            val_str = (vm.group(2) or "").strip()
            try:
                value = int(val_str, 0)
            except (ValueError, TypeError):
                value = val_str
            values.append(
                {
                    "name": vm.group(1),
                    "value": value,
                    "description": vm.group(3) or vm.group(4) or "",
                }
            )

        entries.append(
            {
                "function": name,
                "signature": f"typedef enum {{ ... }} {name}",
                "module": "core",
                "description": brief,
                "params": [],
                "returns": "",
                "note": "",
                "header_file": str(filepath),
                "kconfig_guards": [],
                "enum_values": values,
            }
        )

    return entries


def parse_kconfig(filepath: Path) -> list[dict]:
    """Parse a Kconfig file into structured entries."""
    text = filepath.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()

    entries = []
    current: dict | None = None
    in_help = False
    help_lines: list[str] = []
    current_menu = ""

    for line in lines:
        stripped = line.strip()

        menu_match = re.match(r'^menu\s+"(.+)"', stripped)
        if menu_match:
            current_menu = menu_match.group(1)
            continue

        if stripped == "endmenu":
            current_menu = ""
            continue

        config_match = re.match(r"^config\s+(\w+)", stripped)
        if config_match:
            if current and in_help:
                current["help"] = "\n".join(help_lines).strip()
            if current:
                entries.append(current)

            current = {
                "name": config_match.group(1),
                "type": "",
                "default": "",
                "depends_on": "",
                "help": "",
                "menu": current_menu,
                "range": "",
                "prompt": "",
            }
            in_help = False
            help_lines = []
            continue

        if current is None:
            continue

        if in_help:
            if line.startswith("    ") or line.startswith("\t") or stripped == "":
                help_lines.append(stripped)
                continue
            else:
                current["help"] = "\n".join(help_lines).strip()
                in_help = False

        if stripped.startswith("bool ") or stripped.startswith("bool\t"):
            current["type"] = "bool"
            prompt_match = re.match(r'bool\s+"(.+)"', stripped)
            if prompt_match:
                current["prompt"] = prompt_match.group(1)
        elif stripped.startswith("int ") or stripped.startswith("int\t"):
            current["type"] = "int"
            prompt_match = re.match(r'int\s+"(.+)"', stripped)
            if prompt_match:
                current["prompt"] = prompt_match.group(1)
        elif stripped.startswith("hex ") or stripped.startswith("hex\t"):
            current["type"] = "hex"
            prompt_match = re.match(r'hex\s+"(.+)"', stripped)
            if prompt_match:
                current["prompt"] = prompt_match.group(1)
        elif stripped.startswith("default "):
            current["default"] = stripped.split("default ", 1)[1].strip()
        elif stripped.startswith("depends on "):
            current["depends_on"] = stripped.split("depends on ", 1)[1].strip()
        elif stripped.startswith("range "):
            current["range"] = stripped.split("range ", 1)[1].strip()
        elif stripped == "help":
            in_help = True
            help_lines = []

    if current:
        if in_help:
            current["help"] = "\n".join(help_lines).strip()
        entries.append(current)

    return entries


def chunk_source_file(filepath: Path) -> list[dict]:
    """Chunk a C source file by function definitions."""
    text = filepath.read_text(encoding="utf-8", errors="replace")
    filename = filepath.name
    lines = text.splitlines()

    module = "unknown"
    if "core/cpu" in str(filepath):
        module = "cpu"
    elif "core/interrupt" in str(filepath):
        module = "interrupt"
    elif "core/memory" in str(filepath):
        module = "memory"
    elif "core/clock" in str(filepath):
        module = "clock"
    elif "core/wdt" in str(filepath):
        module = "wdt"
    elif "core/io" in str(filepath):
        module = "io"
    elif "drivers" in str(filepath):
        module = "drivers"

    func_re = re.compile(
        r"^(?:static\s+|inline\s+|__attribute__\s*\([^)]*\)\s+)*"
        r"(?:(?:unsigned\s+|signed\s+|const\s+|volatile\s+|struct\s+|enum\s+)*"
        r"[a-zA-Z_]\w*(?:\s*\*)*)\s+"
        r"([a-zA-Z_]\w*)\s*\([^)]*\)\s*\{",
        re.MULTILINE,
    )

    chunks = []
    matches = list(func_re.finditer(text))

    for i, match in enumerate(matches):
        func_name = match.group(1)
        start_pos = match.start()
        line_start = text[:start_pos].count("\n") + 1

        brace_count = 0
        end_pos = match.start()
        found_open = False
        for j in range(match.start(), len(text)):
            if text[j] == "{":
                brace_count += 1
                found_open = True
            elif text[j] == "}":
                brace_count -= 1
                if found_open and brace_count == 0:
                    end_pos = j + 1
                    break

        line_end = text[:end_pos].count("\n") + 1
        body = text[start_pos:end_pos]

        if len(body) > 3000:
            body = body[:3000] + "\n    // ... (truncated)"

        chunks.append(
            {
                "function_name": func_name,
                "body": body,
                "file": str(filepath),
                "line_start": line_start,
                "line_end": line_end,
                "module": module,
                "content_type": "source",
            }
        )

    return chunks


def build_socs_data(repo_root: Path) -> list[dict]:
    """Build SoC support matrix from cmake files and directory structure."""
    socs = []
    cmake_dir = repo_root / "cmake"
    soc_dir = repo_root / "src" / "soc"

    soc_info = {
        "esp32c3": {
            "name": "ESP32-C3",
            "cpu": "Single-core RISC-V",
            "max_freq_mhz": 160,
            "sram_kb": "400 + 8 RTC",
            "psram": False,
            "gpio_count": "22 / 16",
            "has_pma": False,
            "has_xt_wdt": True,
            "csr_count": 25,
        },
        "esp32c5": {
            "name": "ESP32-C5",
            "cpu": "Single-core RISC-V + LP RISC-V",
            "max_freq_mhz": 240,
            "sram_kb": "384 HP + 16 LP",
            "psram": True,
            "gpio_count": "29",
            "has_pma": True,
            "has_xt_wdt": False,
            "csr_count": 39,
        },
        "esp32c6": {
            "name": "ESP32-C6",
            "cpu": "Single-core RISC-V + LP RISC-V",
            "max_freq_mhz": 160,
            "sram_kb": "512 HP + 16 LP",
            "psram": False,
            "gpio_count": "30 / 22",
            "has_pma": True,
            "has_xt_wdt": False,
            "csr_count": 37,
        },
        "esp32h2": {
            "name": "ESP32-H2",
            "cpu": "Single-core RISC-V",
            "max_freq_mhz": 96,
            "sram_kb": "320 + 4 RTC",
            "psram": False,
            "gpio_count": "19",
            "has_pma": True,
            "has_xt_wdt": False,
            "csr_count": 37,
        },
        "esp32p4": {
            "name": "ESP32-P4",
            "cpu": "Dual-core RISC-V + LP RISC-V",
            "max_freq_mhz": 400,
            "sram_kb": "768 KB HP L2MEM + 32 KB LP + 8 KB TCM",
            "psram": True,
            "gpio_count": "55",
            "has_pma": True,
            "has_xt_wdt": False,
            "csr_count": 40,
        },
    }

    for soc_key, info in soc_info.items():
        cmake_file = cmake_dir / f"{soc_key}.cmake"
        soc_path = soc_dir / soc_key

        cmake_notes = ""
        if cmake_file.exists():
            cmake_text = cmake_file.read_text(encoding="utf-8", errors="replace")
            cmake_notes = f"CMake config at cmake/{soc_key}.cmake ({len(cmake_text.splitlines())} lines)"

        ld_files = []
        if soc_path.exists():
            for f in sorted(soc_path.rglob("*.ld")):
                ld_files.append(str(f.relative_to(repo_root)))

        entry = {
            "soc": soc_key,
            **info,
            "cmake_file": str(cmake_file.relative_to(repo_root)) if cmake_file.exists() else "",
            "linker_scripts": ld_files,
            "soc_dir": str(soc_path.relative_to(repo_root)) if soc_path.exists() else "",
            "specific_notes": cmake_notes,
        }
        socs.append(entry)

    return socs


_PATH_FIELDS = ("source_file", "header_file", "file", "cmake_file", "soc_dir")


def _relativize(entries: list[dict], repo_root: Path) -> list[dict]:
    """Rewrite absolute path fields so they are relative to ``repo_root``.

    Keeping snapshots path-independent is required so the CI drift check
    (and any two developers on different machines) produce byte-identical
    ``data/*.json`` files. This rewrites every known path-bearing field
    (``source_file``, ``header_file``, ``file``, ``cmake_file``,
    ``soc_dir``) in every entry.
    """
    for entry in entries:
        for field in _PATH_FIELDS:
            value = entry.get(field)
            if not value:
                continue
            path = Path(value)
            if not path.is_absolute():
                entry[field] = str(path)
                continue
            try:
                entry[field] = str(path.resolve().relative_to(repo_root))
            except ValueError:
                entry[field] = str(path)
    return entries


def ingest(repo_root: Path, output_dir: Path):
    """Run the full ingestion pipeline."""
    output_dir.mkdir(parents=True, exist_ok=True)
    repo_root = repo_root.resolve()

    # 1. Documentation chunks (RST + Markdown)
    print("Ingesting documentation...")
    doc_chunks = []

    rst_dir = repo_root / "docs" / "en"
    if rst_dir.exists():
        for rst_file in sorted(rst_dir.glob("*.rst")):
            chunks = chunk_rst(rst_file)
            doc_chunks.extend(chunks)
            print(f"  RST: {rst_file.name} -> {len(chunks)} chunks")

    readme_files = [
        repo_root / "README.md",
        repo_root / "src" / "bist" / "README.md",
        repo_root / "samples" / "zephyr" / "README.md",
    ]
    for md_file in readme_files:
        if md_file.exists():
            chunks = chunk_markdown(md_file)
            doc_chunks.extend(chunks)
            print(f"  MD:  {md_file.name} ({md_file.parent.name}) -> {len(chunks)} chunks")

    doc_chunks = _relativize(doc_chunks, repo_root)
    with open(output_dir / "docs.json", "w") as f:
        json.dump(doc_chunks, f, indent=2)
    print(f"  Total doc chunks: {len(doc_chunks)}")

    # 2. API reference (headers)
    print("\nIngesting API headers...")
    api_entries = []

    header_dirs = [
        repo_root / "src" / "bist" / "include",
        repo_root / "src" / "bist" / "core" / "cpu" / "include",
        repo_root / "src" / "bist" / "core" / "interrupt" / "include",
        repo_root / "src" / "bist" / "core" / "memory" / "include",
        repo_root / "src" / "bist" / "core" / "clock" / "include",
        repo_root / "src" / "bist" / "core" / "wdt" / "include",
        repo_root / "src" / "bist" / "core" / "io" / "include",
        repo_root / "src" / "bist" / "drivers" / "include",
    ]

    for hdir in header_dirs:
        if not hdir.exists():
            continue
        for hfile in sorted(hdir.glob("*.h")):
            funcs = parse_c_header(hfile)
            enums = parse_enums_and_typedefs(hfile)
            api_entries.extend(funcs)
            api_entries.extend(enums)
            print(f"  {hfile.name} -> {len(funcs)} functions, {len(enums)} types")

    api_entries = _relativize(api_entries, repo_root)
    with open(output_dir / "api.json", "w") as f:
        json.dump(api_entries, f, indent=2)
    print(f"  Total API entries: {len(api_entries)}")

    # 3. Kconfig
    print("\nIngesting Kconfig...")
    kconfig_file = repo_root / "src" / "bist" / "Kconfig"
    kconfig_entries = []
    if kconfig_file.exists():
        kconfig_entries = parse_kconfig(kconfig_file)
        print(f"  {kconfig_file.name} -> {len(kconfig_entries)} options")

    kconfig_entries = _relativize(kconfig_entries, repo_root)
    with open(output_dir / "kconfig.json", "w") as f:
        json.dump(kconfig_entries, f, indent=2)

    # 4. Source code chunks
    print("\nIngesting source code...")
    source_chunks = []

    source_dirs = [
        repo_root / "src" / "bist" / "core" / "cpu",
        repo_root / "src" / "bist" / "core" / "interrupt",
        repo_root / "src" / "bist" / "core" / "memory",
        repo_root / "src" / "bist" / "core" / "clock",
        repo_root / "src" / "bist" / "core" / "wdt",
        repo_root / "src" / "bist" / "core" / "io",
        repo_root / "src" / "bist" / "drivers",
    ]

    for sdir in source_dirs:
        if not sdir.exists():
            continue
        for cfile in sorted(sdir.glob("*.c")):
            chunks = chunk_source_file(cfile)
            source_chunks.extend(chunks)
            print(f"  {cfile.name} -> {len(chunks)} functions")

    source_chunks = _relativize(source_chunks, repo_root)
    with open(output_dir / "source.json", "w") as f:
        json.dump(source_chunks, f, indent=2)
    print(f"  Total source chunks: {len(source_chunks)}")

    # 5. SoC data
    print("\nIngesting SoC data...")
    socs = build_socs_data(repo_root)
    socs = _relativize(socs, repo_root)
    with open(output_dir / "socs.json", "w") as f:
        json.dump(socs, f, indent=2)
    print(f"  SoCs: {len(socs)}")

    print(f"\nDone! Data written to {output_dir}/")


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <path-to-esp-bist-repo>")
        sys.exit(1)

    repo_root = Path(sys.argv[1]).resolve()
    if not repo_root.exists():
        print(f"Error: {repo_root} does not exist")
        sys.exit(1)

    script_dir = Path(__file__).parent
    output_dir = script_dir / "data"

    ingest(repo_root, output_dir)


if __name__ == "__main__":
    main()

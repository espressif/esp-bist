"""Sphinx directive that fills a Module API table from a metrics JSON capture.

A missing file, or a function missing from the file, renders ``not measured``.
The JSON is produced by tests/metrics/pytest_device_metrics.py.
"""

from __future__ import annotations

import json
from pathlib import Path

from docutils.parsers.rst import Directive, directives
from sphinx.util import logging

logger = logging.getLogger(__name__)

_NOT_MEASURED = "not measured"


def load_metrics(path: Path) -> dict | None:
    """Return the capture dict, or None when the file is absent or unreadable."""
    if not path.is_file():
        logger.warning("Metrics capture not found: %s", path)
        return None
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        logger.warning("Could not read metrics capture %s: %s", path, exc)
        return None
    if not isinstance(data, dict) or not isinstance(data.get("functions"), dict):
        logger.warning("Metrics capture %s has no functions object", path)
        return None
    return data


def format_exec_time(exec_time_ms) -> str:
    """Present milliseconds, or microseconds when the interval is under 1 ms."""
    if not isinstance(exec_time_ms, (int, float)):
        return _NOT_MEASURED
    microseconds = float(exec_time_ms) * 1000.0
    if microseconds < 1000.0:
        return f"{microseconds:.2f} us"
    return f"{float(exec_time_ms):.2f} ms"


def _cell(value, formatter) -> str:
    if value is None:
        return _NOT_MEASURED
    return formatter(value)


def render_metrics_table(functions: list[str], data: dict | None) -> str:
    """RST list-table. Missing keys stay ``not measured``."""
    rows = data.get("functions", {}) if data else {}
    lines = [
        ".. list-table::",
        "   :header-rows: 1",
        "   :widths: 30 15 15 20 20",
        "",
        "   * - Function name",
        "     - Exec Time",
        "     - Cycles",
        "     - Instruction Count",
        "     - Code size (Bytes)",
    ]
    for name in functions:
        entry = rows.get(name) if isinstance(rows.get(name), dict) else {}
        lines.append(f"   * - ``{name}``")
        lines.append(f"     - {_cell(entry.get('exec_time_ms'), format_exec_time)}")
        lines.append(f"     - {_cell(entry.get('cycles'), lambda n: str(int(n)))}")
        lines.append(
            f"     - {_cell(entry.get('instructions'), lambda n: str(int(n)))}"
        )
        lines.append(f"     - {_cell(entry.get('code_size'), lambda n: str(int(n)))}")
    lines.append("")
    return "\n".join(lines)


class MetricsTableDirective(Directive):
    """
    Include one Module API measurements table.

    Usage:
        .. metrics-table:: tests/metrics/build/tests/{IDF_TARGET_PATH_NAME}_metrics.json
           :functions: bist_cpu_regs_test, bist_cpu_csr_regs_test
    """

    required_arguments = 1
    optional_arguments = 0
    final_argument_whitespace = True
    has_content = False
    option_spec = {
        "functions": directives.unchanged_required,
    }

    def run(self):
        report_path = self.arguments[0]
        source_dir = Path(self.state.document.settings.env.srcdir)
        full_path = source_dir.parent.parent / report_path
        logger.info("Processing metrics capture: %s", full_path)

        names = [
            part.strip()
            for part in self.options.get("functions", "").split(",")
            if part.strip()
        ]
        data = load_metrics(full_path)
        rst_content = render_metrics_table(names, data)

        from docutils.parsers.rst import Parser
        from docutils.utils import new_document

        parser = Parser()
        settings = self.state.document.settings
        doc = new_document("<metrics-table>", settings=settings)
        parser.parse(rst_content, doc)
        return doc.children


def setup(app):
    app.add_directive("metrics-table", MetricsTableDirective)
    return {
        "version": "0.1.0",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }

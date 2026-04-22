"""ESP-BIST MCP Server.

Provides documentation search, API reference, Kconfig options,
architecture guidance, and source code navigation for the
ESP-BIST (Built-In Self Test) library.

Deploy on ESP MCP Deployer or run locally with: python server.py
"""

from mcp.server.fastmcp import FastMCP
from pathlib import Path
from typing import Optional

from search import BISTSearchEngine

mcp = FastMCP("esp-bist")

DATA_DIR = Path(__file__).parent / "data"
engine: Optional[BISTSearchEngine] = None


def get_engine() -> BISTSearchEngine:
    global engine
    if engine is None:
        print("Loading ESP-BIST search indexes...")
        engine = BISTSearchEngine(DATA_DIR)
    return engine


def format_doc_results(results: list[dict]) -> str:
    if not results:
        return "No results found."

    parts = []
    for i, r in enumerate(results, 1):
        title = r.get("title", "Untitled")
        section = r.get("section", "")
        source = r.get("source_file", "")
        body = r.get("body", "")
        score = r.get("_score", 0)

        if len(body) > 1500:
            body = body[:1500] + "\n... (truncated)"

        header = f"### Result {i}: {title}"
        if section:
            header += f" > {section}"
        header += f"\n**Source**: `{source}` | **Relevance**: {score}"

        parts.append(f"{header}\n\n{body}")

    return "\n\n---\n\n".join(parts)


def format_api_results(results: list[dict]) -> str:
    if not results:
        return "No matching API entries found."

    parts = []
    for entry in results:
        func = entry.get("function", "")
        sig = entry.get("signature", "")
        module = entry.get("module", "")
        desc = entry.get("description", "")
        params = entry.get("params", [])
        returns = entry.get("returns", "")
        note = entry.get("note", "")
        header = entry.get("header_file", "")
        kconfig = entry.get("kconfig_guards", [])
        enum_values = entry.get("enum_values", [])

        lines = [f"### `{func}`"]
        if module:
            lines.append(f"**Module**: {module}")
        if header:
            lines.append(f"**Header**: `{header}`")
        if sig:
            lines.append(f"\n```c\n{sig};\n```")
        if desc:
            lines.append(f"\n{desc}")
        if params:
            lines.append("\n**Parameters**:")
            for p in params:
                lines.append(f"- `{p['name']}`: {p['description']}")
        if returns:
            lines.append(f"\n**Returns**: {returns}")
        if note:
            lines.append(f"\n**Note**: {note}")
        if kconfig:
            lines.append(f"\n**Kconfig guards**: {', '.join(f'`{k}`' for k in kconfig)}")
        if enum_values:
            lines.append("\n**Values**:")
            for v in enum_values:
                lines.append(f"- `{v['name']}` = {v['value']}: {v['description']}")

        parts.append("\n".join(lines))

    return "\n\n---\n\n".join(parts)


def format_kconfig_results(results: list[dict]) -> str:
    if not results:
        return "No matching Kconfig options found."

    parts = []
    for entry in results:
        name = entry.get("name", "")
        ktype = entry.get("type", "")
        default = entry.get("default", "")
        depends = entry.get("depends_on", "")
        help_text = entry.get("help", "")
        menu = entry.get("menu", "")
        range_val = entry.get("range", "")
        prompt = entry.get("prompt", "")

        lines = [f"### `CONFIG_{name}`"]
        if prompt:
            lines.append(f"**Prompt**: {prompt}")
        if ktype:
            lines.append(f"**Type**: {ktype}")
        if default:
            lines.append(f"**Default**: `{default}`")
        if range_val:
            lines.append(f"**Range**: {range_val}")
        if depends:
            lines.append(f"**Depends on**: `{depends}`")
        if menu:
            lines.append(f"**Menu**: {menu}")
        if help_text:
            lines.append(f"\n{help_text}")

        parts.append("\n".join(lines))

    return "\n\n---\n\n".join(parts)


def format_source_results(results: list[dict]) -> str:
    if not results:
        return "No matching source code found."

    parts = []
    for r in results:
        func = r.get("function_name", "")
        filepath = r.get("file", "")
        line_start = r.get("line_start", 0)
        line_end = r.get("line_end", 0)
        module = r.get("module", "")
        body = r.get("body", "")

        if len(body) > 2000:
            body = body[:2000] + "\n    // ... (truncated)"

        lines = [f"### `{func}()`"]
        lines.append(f"**File**: `{filepath}` (lines {line_start}-{line_end})")
        if module:
            lines.append(f"**Module**: {module}")
        lines.append(f"\n```c\n{body}\n```")

        parts.append("\n".join(lines))

    return "\n\n---\n\n".join(parts)


def format_soc_results(socs: list[dict]) -> str:
    if not socs:
        return "No SoC information found."

    parts = []
    for s in socs:
        lines = [f"### {s.get('name', s.get('soc', 'Unknown'))}"]
        lines.append(f"- **CPU**: {s.get('cpu', 'N/A')}")
        lines.append(f"- **Max Frequency**: {s.get('max_freq_mhz', 'N/A')} MHz")
        lines.append(f"- **SRAM**: {s.get('sram_kb', 'N/A')}")
        lines.append(f"- **PSRAM**: {'Yes' if s.get('psram') else 'No'}")
        lines.append(f"- **GPIO Count**: {s.get('gpio_count', 'N/A')}")
        lines.append(f"- **PMA Support**: {'Yes' if s.get('has_pma') else 'No'}")
        lines.append(f"- **XT WDT Support**: {'Yes' if s.get('has_xt_wdt') else 'No'}")
        lines.append(f"- **CSR Count (tested)**: {s.get('csr_count', 'N/A')}")
        if s.get("cmake_file"):
            lines.append(f"- **CMake**: `{s['cmake_file']}`")
        if s.get("linker_scripts"):
            lines.append(f"- **Linker Scripts**: {', '.join(f'`{l}`' for l in s['linker_scripts'])}")
        if s.get("specific_notes"):
            lines.append(f"- **Notes**: {s['specific_notes']}")

        parts.append("\n".join(lines))

    return "\n\n---\n\n".join(parts)


@mcp.tool()
def search_bist_docs(query: str, soc_target: str = "") -> str:
    """Search ESP-BIST documentation for information about safety tests, architecture, and usage.

    Returns ranked documentation chunks from RST docs and READMEs.

    When to use:
        - Find documentation about ESP-BIST safety tests, IEC 60730 compliance,
          build instructions, or development workflows
        - Understand how specific BIST modules work
        - Learn about validation, coverage analysis, or tool qualification

    How to use:
        - Provide a semantic query (e.g., "How does the RAM March test work?")
        - Optionally filter by SoC target (e.g., "esp32c3", "esp32c6")

    Example:
        search_bist_docs("watchdog timer windowed underflow")
        search_bist_docs("flash CRC validation", soc_target="esp32c5")

    Returns:
        Ranked documentation chunks with source file references.
    """
    e = get_engine()
    target = soc_target if soc_target else None
    results = e.search_docs(query, top_k=8, soc_target=target)
    return format_doc_results(results)


@mcp.tool()
def get_api_reference(name: str, soc_target: str = "") -> str:
    """Get API documentation for a specific ESP-BIST function, type, or module.

    Returns function signatures, parameters, return values, and Doxygen documentation.

    When to use:
        - Look up function signatures and parameters
        - Understand what a specific BIST test function does
        - Find error codes and return types
        - Discover functions in a specific module (cpu, memory, clock, wdt, io, drivers)

    How to use:
        - Provide a function name (e.g., "bist_ram_test_march_a")
        - Or a module name to find all functions in it (e.g., "cpu", "wdt")
        - Or a type name (e.g., "bist_esp_err_t")

    Example:
        get_api_reference("bist_cpu_regs_test")
        get_api_reference("bist_esp_err_t")
        get_api_reference("wdt")

    Returns:
        Function signature, description, parameters, return values, and related config.
    """
    e = get_engine()
    exact = e.get_api_by_name(name)
    if exact:
        return format_api_results(exact)

    results = e.search_api(name, top_k=8, soc_target=soc_target if soc_target else None)
    return format_api_results(results)


@mcp.tool()
def get_architecture_info(topic: str, soc_target: str = "") -> str:
    """Get architectural documentation about the ESP-BIST project or a specific subsystem.

    Returns information about software architecture, module design, data flow,
    memory model, build system, and safety-related design decisions.

    When to use:
        - Understand the overall ESP-BIST architecture and layering
        - Learn about a specific subsystem design (CPU, memory, clock, WDT, IO)
        - Find build system and toolchain information
        - Understand memory layout and linker configuration
        - Learn about safety case, risk assessment, or IEC 60730 compliance

    How to use:
        - Provide a topic (e.g., "overview", "memory model", "build system",
          "interrupt handling", "safety requirements", "validation")

    Example:
        get_architecture_info("software architecture overview")
        get_architecture_info("windowed watchdog design")
        get_architecture_info("flash CRC build process")

    Returns:
        Relevant architectural documentation.
    """
    e = get_engine()
    target = soc_target if soc_target else None
    results = e.search_architecture(topic, top_k=8, soc_target=target)
    return format_doc_results(results)


@mcp.tool()
def search_kconfig_options(query: str) -> str:
    """Search ESP-BIST Kconfig configuration options.

    Returns matching Kconfig entries with name, type, default value,
    dependencies, and help text.

    When to use:
        - Find what configuration options are available
        - Understand what a CONFIG_ option does and its default value
        - See dependencies between Kconfig options
        - Configure BIST tests for a specific application

    How to use:
        - Search by option name (e.g., "ESP_BIST_WDT_TIMEOUT_US")
        - Or by keyword (e.g., "watchdog", "flash", "stack")
        - CONFIG_ prefix is optional

    Example:
        search_kconfig_options("WDT_TIMEOUT")
        search_kconfig_options("memory test")
        search_kconfig_options("ESP_BIST_CLOCK_PERCENT_FREQUENCY_DRIFT")

    Returns:
        Matching Kconfig options with type, default, dependencies, and help text.
    """
    e = get_engine()
    results = e.search_kconfig(query, top_k=15)
    return format_kconfig_results(results)


@mcp.tool()
def search_source_code(query: str, file_type: str = "all") -> str:
    """Search ESP-BIST source code for implementation details.

    Returns relevant code chunks with file paths, line numbers, and context.

    When to use:
        - Find how a specific BIST test is implemented
        - Look up algorithm details (March A/X, CRC32, etc.)
        - Understand driver implementations (WDT, GPIO, timer)
        - Find usage patterns or examples

    How to use:
        - Provide a search query (e.g., "march algorithm ram backup")
        - Optionally filter by file_type: "header", "source", or "all"

    Example:
        search_source_code("CRC32 flash validation")
        search_source_code("watchdog init windowed", file_type="source")

    Returns:
        Code chunks with file paths, line numbers, and function context.
    """
    e = get_engine()
    ftype = file_type if file_type in ("header", "source", "all") else "all"
    results = e.search_source(query, top_k=5, file_type=ftype)
    return format_source_results(results)


@mcp.tool()
def get_supported_socs(soc_target: str = "") -> str:
    """Get the ESP-BIST SoC support matrix and per-SoC details.

    Returns supported SoC information including CPU type, frequencies,
    memory sizes, peripheral support, and BIST-specific capabilities.

    When to use:
        - See which SoCs are supported by ESP-BIST
        - Compare SoC capabilities (memory, frequency, GPIO count)
        - Check SoC-specific features (PMA support, XT WDT, CSR count)
        - Find per-SoC CMake and linker script locations

    How to use:
        - Call without arguments for full comparison matrix
        - Provide soc_target for details on a specific SoC

    Example:
        get_supported_socs()
        get_supported_socs(soc_target="esp32c5")

    Returns:
        SoC comparison table or detailed info for a single target.
    """
    e = get_engine()
    target = soc_target if soc_target else None
    socs = e.get_socs(target)
    return format_soc_results(socs)


if __name__ == "__main__":
    mcp.run()

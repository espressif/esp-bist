# ESP-BIST MCP Server

A [Model Context Protocol](https://modelcontextprotocol.io/) server that gives AI coding assistants (Cursor, Claude, etc.) structured access to the ESP-BIST library's documentation, API reference, Kconfig options, architecture guidance, and source code.

With this server enabled, the assistant stops guessing and starts citing: it can look up exact function signatures, quote relevant safety documentation, explain Kconfig options with real defaults, and surface implementation details with file paths and line numbers -- all grounded in the actual ESP-BIST repo.

## Tools exposed

| Tool | Description |
|------|-------------|
| `search_bist_docs` | Search RST docs and READMEs for safety tests, architecture, and usage |
| `get_api_reference` | Look up function signatures, types, and Doxygen docs by name or module |
| `get_architecture_info` | Get architecture, design, memory model, and safety documentation |
| `search_kconfig_options` | Search Kconfig options with defaults, types, and dependencies |
| `search_source_code` | Search C source code for implementation details and algorithms |
| `get_supported_socs` | SoC support matrix (ESP32-C3, C5, C6, C61, H2, P4) with capabilities |

## Project layout

| File | Purpose |
|------|---------|
| `server.py` | FastMCP server with 6 tools |
| `search.py` | BM25 search engine over pre-built JSON data |
| `ingest.py` | Offline script: reads the ESP-BIST repo, produces `data/*.json` |
| `requirements.txt` | Python dependencies (`rank_bm25`, `mcp`) |
| `mcp.json` | Optional entry-point override for hosted deployments |
| `data/` | Pre-built JSON content committed to the repo |
| `.venv/` | Local virtualenv (gitignored) |

The repo root also contains `.cursor/mcp.json` which registers this server with Cursor automatically when you open the `esp-bist` project.

## Quick start

One-time setup after cloning `esp-bist`:

```bash
cd mcp-server
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

The repository ships workspace-level configs for both editors, so opening the `esp-bist` project just works:

| Editor | Config file | Verify |
|--------|-------------|--------|
| Cursor | `.cursor/mcp.json` | **Settings > MCP > ESP-BIST** lists 6 tools |
| VS Code (Copilot Chat) | `.vscode/mcp.json` | **MCP: List Servers** shows `ESP-BIST` running |

Both files use `${workspaceFolder}` so the same checkout works on any machine. No per-user configuration is needed.

The VS Code config additionally enables `dev.watch` -- the server auto-restarts when you edit anything under `mcp-server/`, useful when iterating on `ingest.py` or the tool implementations.

## Alternative: global setup (Cursor)

To make the server available outside this project, add to `~/.cursor/mcp.json`:

```json
{
  "mcpServers": {
    "ESP-BIST": {
      "command": "/absolute/path/to/esp-bist/mcp-server/.venv/bin/python3",
      "args": ["/absolute/path/to/esp-bist/mcp-server/server.py"]
    }
  }
}
```

## Alternative: global setup (VS Code)

Run **MCP: Open User Configuration** from the command palette and add:

```json
{
  "servers": {
    "ESP-BIST": {
      "type": "stdio",
      "command": "/absolute/path/to/esp-bist/mcp-server/.venv/bin/python3",
      "args": ["/absolute/path/to/esp-bist/mcp-server/server.py"]
    }
  }
}
```

## Alternative: self-hosted remote server

If you want to share a single instance across a team, you can host the server yourself and expose it over HTTP. The server is a standard FastMCP app, so any container platform or VM works:

1. Run `python ingest.py /path/to/esp-bist` to refresh `data/`.
2. Host `server.py` behind an HTTP transport of your choice (FastMCP supports both `stdio` and `streamable-http`; see the [MCP Python SDK docs](https://github.com/modelcontextprotocol/python-sdk)).
3. Point team members at the hosted URL in their editor config.

   Cursor (`~/.cursor/mcp.json`):

   ```json
   {
     "mcpServers": {
       "ESP-BIST": {
         "url": "https://your-host.example.com/mcp",
         "type": "http"
       }
     }
   }
   ```

   VS Code (run **MCP: Open User Configuration**):

   ```json
   {
     "servers": {
       "ESP-BIST": {
         "type": "http",
         "url": "https://your-host.example.com/mcp"
       }
     }
   }
   ```

Since this repository is public, make sure any hosted deployment uses infrastructure you control -- do not rely on third-party hosting that you cannot audit or take down.

## Local development and debugging

Run the server directly to inspect stdout/stderr:

```bash
.venv/bin/python server.py
```

Test tool calls with the MCP Inspector:

```bash
npx @modelcontextprotocol/inspector .venv/bin/python server.py
```

## Keeping content in sync

The `data/*.json` files in this repo are pre-built snapshots of the ESP-BIST content. Re-run ingestion whenever you change:

- RST documentation under `docs/en/`
- Public headers under `src/bist/**/*.h`
- `src/bist/Kconfig`
- Source files under `src/bist/**/*.c`
- Project READMEs

```bash
cd mcp-server
.venv/bin/python ingest.py ..
```

Then commit the updated `data/*.json` files alongside your source changes.

The ingester is deterministic and path-independent, so two developers on different machines produce byte-identical snapshots. A GitLab CI job (`mcp_data_drift` in `.gitlab-ci.yml`, stage `Lint`) re-runs `ingest.py` on every MR that touches indexed content and fails the pipeline if the committed snapshot is stale -- drift cannot be merged.

If `mcp_data_drift` fails on your MR, just run the command above locally and commit the diff. See the *CI Enforcement* section of `docs/en/mcp_server.rst` for the full rationale.

## Content coverage

- **297** documentation chunks from 13 RST files + 3 READMEs
- **57** API entries (functions, types, enums) from 16 headers
- **18** Kconfig options with defaults, types, and help text
- **50** source code function chunks from 13 C files
- **6** SoCs: ESP32-C3, ESP32-C5, ESP32-C6, ESP32-C61, ESP32-H2, ESP32-P4

MCP Server (AI Assistant Integration)
=====================================

ESP-BIST ships with a `Model Context Protocol <https://modelcontextprotocol.io/>`_ (MCP) server that exposes the library's documentation, API reference, Kconfig options, architecture guidance, and source code to AI coding assistants such as Cursor and Claude.

When the MCP server is active, the assistant answers ESP-BIST questions from the actual repository content instead of guessing: it can cite exact function signatures from ``src/bist/include``, quote chapters of this safety documentation, explain Kconfig options with their real defaults, and surface implementation details with file paths and line numbers.

.. note::

   The MCP server is a developer-productivity tool. It is **not** part of the safety-qualified scope of the ESP-BIST library. It does not run on the target SoC, does not affect generated firmware, and is excluded from tool qualification activities described in :doc:`tool_qualification`.

Location
--------

The server lives in the ``mcp-server/`` directory at the repository root:

.. code-block:: text

   esp-bist/
   ├── mcp-server/
   │   ├── server.py            # FastMCP server, exposes the 6 tools
   │   ├── search.py            # BM25 search engine
   │   ├── ingest.py            # Offline content extractor
   │   ├── requirements.txt     # Python dependencies
   │   ├── data/                # Pre-built JSON content (committed)
   │   └── README.md
   ├── .cursor/
   │   └── mcp.json             # Auto-registers the server in Cursor
   └── .vscode/
       └── mcp.json             # Auto-registers the server in VS Code

Exposed Tools
-------------

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Tool
     - Description
   * - ``search_bist_docs``
     - Full-text search over RST docs and READMEs (safety requirements, architecture, validation, ...).
   * - ``get_api_reference``
     - Look up functions, types, and enums by name or module; returns signatures and Doxygen comments.
   * - ``get_architecture_info``
     - Fetch architecture, module design, memory model, and safety documentation chapters.
   * - ``search_kconfig_options``
     - Search Kconfig symbols with their defaults, types, help text, and dependencies.
   * - ``search_source_code``
     - Locate C function implementations inside ``src/bist``.
   * - ``get_supported_socs``
     - Return the SoC support matrix with per-target capabilities.

Quick Start
-----------

One-time setup after cloning the repository:

.. code-block:: bash

   cd esp-bist/mcp-server
   python3 -m venv .venv
   .venv/bin/pip install -r requirements.txt

The repository ships workspace-level configuration files for both supported editors. Opening the project just works -- both files use ``${workspaceFolder}`` so the same checkout is portable across machines.

.. list-table::
   :header-rows: 1
   :widths: 20 30 50

   * - Editor
     - Config file (committed)
     - Verification
   * - Cursor
     - ``.cursor/mcp.json``
     - **Settings > MCP > ESP-BIST** lists the six tools.
   * - VS Code (Copilot Chat)
     - ``.vscode/mcp.json``
     - Run **MCP: List Servers** -- ``ESP-BIST`` appears as *running*.

The VS Code config additionally enables ``dev.watch``: the server automatically restarts when files under ``mcp-server/`` change, which is convenient when iterating on ``ingest.py`` or the tool implementations. Cursor does not currently support this auto-restart hint and will use the server as-is.

Global Installation
-------------------

To make the server available outside this project, register it at the user-profile level.

Cursor (``~/.cursor/mcp.json``):

.. code-block:: json

   {
     "mcpServers": {
       "ESP-BIST": {
         "command": "/absolute/path/to/esp-bist/mcp-server/.venv/bin/python3",
         "args": ["/absolute/path/to/esp-bist/mcp-server/server.py"]
       }
     }
   }

VS Code (run **MCP: Open User Configuration**):

.. code-block:: json

   {
     "servers": {
       "ESP-BIST": {
         "type": "stdio",
         "command": "/absolute/path/to/esp-bist/mcp-server/.venv/bin/python3",
         "args": ["/absolute/path/to/esp-bist/mcp-server/server.py"]
       }
     }
   }

Self-Hosted Remote Server
-------------------------

For a shared, always-on server accessible to a whole team, host the server yourself. It is a standard FastMCP application, so any container platform or VM will work:

1. Refresh content: ``python ingest.py ..``
2. Host ``server.py`` behind an HTTP transport. FastMCP supports both ``stdio`` and ``streamable-http``; see the `MCP Python SDK documentation <https://github.com/modelcontextprotocol/python-sdk>`_ for the exact wiring.
3. Team members register the hosted URL in their editor of choice.

   Cursor (``~/.cursor/mcp.json``):

   .. code-block:: json

      {
        "mcpServers": {
          "ESP-BIST": {
            "url": "https://your-host.example.com/mcp",
            "type": "http"
          }
        }
      }

   VS Code (run **MCP: Open User Configuration**):

   .. code-block:: json

      {
        "servers": {
          "ESP-BIST": {
            "type": "http",
            "url": "https://your-host.example.com/mcp"
          }
        }
      }

Because ESP-BIST is an open-source project, any hosted deployment must use infrastructure that your organization controls. Do not recommend or rely on third-party hosting that you cannot audit, update, or take down.

Keeping Content in Sync
-----------------------

The files under ``mcp-server/data/`` are pre-built JSON snapshots of the ESP-BIST repo. They must be re-generated whenever any of the following change:

- RST documentation under ``docs/en/``
- Public headers under ``src/bist/**/*.h``
- ``src/bist/Kconfig``
- Source files under ``src/bist/**/*.c``
- Project READMEs

Re-run ingestion from the ``mcp-server/`` directory:

.. code-block:: bash

   cd mcp-server
   .venv/bin/python ingest.py ..

Commit the updated ``data/*.json`` files alongside the source changes that triggered the refresh.

Snapshot writers are deterministic and path-independent: two runs on different machines produce byte-identical output. This is what allows CI to enforce the invariant.

CI Enforcement
^^^^^^^^^^^^^^

The ``mcp_data_drift`` job in ``.gitlab-ci.yml`` (stage ``Lint``) runs on every merge request that touches indexed content. It re-runs ``ingest.py`` and fails the pipeline if the committed snapshot does not match:

.. code-block:: yaml

   mcp_data_drift:
     stage: Lint
     image: python:3.12-slim
     rules:
       - changes:
           - docs/en/**/*
           - src/bist/**/*.c
           - src/bist/**/*.h
           - src/bist/Kconfig*
           - "**/README.md"
           - mcp-server/ingest.py
           - mcp-server/requirements.txt
     script:
       - pip install -r mcp-server/requirements.txt
       - python mcp-server/ingest.py .
       - git diff --quiet mcp-server/data/ || (git diff --stat mcp-server/data/; exit 1)

If this job fails on your MR, run the ``ingest.py`` command above locally and commit the resulting ``mcp-server/data/*.json`` diff.

Local Debugging
---------------

Run the server directly to inspect stdout/stderr traffic:

.. code-block:: bash

   cd mcp-server
   .venv/bin/python server.py

Exercise tool calls interactively using the official MCP Inspector:

.. code-block:: bash

   npx @modelcontextprotocol/inspector .venv/bin/python server.py

Content Coverage
----------------

The shipped ``data/`` snapshot covers:

- **285** documentation chunks across 13 RST files and 3 READMEs
- **56** API entries (functions, types, enums) from 16 public headers
- **18** Kconfig options with defaults, types, and help text
- **50** C function bodies from 13 source files
- **4** SoCs (ESP32-C3, ESP32-C5, ESP32-C6, ESP32-H2) with capability metadata

See ``mcp-server/README.md`` for the full reference.

#!/usr/bin/env bash
# Regenerate mcp-server/data/*.json when indexed source content changes.
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mcp_dir="${repo_root}/mcp-server"
venv_python="${mcp_dir}/.venv/bin/python"

if [[ ! -x "${venv_python}" ]]; then
    cat >&2 <<'EOF'
mcp-server/.venv is missing or incomplete.

Set up the MCP server virtual environment:
  cd mcp-server
  python3 -m venv .venv
  .venv/bin/pip install -r requirements.txt
EOF
    exit 1
fi

"${venv_python}" "${mcp_dir}/ingest.py" "${repo_root}"

if ! git -C "${repo_root}" diff --quiet -- mcp-server/data/; then
    cat >&2 <<'EOF'
MCP server data was refreshed and is out of sync with the commit.

Stage the updated files and commit again:
  git add mcp-server/data/
EOF
    git -C "${repo_root}" --no-pager diff --stat -- mcp-server/data/ >&2 || true
    exit 1
fi

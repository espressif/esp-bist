"""Shared idf-ci ``target`` parametrization for BIST pytest scripts."""

import pytest

# Same SOCs as the GitLab CI matrix; use ``--target=<soc>`` to run one variant (e.g. CI).
PYTEST_IDF_TARGETS = ("esp32c3", "esp32c5", "esp32c6", "esp32c61", "esp32h2")

pytestmark = pytest.mark.parametrize("target", list(PYTEST_IDF_TARGETS), indirect=True)

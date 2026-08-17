from esp_docs.conf_docs import *  # noqa: F403,F401
from esp_docs.constants import TARGETS
import sys
import os

# Add custom extensions directory to path
sys.path.insert(0, os.path.abspath(os.path.dirname(__file__) + "/extensions"))

languages = ["en"]
idf_targets = TARGETS

extensions += [  # noqa: F405
    "sphinx_copybutton",
    "sphinxcontrib.wavedrom",
    "sphinxcontrib.blockdiag",
    "esp_docs.esp_extensions.dummy_build_system",
    "esp_docs.esp_extensions.run_doxygen",
    "xml_junit_test_report",
]

# link roles config
github_repo = "espressif/esp-bist"

# context used by sphinx_idf_theme
html_context["github_user"] = "espressif"  # noqa: F405
html_context["github_repo"] = "esp-bist"  # noqa: F405

html_static_path = ["../_static"]  # noqa: F405

# Extra options required by sphinx_idf_theme
project_slug = "esp-bist"

# Contains info used for constructing target and version selector
# Can also be hosted externally, see esp-idf for example
versions_url = "./_static/docs_version.js"

# Final PDF filename will contains target and version
pdf_file_prefix = "esp-bist"

# Enable numbered figures for :numref: support
numfig = True
numfig_format = {
    "figure": "Figure %s",
    "table": "Table %s",
    "code-block": "Listing %s",
    "section": "Section %s",
}

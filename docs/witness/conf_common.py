from esp_docs.conf_docs import *  # noqa: F403,F401

languages = ["en"]
idf_targets = ["esp32c6"]

github_repo = "espressif/esp-bist"

html_context["github_user"] = "espressif"  # noqa: F405
html_context["github_repo"] = "esp-bist"  # noqa: F405

html_static_path = ["../_static"]  # noqa: F405

project_slug = "esp-bist-witness"
versions_url = "./_static/docs_version.js"
pdf_file_prefix = "esp-bist-witness-test-report"

numfig = True
numfig_format = {
    "figure": "Figure %s",
    "table": "Table %s",
    "code-block": "Listing %s",
    "section": "Section %s",
}

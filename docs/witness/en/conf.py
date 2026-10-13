# -*- coding: utf-8 -*-
#
# Witness test report Sphinx config. Built on its own, not from docs/en.

try:
    from conf_common import *  # noqa: F403,F401
except ImportError:
    import os
    import sys

    sys.path.insert(0, os.path.abspath("../"))
    from conf_common import *  # noqa: F403,F401

import datetime

current_year = datetime.datetime.now().year

project = "ESP-BIST Witness Test Report"
copyright = "2025 - {}, Espressif Systems (Shanghai) Co., Ltd".format(current_year)  # noqa: A001
pdf_title = "ESP-BIST Witness Test Report"

language = "en"

# esp-docs inserts this preamble after \usepackage{sphinx}. Inline literals
# otherwise cannot break at '_', so long identifiers overflow table columns.
_pdf_code_break = r"""
\makeatletter
\let\bist@sphinxcode\sphinxcode
\protected\def\sphinxcode#1{{%
  \def\_{%
    \textunderscore\discretionary{}{}{}%
  }%
  \bist@sphinxcode{#1}%
}}
\makeatother
\renewcommand{\idfTarget}{Espressif Systems}
"""
latex_elements["preamble"] += _pdf_code_break  # noqa: F405

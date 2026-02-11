"""
Sphinx extension for parsing and displaying JUnit XML test reports.

This extension provides a custom RST directive that reads JUnit XML test report files
and generates formatted RST output with test results, pass/fail counts, and timing information.

Usage in RST:

    .. xml-junit-test-results:: tests/cpu_reg_test/build/tests/report.xml
       :title: CPU Register Test Results
       :show-times: true
"""

import os
import xml.etree.ElementTree as ET
from pathlib import Path
from docutils import nodes
from docutils.parsers.rst import Directive, directives
from sphinx.util import logging

logger = logging.getLogger(__name__)


def parse_junit_xml(filepath):
    """
    Parse JUnit XML report and extract test information.

    Returns:
        dict: Contains 'testsuites' list with test data, or None if file not found
    """
    if not os.path.exists(filepath):
        logger.warning(f"Test report file not found: {filepath}")
        return None

    try:
        tree = ET.parse(filepath)
        root = tree.getroot()

        result = {
            'testsuites': [],
            'total_tests': 0,
            'total_passed': 0,
            'total_failed': 0,
            'total_skipped': 0,
            'total_time': 0.0,
        }

        # Handle both single testsuite and multiple testsuites
        if root.tag == 'testsuites':
            testsuites = root.findall('testsuite')
        else:
            testsuites = [root]

        for testsuite in testsuites:
            suite_data = {
                'name': testsuite.get('name', 'unknown'),
                'tests': int(testsuite.get('tests', 0)),
                'failures': int(testsuite.get('failures', 0)),
                'errors': int(testsuite.get('errors', 0)),
                'skipped': int(testsuite.get('skipped', 0)),
                'time': float(testsuite.get('time', 0.0)),
                'testcases': [],
            }

            # Extract individual test cases
            for testcase in testsuite.findall('testcase'):
                tc_data = {
                    'name': testcase.get('name', 'unknown'),
                    'classname': testcase.get('classname', ''),
                    'time': float(testcase.get('time', 0.0)),
                    'status': 'PASS',  # default
                }

                # Check for failures or errors
                if testcase.find('failure') is not None:
                    tc_data['status'] = 'FAIL'
                elif testcase.find('error') is not None:
                    tc_data['status'] = 'ERROR'
                elif testcase.find('skipped') is not None:
                    tc_data['status'] = 'SKIP'

                suite_data['testcases'].append(tc_data)

            result['testsuites'].append(suite_data)
            result['total_tests'] += suite_data['tests']
            result['total_failed'] += suite_data['failures'] + suite_data['errors']
            result['total_skipped'] += suite_data['skipped']
            result['total_time'] += suite_data['time']

        result['total_passed'] = result['total_tests'] - result['total_failed'] - result['total_skipped']

        return result

    except ET.ParseError as e:
        logger.error(f"Failed to parse XML file {filepath}: {e}")
        return None


def generate_rst_table(test_data):
    """
    Generate RST table content from test data.

    Returns:
        str: RST formatted table
    """
    if not test_data or not test_data['testsuites']:
        return ""

    lines = []

    # Summary line
    total = test_data['total_tests']
    passed = test_data['total_passed']
    failed = test_data['total_failed']
    skipped = test_data['total_skipped']
    total_time = test_data['total_time']

    lines.append("**Test Summary:**\n")
    lines.append(f"- Total: {total} tests")
    lines.append(f"- Passed: {passed}")
    lines.append(f"- Failed: {failed}")
    lines.append(f"- Skipped: {skipped}")
    lines.append(f"- Total Time: {total_time:.3f}s\n")

    # Generate table for each testsuite
    for suite in test_data['testsuites']:
        if suite['testcases']:
            lines.append(f"**Testsuite: {suite['name']}**\n")

            # Table header
            lines.append(".. list-table::")
            lines.append("   :widths: 40 15 15")
            lines.append("   :header-rows: 1\n")

            # Header row
            lines.append("   * - Test Case")
            lines.append("     - Status")
            lines.append("     - Time (s)")

            # Data rows
            for tc in suite['testcases']:
                status_icon = "✓ PASS" if tc['status'] == 'PASS' else f"✗ {tc['status']}"
                lines.append(f"   * - ``{tc['name']}``")
                lines.append(f"     - {status_icon}")
                lines.append(f"     - {tc['time']:.3f}")

            lines.append("")

    return "\n".join(lines)


class XmlJunitTestResultsDirective(Directive):
    """
    Sphinx directive for including test results from JUnit XML reports.

    Usage:
        .. xml-junit-test-results:: path/to/report.xml
           :title: Test Results Title
           :show-times: true
    """
    required_arguments = 1
    optional_arguments = 0
    final_argument_whitespace = True
    has_content = False
    option_spec = {
        'title': directives.unchanged,
        'show-times': directives.flag,
    }

    def run(self):
        # Get the report file path relative to source directory
        report_path = self.arguments[0]

        # Resolve path relative to source directory
        source_dir = Path(self.state.document.settings.env.srcdir)
        full_path = source_dir.parent.parent / report_path

        logger.info(f"Processing test results from: {full_path}")

        # Parse the XML file
        test_data = parse_junit_xml(str(full_path))

        if not test_data:
            logger.warning(f"Could not parse test results from {report_path}")
            return [nodes.warning(
                '',
                nodes.paragraph(text=f"Test report not found or could not be parsed: {report_path}")
            )]

        # Generate RST content
        rst_content = generate_rst_table(test_data)

        # Parse the generated RST and return nodes
        from docutils.parsers.rst import Parser
        from docutils.utils import new_document

        parser = Parser()
        settings = self.state.document.settings
        doc = new_document('<test-results>', settings=settings)
        parser.parse(rst_content, doc)

        return doc.children


def setup(app):
    """
    Register the extension with Sphinx.

    Args:
        app: Sphinx application object
    """
    app.add_directive('xml-junit-test-results', XmlJunitTestResultsDirective)

    return {
        'version': '0.1.0',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }

from tests.idf_targets import pytestmark  # noqa: F401


def test_standalone_sample(dut, target):
    expected_outputs = [
        "All post boot tests passed",
        "Initializing WDT",
    ]

    for expected_output in expected_outputs:
        dut.expect(expected_output, timeout=10)

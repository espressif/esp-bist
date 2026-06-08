from tests.idf_targets import pytestmark  # noqa: F401


def test_windowed_wdt_success(dut, target):
    tests_names = [
        "test_BIST_WINDOWED_WDT_NORMAL",
        "test_BIST_WINDOWED_WDT_UNDERFLOW",
        "test_BIST_WINDOWED_WDT_CONSECUTIVE",
    ]
    expected_outputs = [f"{test}:PASS" for test in tests_names]

    for expected_output in expected_outputs:
        dut.expect(expected_output)

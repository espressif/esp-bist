from tests.idf_targets import pytestmark  # noqa: F401


def test_interrupt_success(dut, target):
    tests_names = ["test_BIST_Interrupt_Source_Map", "test_BIST_Hardware_Interrupt"]
    expected_outputs = [f"{test}:PASS" for test in tests_names]

    # expect from what esptool.py printed to sys.stdout
    for expected_output in expected_outputs:
        dut.expect(expected_output)

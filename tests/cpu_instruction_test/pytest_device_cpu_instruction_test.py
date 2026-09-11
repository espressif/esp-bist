from tests.idf_targets import pytestmark  # noqa: F401


def test_cpu_instruction_success(dut, target):
    dut.expect("test_BIST_Cpu_Instructions:PASS")

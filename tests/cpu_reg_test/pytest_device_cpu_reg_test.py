def test_cpu_reg_success(dut):
    tests_names = ["test_BIST_Cpu_Regs", "test_BIST_Cpu_Csr_Regs"]
    expected_outputs = [f"{test}:PASS" for test in tests_names]

    # expect from what esptool.py printed to sys.stdout
    for expected_output in expected_outputs:
        dut.expect(expected_output)

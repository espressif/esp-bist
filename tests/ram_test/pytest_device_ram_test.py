def test_ram_success(dut):
    tests_names = ["test_BIST_ram_march_a", "test_BIST_ram_march_x"]
    expected_outputs = [f"{test}:PASS" for test in tests_names]

    # expect from what esptool.py printed to sys.stdout
    for expected_output in expected_outputs:
        dut.expect(expected_output)

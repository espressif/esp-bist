import time

import pytest

from tests.idf_targets import pytestmark  # noqa: F401

COMMON_CSRS = [
    "mtvec",
    "mscratch",
    "mepc",
    "mcause",
    "mtval",
    "pmpaddr0",
    "pmpaddr1",
    "pmpaddr2",
    "pmpaddr3",
    "pmpaddr4",
    "pmpaddr5",
    "pmpaddr6",
    "pmpaddr7",
    "pmpaddr8",
    "pmpaddr9",
    "pmpaddr10",
    "pmpaddr11",
    "pmpaddr12",
    "pmpaddr13",
    "pmpaddr14",
    "pmpaddr15",
    "pmpcfg0",
    "pmpcfg1",
    "pmpcfg2",
    "pmpcfg3",
]

PMA_ADDR_CSRS = [
    "0xBD0",
    "0xBD1",
    "0xBD2",
    "0xBD3",
    "0xBD4",
    "0xBD5",
    "0xBD6",
    "0xBD7",
    "0xBD8",
    "0xBD9",
    "0xBDA",
    "0xBDB",
]

C5_ONLY_CSRS = [
    "0x7E1",
    "0x7C5",
]

PMA_TARGETS = {"esp32c5", "esp32c6", "esp32h2"}


def test_cpu_reg_success(dut, target):
    tests_names = ["test_BIST_Cpu_Regs", "test_BIST_Cpu_Csr_Regs"]
    expected_outputs = [f"{test}:PASS" for test in tests_names]

    # expect from what esptool.py printed to sys.stdout
    for expected_output in expected_outputs:
        dut.expect(expected_output)


def cpu_reg_error_test_device(openocd_dut, gdb_instance, dut, reg_name, script=None):
    expected_output = "FAIL"
    if script is None:
        script = """
    # connect to OpenOCD GDB stub (QEMU uses :1234)
    target remote 127.0.0.1:3333
    mon reset halt
    maintenance flush register-cache
    monitor gdb breakpoint_override hard

    # Set a breakpoint at the specific address
    tb testRegA_{}
    commands
        set ${}=0x55555555
        monitor step
        monitor resume
    end
    continue
    """.format(reg_name, reg_name)

    time.sleep(1)

    with openocd_dut.run():
        gdb_process = gdb_instance.attach(script)
        time.sleep(5)
        try:
            dut.expect(expected_output, timeout=30)
        finally:
            gdb_instance.stop(gdb_process)


@pytest.mark.injection
@pytest.mark.parametrize(
    "reg_name",
    [
        "ra",
        "sp",
        "gp",
        "tp",
        "t0",
        "t1",
        "t2",
        "t3",
        "t4",
        "t5",
        "t6",
        "s0",
        "s1",
        "s2",
        "s3",
        "s4",
        "s5",
        "s6",
        "s7",
        "s8",
        "s9",
        "s10",
        "s11",
        "a0",
        "a1",
        "a2",
        "a3",
        "a4",
        "a5",
        "a6",
        "a7",
    ],
)
def test_reg_error_device(openocd_dut, gdb_instance, dut, reg_name, target):
    cpu_reg_error_test_device(openocd_dut, gdb_instance, dut, reg_name)


@pytest.mark.injection
@pytest.mark.parametrize(
    "freg_name",
    [
        "ft0",
        "ft1",
        "ft2",
        "ft3",
        "ft4",
        "ft5",
        "ft6",
        "ft7",
        "ft8",
        "ft9",
        "ft10",
        "ft11",
        "fs0",
        "fs1",
        "fs2",
        "fs3",
        "fs4",
        "fs5",
        "fs6",
        "fs7",
        "fs8",
        "fs9",
        "fs10",
        "fs11",
        "fa0",
        "fa1",
        "fa2",
        "fa3",
        "fa4",
        "fa5",
        "fa6",
        "fa7",
    ],
)
def test_freg_error_device(openocd_dut, gdb_instance, dut, freg_name, target):
    if target not in ("esp32h4", "esp32p4"):
        pytest.skip("Test is only available for FPU supported boards")
    script = """
# connect to OpenOCD GDB stub (QEMU uses :1234)
target remote 127.0.0.1:3333
mon reset halt
maintenance flush register-cache
monitor gdb breakpoint_override hard

tb testFRegA_{}
commands
    set ${}=0x555555555
    monitor step
    monitor resume
end
continue
""".format(freg_name, freg_name)
    cpu_reg_error_test_device(openocd_dut, gdb_instance, dut, freg_name, script)


@pytest.mark.injection
@pytest.mark.parametrize("csr_name", COMMON_CSRS + PMA_ADDR_CSRS + C5_ONLY_CSRS)
def test_reg_csr_error_device(openocd_dut, gdb_instance, dut, csr_name, target):
    if csr_name in PMA_ADDR_CSRS and target not in PMA_TARGETS:
        pytest.skip(f"CSR {csr_name} requires PMA (not available on {target})")
    if csr_name in C5_ONLY_CSRS and target != "esp32c5":
        pytest.skip(f"CSR {csr_name} only available on ESP32-C5")
    script = """
# connect to OpenOCD GDB stub (QEMU uses :1234)
target remote 127.0.0.1:3333
mon reset halt
maintenance flush register-cache
monitor gdb breakpoint_override hard

tb testRegA_{}
commands
    set $t0=0x55555555
    monitor step
    monitor resume
end
continue
""".format(csr_name)
    cpu_reg_error_test_device(openocd_dut, gdb_instance, dut, csr_name, script)


@pytest.mark.injection
@pytest.mark.parametrize("fcsr_name", ["fflags", "frm", "fcsr"])
def test_freg_csr_error_device(openocd_dut, gdb_instance, dut, fcsr_name, target):
    if target not in ("esp32h4", "esp32p4"):
        pytest.skip("Test is only available for FPU supported boards")
    script = """
# connect to OpenOCD GDB stub (QEMU uses :1234)
target remote 127.0.0.1:3333
mon reset halt
maintenance flush register-cache
monitor gdb breakpoint_override hard

tb testRegA_{}
commands
    set $t0=0x55555555
    monitor step
    monitor resume
end
continue
""".format(fcsr_name)
    cpu_reg_error_test_device(openocd_dut, gdb_instance, dut, fcsr_name, script)

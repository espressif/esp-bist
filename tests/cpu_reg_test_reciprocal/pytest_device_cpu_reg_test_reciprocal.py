"""On-device fault injection for the reciprocal CPU register and CSR test.

The reciprocal test executes the same walk once on HP Core 0 and once on HP
Core 1. OpenOCD puts both HP cores in one SMP group and arms a hardware
breakpoint on every target of that group, so a plain temporary breakpoint
always fires during Core 0's walk. Core 1 injection therefore skips the first
hit and acts on the second one.
"""

import time

import pytest

from tests.idf_targets import pytestmark  # noqa: F401

CORES = (0, 1)
CORE_IDS = [f"core{core}" for core in CORES]

GPR_NAMES = [
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
]

FPR_NAMES = [
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
]

# Trap and PMP CSRs. The reciprocal CSR walk is ESP32-P4 only, so the per-SoC
# mask variants and the ESP32-C5-only CSRs of the static test do not apply.
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

# Written as 0xBD0-0xBDB in the walk, labelled pmaaddr0-pmaaddr11.
PMA_ADDR_CSRS = [f"pmaaddr{index}" for index in range(12)]

FPU_CSRS = ["fflags", "frm", "fcsr"]

INJECT_GPR_VALUE = "0x55555555"
# Same literal as the static CPU register test: GDB widens it to the FPR union,
# and the raw binary32 bits end up different from the 0xAAAAAAAA under check.
INJECT_FPR_VALUE = "0x55555555"

CPU_REGS_TEST = "test_BIST_Cpu_Regs"
CPU_CSR_REGS_TEST = "test_BIST_Cpu_Csr_Regs"

# Core 0 walks the registers before dispatching to Core 1, so its label hit is
# the first one and a temporary breakpoint is enough. This matches the static
# reference test.
GDB_SCRIPT_CORE0 = """\
# connect to OpenOCD GDB stub
target remote 127.0.0.1:3333
mon reset halt
maintenance flush register-cache
monitor gdb breakpoint_override hard

tb {label}
commands
    {inject}
    monitor step
    monitor resume
end
continue
"""

# Both harts share one breakpoint and one halt group, and GDB is attached to
# Core 0. Core 0 therefore consumes the label hit. `ignore` single-steps that
# hit and the later Core 1 stop is reported as SIGINT on Core 0, so a command
# list never runs. Step Core 0 past the label, wait for the halt-group stop,
# and write the register through OpenOCD on cpu1: GDB's `set` follows Core 0.
GDB_SCRIPT_CORE1 = """\
# connect to OpenOCD GDB stub
target remote 127.0.0.1:3333
mon reset halt
maintenance flush register-cache
monitor gdb breakpoint_override hard

hbreak {label}
continue
monitor step
continue
monitor targets esp32p4.hp.cpu1
monitor reg {reg} {value}
disable breakpoints
monitor targets esp32p4.hp.cpu0
monitor resume
"""

GDB_SCRIPTS = {
    0: GDB_SCRIPT_CORE0,
    1: GDB_SCRIPT_CORE1,
}


def cpu_reg_error_test_device(
    openocd_dut, gdb_instance, dut, unity_test, label, inject, core
):
    """Corrupt `inject` at `label` on `core` and expect `unity_test` to report FAIL."""
    reg_name, reg_value = inject.lstrip("$").split("=", 1)
    # OpenOCD names RISC-V x8 "fp". GDB's $s0 is the same register, and
    # `monitor reg s0` fails with "register s0 not found".
    openocd_reg_name = "fp" if reg_name == "s0" else reg_name
    script = GDB_SCRIPTS[core].format(
        label=label,
        inject=f"set {inject}",
        reg=openocd_reg_name,
        value=reg_value,
    )
    time.sleep(1)

    with openocd_dut.run():
        gdb_process = gdb_instance.attach(script)
        try:
            # The serial buffer keeps lines printed while GDB connects and
            # injects, so expect returns as soon as the FAIL line arrives.
            dut.expect(f"{unity_test}:FAIL", timeout=30)
        finally:
            gdb_instance.stop(gdb_process)


def test_cpu_reg_success(dut, target):
    if target != "esp32p4":
        pytest.skip("Reciprocal CPU register test is ESP32-P4 only")

    tests_names = [CPU_REGS_TEST, CPU_CSR_REGS_TEST]
    expected_outputs = [f"{test}:PASS" for test in tests_names]

    # expect from what esptool.py printed to sys.stdout
    for expected_output in expected_outputs:
        dut.expect(expected_output)


@pytest.mark.injection
@pytest.mark.parametrize("core", CORES, ids=CORE_IDS)
@pytest.mark.parametrize("reg_name", GPR_NAMES)
def test_reg_error_device(openocd_dut, gdb_instance, dut, reg_name, core, target):
    """Corrupt a general-purpose register while the walk compares it."""
    if target != "esp32p4":
        pytest.skip("Reciprocal CPU register test is ESP32-P4 only")
    cpu_reg_error_test_device(
        openocd_dut,
        gdb_instance,
        dut,
        CPU_REGS_TEST,
        f"bist_cc_test_gpr_a_{reg_name}",
        f"${reg_name}={INJECT_GPR_VALUE}",
        core,
    )


@pytest.mark.injection
@pytest.mark.parametrize("core", CORES, ids=CORE_IDS)
@pytest.mark.parametrize("freg_name", FPR_NAMES)
def test_freg_error_device(openocd_dut, gdb_instance, dut, freg_name, core, target):
    """Corrupt a floating-point register while the walk compares it."""
    if target != "esp32p4":
        pytest.skip("Reciprocal CPU register test is ESP32-P4 only")
    cpu_reg_error_test_device(
        openocd_dut,
        gdb_instance,
        dut,
        CPU_REGS_TEST,
        f"bist_cc_test_fpr_a_{freg_name}",
        f"${freg_name}={INJECT_FPR_VALUE}",
        core,
    )


@pytest.mark.injection
@pytest.mark.parametrize("core", CORES, ids=CORE_IDS)
@pytest.mark.parametrize("csr_name", COMMON_CSRS + PMA_ADDR_CSRS)
def test_reg_csr_error_device(openocd_dut, gdb_instance, dut, csr_name, core, target):
    """Corrupt the masked expected value of a trap, PMP, or PMA CSR check."""
    if target != "esp32p4":
        pytest.skip("Reciprocal CPU register test is ESP32-P4 only")
    cpu_reg_error_test_device(
        openocd_dut,
        gdb_instance,
        dut,
        CPU_CSR_REGS_TEST,
        f"bist_cc_test_csr_a_{csr_name}",
        f"$t0={INJECT_GPR_VALUE}",
        core,
    )


@pytest.mark.injection
@pytest.mark.parametrize("core", CORES, ids=CORE_IDS)
@pytest.mark.parametrize("fcsr_name", FPU_CSRS)
def test_freg_csr_error_device(openocd_dut, gdb_instance, dut, fcsr_name, core, target):
    """Corrupt the masked expected value of an FPU CSR check."""
    if target != "esp32p4":
        pytest.skip("Reciprocal CPU register test is ESP32-P4 only")
    cpu_reg_error_test_device(
        openocd_dut,
        gdb_instance,
        dut,
        CPU_CSR_REGS_TEST,
        f"bist_cc_test_csr_a_{fcsr_name}",
        f"$t0={INJECT_GPR_VALUE}",
        core,
    )

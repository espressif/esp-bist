/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * HP-core side of the ESP-BIST Zephyr sample.
 *
 * Starts the Host Diagnostic Agent, which sends AGENT_READY, queues the LP
 * companion status bitmasks and answers Q&A challenges on its worker thread.
 * The companion under remote/ does the actual testing; this side only reports.
 * Mirrors samples/idf/main/main.c so both platforms read the same way.
 *
 * This is the happy path only, so it stays readable as a starting point.
 * Fail-closed behaviour is validated in tests/integration/hd_zephyr, which
 * makes the host misbehave and observes the companion resetting it.
 */

#include <zephyr/kernel.h>

#include <bist_hd_agent.h>
#include <bist_hd_protocol.h>

/* Let the LP core boot and register its mbox callback before AGENT_READY. */
#define LP_BOOT_SETTLE_MS 200

#define STATUS_TIMEOUT_MS 10000
#define RUNTIME_LOOPS     5

/* Tests enabled on the LP core, from remote/prj.conf. */
#define POSTBOOT_EXPECTED                                                                          \
	(BIST_HD_BIT_CPU_REG | BIST_HD_BIT_RAM_X | BIST_HD_BIT_FLASH | BIST_HD_BIT_ABRAHAM)
#define RUNTIME_EXPECTED                                                                           \
	(BIST_HD_BIT_CPU_REG | BIST_HD_BIT_RAM_A | BIST_HD_BIT_STACK | BIST_HD_BIT_ABRAHAM)

static void print_postboot_results(uint32_t result)
{
	printk("=== Post-boot BIST results ===\n");
	printk("test_BIST_cpu_reg:%s\n", (result & BIST_HD_BIT_CPU_REG) ? "PASS" : "FAIL");
	printk("test_BIST_ram_march_x:%s\n", (result & BIST_HD_BIT_RAM_X) ? "PASS" : "FAIL");
	printk("test_BIST_ram_abraham:%s\n", (result & BIST_HD_BIT_ABRAHAM) ? "PASS" : "FAIL");
	printk("test_BIST_flash_crc:%s\n", (result & BIST_HD_BIT_FLASH) ? "PASS" : "FAIL");
}

static void print_runtime_results(uint32_t result)
{
	printk("=== Runtime BIST results ===\n");
	printk("test_BIST_runtime_cpu_reg:%s\n", (result & BIST_HD_BIT_CPU_REG) ? "PASS" : "FAIL");
	printk("test_BIST_runtime_ram_march_a:%s\n", (result & BIST_HD_BIT_RAM_A) ? "PASS" : "FAIL");
	printk("test_BIST_runtime_ram_abraham:%s\n",
	       (result & BIST_HD_BIT_ABRAHAM) ? "PASS" : "FAIL");
	printk("test_BIST_runtime_stack_check:%s\n", (result & BIST_HD_BIT_STACK) ? "PASS" : "FAIL");
}

int main(void)
{
	uint32_t status;
	bool all_pass = true;
	bool runtime_ok = true;
	int err;

	k_msleep(LP_BOOT_SETTLE_MS);

	err = bist_hd_agent_start();
	if (err != 0) {
		printk("Failed to start Host Diagnostic Agent: %d\n", err);
		printk("test_HD_agent_ready:FAIL\n");
		return 0;
	}
	printk("Host Diagnostic Agent started\n");
	printk("test_HD_agent_ready:PASS\n");

	err = bist_hd_agent_wait_lp_status(&status, BIST_HD_BIT_POSTBOOT, STATUS_TIMEOUT_MS);
	if (err == 0) {
		print_postboot_results(status);
		all_pass = (status & POSTBOOT_EXPECTED) == POSTBOOT_EXPECTED;
	} else {
		printk("test_BIST_postboot:TIMEOUT\n");
		all_pass = false;
	}

	for (int i = 1; i <= RUNTIME_LOOPS; i++) {
		err = bist_hd_agent_wait_lp_status(&status, BIST_HD_BIT_RUNTIME, STATUS_TIMEOUT_MS);
		if (err != 0) {
			printk("test_BIST_runtime:TIMEOUT (loop %d)\n", i);
			all_pass = false;
			runtime_ok = false;
			break;
		}
		printk("--- Runtime loop %d/%d ---\n", i, RUNTIME_LOOPS);
		print_runtime_results(status);
		if ((status & RUNTIME_EXPECTED) != RUNTIME_EXPECTED) {
			all_pass = false;
		}
	}

	if (IS_ENABLED(CONFIG_ESP_BIST_HD_AUDIT_QA)) {
		/*
		 * Completing all runtime loops proves Q&A rounds passed; a
		 * runtime test failure does not invalidate Q&A.
		 */
		printk("test_HD_challenge:%s\n", runtime_ok ? "PASS" : "FAIL");
	}

	printk("BIST_RESULT:%s\n", all_pass ? "PASS" : "FAIL");

	/*
	 * The agent thread keeps receiving companion messages (runtime status
	 * and optional host-audit traffic). Park main so the sample stays up.
	 */
	while (1) {
		k_msleep(1000);
	}

	return 0;
}

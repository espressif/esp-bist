/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * HP-core side of the Host Diagnostics validation app for Zephyr.
 *
 * This is not an integration example; samples/zephyr is. Here the assertions
 * live: the ztest build checks the companion's post-boot and runtime bitmasks
 * and its repeated Q&A rounds, and the fail-closed builds make the host
 * misbehave so the companion must reset it.
 *
 * The ESP-BIST library is built exactly as a product would build it. A fault is
 * either a host image carrying a challenge key the LP image does not share
 * (CONFIG_ESP_BIST_HD_CHALLENGE_KEY differs between the two sysbuild images) or
 * this file starving the agent past the challenge window
 * (CONFIG_BIST_HD_TEST_STARVE_AGENT). Either way the companion enters safe
 * state, stops feeding the LP watchdog, and the chip resets.
 */

#include <zephyr/kernel.h>

#include <bist_hd_agent.h>
#include <bist_hd_protocol.h>

/* Let the LP core boot and register its mbox callback before AGENT_READY. */
#define LP_BOOT_SETTLE_MS 200

#define STATUS_TIMEOUT_MS 10000
#define RUNTIME_LOOPS     5

#ifdef CONFIG_BIST_HD_TEST_STARVE_AGENT
/* Outlasts the challenge window plus the companion LP watchdog timeout. */
#define STARVE_US 500000

/*
 * Model a host too busy to answer: become cooperative above the agent thread
 * priority and busy-wait, so the agent is never scheduled and no ANSWER reaches
 * the companion before the window closes. Nothing in the library participates.
 */
static void starve_agent(void)
{
	k_thread_priority_set(k_current_get(), K_PRIO_COOP(0));
	k_busy_wait(STARVE_US);
}
#endif

#ifdef CONFIG_ZTEST

#include <zephyr/ztest.h>

static uint32_t postboot_result;
static uint32_t runtime_result;
static uint32_t runtime_rounds;

#ifdef CONFIG_BIST_HD_TEST_STARVE_AGENT

#include <zephyr/drivers/hwinfo.h>

static bool armed_boot;
static uint32_t reset_cause;

/*
 * Check the hardware reset cause: the first boot (power-on, flash, or pin reset)
 * arms the starvation test, while the post-safe-state boot observes RESET_WATCHDOG.
 */
static void fail_closed_boot_classify(void)
{
	if (hwinfo_get_reset_cause(&reset_cause) != 0) {
		reset_cause = 0;
	}

	armed_boot = ((reset_cause & RESET_WATCHDOG) == 0);
}

#endif /* CONFIG_BIST_HD_TEST_STARVE_AGENT */

static void *bist_suite_setup(void)
{
	int err;

#ifdef CONFIG_BIST_HD_TEST_STARVE_AGENT
	fail_closed_boot_classify();
#endif

	k_msleep(LP_BOOT_SETTLE_MS);

	err = bist_hd_agent_start();
	zassert_ok(err, "failed to start Host Diagnostic Agent: %d", err);

	err = bist_hd_agent_wait_lp_status(&postboot_result, BIST_HD_BIT_POSTBOOT,
					   STATUS_TIMEOUT_MS);
	zassert_ok(err, "timed out waiting for LP companion post-boot status");

#ifdef CONFIG_BIST_HD_TEST_STARVE_AGENT
	if (armed_boot) {
		/*
		 * The companion reports one runtime status before its first
		 * challenge, so the agent is known live when starvation starts.
		 */
		err = bist_hd_agent_wait_lp_status(&runtime_result, BIST_HD_BIT_RUNTIME,
						   STATUS_TIMEOUT_MS);
		zassert_ok(err, "timed out waiting for the first runtime status");

		starve_agent();
		zassert_unreachable("companion did not reset the host after the missed window");
	}
#endif

	/*
	 * The companion issues one challenge per runtime round and stops
	 * reporting once it enters safe state, so completing every round
	 * means each preceding Q&A round passed.
	 */
	for (uint32_t i = 0; i < RUNTIME_LOOPS; i++) {
		err = bist_hd_agent_wait_lp_status(&runtime_result, BIST_HD_BIT_RUNTIME,
						   STATUS_TIMEOUT_MS);
		zassert_ok(err, "timed out waiting for LP companion runtime status %u", i + 1);
		runtime_rounds++;
	}

	return NULL;
}

/* --- Host Diagnostics --- */

ZTEST(bist_lp, test_hd_runtime_rounds)
{
	zassert_equal(runtime_rounds, RUNTIME_LOOPS,
		      "companion stopped reporting after %u of %u rounds", runtime_rounds,
		      RUNTIME_LOOPS);
}

ZTEST(bist_lp, test_hd_challenge)
{
	if (!IS_ENABLED(CONFIG_ESP_BIST_HD_AUDIT_QA)) {
		ztest_test_skip();
	}

	/*
	 * Reported separately from test_hd_runtime_rounds so the Q&A audit is
	 * traceable in the test report. Both rest on the companion verdict: a
	 * host-side answer count would only show the host tried.
	 */
	zassert_equal(runtime_rounds, RUNTIME_LOOPS,
		      "companion stopped issuing challenges after %u of %u rounds",
		      runtime_rounds, RUNTIME_LOOPS);
}

ZTEST(bist_lp, test_hd_fail_closed_reset_cause)
{
#ifdef CONFIG_BIST_HD_TEST_STARVE_AGENT
	/*
	 * Reaching the second boot at all is the fail-closed evidence: the
	 * classifier only clears the armed state on a watchdog reset, so a
	 * panic or brownout would have restarted the starved round instead.
	 * Assert the cause explicitly so the report names what reset us.
	 */
	zassert_false(armed_boot, "suite completed without a safe-state reset");
	zassert_true(reset_cause & RESET_WATCHDOG,
		     "expected LP WDT reset, got hwinfo cause 0x%08x", reset_cause);
#else
	ztest_test_skip();
#endif
}

/* --- Post-boot test assertions --- */

ZTEST(bist_lp, test_postboot_cpu_reg)
{
	zassert_true(postboot_result & BIST_HD_BIT_CPU_REG,
		     "LP post-boot: CPU register test failed");
}

ZTEST(bist_lp, test_postboot_ram_march_x)
{
	zassert_true(postboot_result & BIST_HD_BIT_RAM_X,
		     "LP post-boot: RAM March-X test failed");
}

ZTEST(bist_lp, test_postboot_ram_abraham)
{
	zassert_true(postboot_result & BIST_HD_BIT_ABRAHAM,
		     "LP post-boot: RAM Abraham test failed");
}

ZTEST(bist_lp, test_postboot_flash_crc)
{
	zassert_true(postboot_result & BIST_HD_BIT_FLASH,
		     "LP post-boot: Flash CRC test failed");
}

/* --- Runtime test assertions --- */

ZTEST(bist_lp, test_runtime_cpu_reg)
{
	zassert_true(runtime_result & BIST_HD_BIT_CPU_REG,
		     "LP runtime: CPU register test failed");
}

ZTEST(bist_lp, test_runtime_ram_march_a)
{
	zassert_true(runtime_result & BIST_HD_BIT_RAM_A,
		     "LP runtime: RAM March-A test failed");
}

ZTEST(bist_lp, test_runtime_ram_abraham)
{
	zassert_true(runtime_result & BIST_HD_BIT_ABRAHAM,
		     "LP runtime: RAM Abraham test failed");
}

ZTEST(bist_lp, test_runtime_stack_check)
{
	zassert_true(runtime_result & BIST_HD_BIT_STACK,
		     "LP runtime: Stack overflow check failed");
}

ZTEST_SUITE(bist_lp, NULL, bist_suite_setup, NULL, NULL, NULL);

#else /* !CONFIG_ZTEST — console fail-closed build */

int main(void)
{
	uint32_t status;
	int err;

	k_msleep(LP_BOOT_SETTLE_MS);

	err = bist_hd_agent_start();
	if (err != 0) {
		printk("test_HD_agent_ready:FAIL\n");
		return 0;
	}
	printk("test_HD_agent_ready:PASS\n");

	err = bist_hd_agent_wait_lp_status(&status, BIST_HD_BIT_POSTBOOT, STATUS_TIMEOUT_MS);
	printk("test_BIST_postboot:%s\n", (err == 0) ? "PASS" : "TIMEOUT");

	/*
	 * One runtime status is reported before the first challenge, so this
	 * marker confirms the supervision loop is live before the fault lands.
	 */
	err = bist_hd_agent_wait_lp_status(&status, BIST_HD_BIT_RUNTIME, STATUS_TIMEOUT_MS);
	printk("test_HD_fail_closed_armed:%s\n", (err == 0) ? "PASS" : "FAIL");

#ifdef CONFIG_BIST_HD_TEST_STARVE_AGENT
	starve_agent();
#endif

	while (1) {
		k_msleep(1000);
	}

	return 0;
}

#endif /* CONFIG_ZTEST */

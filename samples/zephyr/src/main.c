/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * HP-core side of the ESP-BIST Zephyr sample.
 *
 * Waits for the LP core to execute BIST tests and report results over
 * the mailbox. Two rounds are collected:
 *   1. Post-boot  – runs once right after LP core boots.
 *   2. Runtime    – first periodic runtime round.
 *
 * Each message is a uint32_t bitmask (bit set = test passed).
 * Individual ztests assert on each bit from the post-boot and runtime
 * results.
 *
 * Handshake: the HP core sends BIST_MSG_READY to the LP core after
 * registering the mbox callback. The LP core waits for this before
 * running any tests, ensuring no results are lost.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/mbox.h>
#include <string.h>

#define BIST_BIT_CPU_REG  BIT(0)
#define BIST_BIT_CPU_CSR  BIT(1)
#define BIST_BIT_RAM_A    BIT(2)
#define BIST_BIT_RAM_X    BIT(3)
#define BIST_BIT_FLASH    BIT(4)
#define BIST_BIT_STACK    BIT(5)
#define BIST_BIT_RUNTIME  BIT(30)
#define BIST_BIT_DONE     BIT(31)

#define BIST_MSG_READY    0xCAFECAFE

static K_SEM_DEFINE(postboot_sem, 0, 1);
static K_SEM_DEFINE(runtime_sem, 0, 1);

static volatile uint32_t postboot_result;
static volatile uint32_t runtime_result;

static void mbox_cb(const struct device *dev, mbox_channel_id_t channel_id,
		    void *user_data, struct mbox_msg *data)
{
	uint32_t value = 0;

	memcpy(&value, data->data, MIN(data->size, sizeof(value)));

	if (!(value & BIST_BIT_RUNTIME)) {
		postboot_result = value;
		k_sem_give(&postboot_sem);
	} else {
		runtime_result = value;
		k_sem_give(&runtime_sem);
	}
}

static void *bist_suite_setup(void)
{
	const struct mbox_dt_spec rx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), rx);
	const struct mbox_dt_spec tx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), tx);
	uint32_t ready_msg = BIST_MSG_READY;
	struct mbox_msg msg = { .data = &ready_msg, .size = sizeof(ready_msg) };
	int ret;

	ret = mbox_register_callback_dt(&rx_channel, mbox_cb, NULL);
	zassert_ok(ret, "mbox_register_callback failed: %d", ret);

	ret = mbox_set_enabled_dt(&rx_channel, 1);
	zassert_ok(ret, "mbox_set_enabled failed: %d", ret);

	ret = mbox_send_dt(&tx_channel, &msg);
	zassert_ok(ret, "failed to send ready signal to LP core: %d", ret);

	ret = k_sem_take(&postboot_sem, K_SECONDS(10));
	zassert_ok(ret, "timed out waiting for LP core post-boot results");

	ret = k_sem_take(&runtime_sem, K_SECONDS(10));
	zassert_ok(ret, "timed out waiting for LP core runtime results");

	return NULL;
}

/* --- Post-boot test assertions --- */

ZTEST(bist_lp, test_postboot_cpu_reg)
{
	zassert_true(postboot_result & BIST_BIT_CPU_REG,
		     "LP post-boot: CPU register test failed");
}

ZTEST(bist_lp, test_postboot_ram_march_x)
{
	zassert_true(postboot_result & BIST_BIT_RAM_X,
		     "LP post-boot: RAM March-X test failed");
}

ZTEST(bist_lp, test_postboot_flash_crc)
{
	zassert_true(postboot_result & BIST_BIT_FLASH,
		     "LP post-boot: Flash CRC test failed");
}

/* --- Runtime test assertions --- */

ZTEST(bist_lp, test_runtime_cpu_reg)
{
	zassert_true(runtime_result & BIST_BIT_CPU_REG,
		     "LP runtime: CPU register test failed");
}

ZTEST(bist_lp, test_runtime_ram_march_a)
{
	zassert_true(runtime_result & BIST_BIT_RAM_A,
		     "LP runtime: RAM March-A test failed");
}

ZTEST(bist_lp, test_runtime_stack_check)
{
	zassert_true(runtime_result & BIST_BIT_STACK,
		     "LP runtime: Stack overflow check failed");
}

ZTEST_SUITE(bist_lp, NULL, bist_suite_setup, NULL, NULL, NULL);

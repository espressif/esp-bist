/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * LP-core side of the ESP-BIST Zephyr sample.
 *
 * Waits for a BIST_MSG_READY handshake from the HP core, then runs
 * BIST post-boot tests once, followed by periodic runtime tests.
 * Results are sent to the HP core over the mailbox as a bitmask.
 *
 * Post-boot: CPU reg, RAM March-X, Flash CRC
 * Runtime:   CPU reg, RAM March-A
 *
 * Bitmask layout (bit set = passed):
 *   bit 0  – CPU register test
 *   bit 1  – CPU CSR register test
 *   bit 2  – RAM March-A test       (runtime only)
 *   bit 3  – RAM March-X test       (post-boot only)
 *   bit 4  – Flash CRC test         (post-boot only)
 *   bit 30 – runtime flag (set when this is a runtime round)
 *   bit 31 – "all tests done" sentinel
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>
#include <bist_esp.h>
#include <string.h>

#define BIST_BIT_CPU_REG    BIT(0)
#define BIST_BIT_CPU_CSR    BIT(1)
#define BIST_BIT_RAM_A      BIT(2)
#define BIST_BIT_RAM_X      BIT(3)
#define BIST_BIT_FLASH      BIT(4)
#define BIST_BIT_STACK      BIT(5)
#define BIST_BIT_RUNTIME    BIT(30)
#define BIST_BIT_DONE       BIT(31)

#define BIST_MSG_READY      0xCAFECAFE

static volatile bool hp_ready;

static void rx_cb(const struct device *dev, mbox_channel_id_t channel_id,
		  void *user_data, struct mbox_msg *data)
{
	uint32_t value = 0;

	memcpy(&value, data->data, MIN(data->size, sizeof(value)));

	if (value == BIST_MSG_READY) {
		hp_ready = true;
	}
}

static uint32_t run_postboot_tests(void)
{
	uint32_t mask = 0;
	bist_esp_err_t err;

#if IS_ENABLED(CONFIG_ESP_BIST_CPU_REG_TEST)
	printk("[LP BIST] CPU reg test... ");
	err = bist_cpu_regs_test();
	printk("%s (%d)\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
	if (err == BIST_ESP_OK) {
		mask |= BIST_BIT_CPU_REG;
	}
#endif

#if IS_ENABLED(CONFIG_ESP_BIST_CPU_CSR_REG_TEST)
	printk("[LP BIST] CPU CSR test... ");
	err = bist_cpu_csr_regs_test();
	printk("%s (%d)\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
	if (err == BIST_ESP_OK) {
		mask |= BIST_BIT_CPU_CSR;
	}
#endif

#if IS_ENABLED(CONFIG_ESP_BIST_MEMORY_RAM_TEST)
	printk("[LP BIST] RAM March-X test... ");
	err = bist_ram_test_march_x();
	printk("%s (%d)\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
	if (err == BIST_ESP_OK) {
		mask |= BIST_BIT_RAM_X;
	}
#endif

#if IS_ENABLED(CONFIG_ESP_BIST_MEMORY_FLASH_TEST)
	printk("[LP BIST] Flash CRC test... ");
	err = bist_flash_test();
	printk("%s (%d)\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
	if (err == BIST_ESP_OK) {
		mask |= BIST_BIT_FLASH;
	}
#endif

	return mask;
}

static uint32_t run_runtime_tests(void)
{
	uint32_t mask = 0;
	bist_esp_err_t err;

#if IS_ENABLED(CONFIG_ESP_BIST_CPU_REG_TEST)
	printk("[LP BIST] CPU reg test... ");
	err = bist_cpu_regs_test();
	printk("%s (%d)\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
	if (err == BIST_ESP_OK) {
		mask |= BIST_BIT_CPU_REG;
	}
#endif

#if IS_ENABLED(CONFIG_ESP_BIST_MEMORY_RAM_TEST)
	printk("[LP BIST] RAM March-A test... ");
	err = bist_ram_test_march_a();
	printk("%s (%d)\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
	if (err == BIST_ESP_OK) {
		mask |= BIST_BIT_RAM_A;
	}
#endif

#if IS_ENABLED(CONFIG_ESP_BIST_STACK_TEST)
	printk("[LP BIST] Stack overflow check... ");
	err = bist_cpu_stack_overflow_check();
	printk("%s (%d)\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
	if (err == BIST_ESP_OK) {
		mask |= BIST_BIT_STACK;
	}
#endif

	return mask;
}

static void send_result(const struct mbox_dt_spec *tx, uint32_t result)
{
	struct mbox_msg msg = {
		.data = &result,
		.size = sizeof(result),
	};

	mbox_send_dt(tx, &msg);
}

int main(void)
{
	const struct mbox_dt_spec tx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), tx);
	const struct mbox_dt_spec rx_channel =
		MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), rx);
	uint32_t result;

	mbox_register_callback_dt(&rx_channel, rx_cb, NULL);
	mbox_set_enabled_dt(&rx_channel, 1);

#if IS_ENABLED(CONFIG_ESP_BIST_STACK_TEST)
	bist_cpu_stack_overflow_init();
#endif

	printk("[LP BIST] Waiting for HP core ready signal...\n");
	while (!hp_ready) {
		k_msleep(10);
	}
	printk("[LP BIST] HP core ready, starting tests\n");

	printk("[LP BIST] === Post-boot tests ===\n");
	result = run_postboot_tests() | BIST_BIT_DONE;
	printk("[LP BIST] Post-boot result: 0x%08x\n", result);
	send_result(&tx_channel, result);

	printk("[LP BIST] === Runtime tests (periodic) ===\n");
	while (1) {
		k_msleep(CONFIG_ESP_BIST_RUNTIME_TEST_INTERVAL_MS);
		result = run_runtime_tests() | BIST_BIT_RUNTIME | BIST_BIT_DONE;
		send_result(&tx_channel, result);
	}

	return 0;
}

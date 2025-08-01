/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>
#include <bist_esp.h>
#include <ulp_lp_core_interrupts.h>

static void callback(const struct device *dev, mbox_channel_id_t channel_id,
		     void *user_data, struct mbox_msg *data)
{
	printf("Pong (on channel %d)\n", channel_id);
}

static void fail_safe_exit(void)
{
	printf("Fail safe exit\n");
	while (1)
	;
}

static void runPOST(void)
{
	bist_esp_err_t test_err = BIST_ESP_OK;

	// Disable interupts
	ulp_lp_core_intr_disable();

	test_err = bist_cpu_regs_test();
	if (test_err == BIST_ESP_CPU_TEST_ERR) {
		printf("CPU register test failed\n");
		fail_safe_exit();
	}

	test_err = bist_cpu_csr_regs_test();
	if (test_err == BIST_ESP_CPU_CSR_TEST_ERR) {
		printf("CPU CSR register test failed\n");
		fail_safe_exit();
	}

	test_err = bist_ram_test_march_x();
	if (test_err == BIST_ESP_RAM_TEST_ERR) {
		printf("RAM test failed\n");
		fail_safe_exit();
	}

	// Enable interrupts
	ulp_lp_core_intr_enable();

	printf("All POST tests passed!\n");
}

static void runtime_tests(void)
{
	bist_esp_err_t test_err = BIST_ESP_OK;

	// Disable interupts
	ulp_lp_core_intr_disable();

	test_err = bist_cpu_regs_test();
	if (test_err == BIST_ESP_CPU_TEST_ERR) {
		printf("CPU register test failed\n");
		fail_safe_exit();
	}

	test_err = bist_cpu_csr_regs_test();
	if (test_err == BIST_ESP_CPU_CSR_TEST_ERR) {
		printf("CPU CSR register test failed\n");
		fail_safe_exit();
	}

	// Enable interrupts
	ulp_lp_core_intr_enable();

	printf("All RUNTIME tests passed!\n");

}

int main(void)
{
	int ret;
	printf("Hello from REMOTE - %s\n", CONFIG_BOARD_TARGET);

	runPOST();

	const struct mbox_dt_spec rx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), rx);

	printf("Maximum RX channels: %d\n", mbox_max_channels_get_dt(&rx_channel));

	ret = mbox_register_callback_dt(&rx_channel, callback, NULL);
	if (ret < 0) {
		printf("Could not register callback (%d)\n", ret);
		return 0;
	}

	ret = mbox_set_enabled_dt(&rx_channel, true);
	if (ret < 0) {
		printf("Could not enable RX channel %d (%d)\n", rx_channel.channel_id, ret);
		return 0;
	}

	const struct mbox_dt_spec tx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), tx);

	printf("Maximum bytes of data in the TX message: %d\n", mbox_mtu_get_dt(&tx_channel));
	printf("Maximum TX channels: %d\n", mbox_max_channels_get_dt(&tx_channel));

	while (1) {

		k_busy_wait(3000000);

		printf("Ping (on channel %d)\n", tx_channel.channel_id);

		ret = mbox_send_dt(&tx_channel, NULL);
		if (ret < 0) {
			printf("Could not send (%d)\n", ret);
			return 0;
		}
		runtime_tests();
	}
	return 0;
}

/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * LP-core side of the ESP-BIST NuttX sample.
 *
 * Waits for a BIST_MSG_READY handshake from the HP core over the LP
 * mailbox, then runs BIST post-boot tests once, followed by periodic
 * runtime tests. Results are sent to the HP core as bitmask messages.
 */

#include "nuttx/config.h"

#include <stdint.h>
#include "ulp_lp_core_utils.h"
#include "ulp_lp_core_print.h"
#include "ulp_lp_core_mailbox.h"
#include "esp_err.h"
#include "bist_esp.h"
#include "bist_log.h"
#include "lp_wdt.h"
#include "bist_protocol.h"
#include "ulp_lp_core_print.h"
#include "ulp_lp_core_uart.h"

#ifndef CONFIG_ESP_BIST_RUNTIME_TEST_INTERVAL_US
#  define CONFIG_ESP_BIST_RUNTIME_TEST_INTERVAL_US 10000
#endif

/* Mailbox timeout is in CPU cycles. (Assuming 40MHz CPU frequency for ~1s) */

#define MBOX_TIMEOUT_CYCLES (10 * 40000000)

static const char *TAG = "ulp_nuttx_bist";

static lp_mailbox_t mailbox;

void handle_stack_overflow(void)
{
  ESP_LOGE(TAG, "Stack overflow detected\r\n");
}

/* NuttX /dev/lp_mailbox transfers one byte per message; pack uint32 LE. */

static int mb_recv_u32(uint32_t *value)
{
  uint8_t bytes[4];
  int i;

  for (i = 0; i < 4; i++)
    {
      lp_message_t msg;
      if (lp_core_mailbox_receive(mailbox, &msg, MBOX_TIMEOUT_CYCLES) != ESP_OK)
        {
          return ESP_FAIL;
        }

      bytes[i] = (uint8_t)msg;
    }

  *value = ((uint32_t)bytes[0]) |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
  return 0;
}

static int mb_send_u32(uint32_t value)
{
  uint8_t bytes[4] =
    {
      (uint8_t)(value),
      (uint8_t)(value >> 8),
      (uint8_t)(value >> 16),
      (uint8_t)(value >> 24),
    };
  int i;

  for (i = 0; i < 4; i++)
    {
      if (lp_core_mailbox_send(mailbox, bytes[i], MBOX_TIMEOUT_CYCLES) != ESP_OK)
        {
          return ESP_FAIL;
        }
    }

  return 0;
}

static uint32_t run_postboot_tests(void)
{
  uint32_t mask = 0;
  bist_esp_err_t err;

#ifdef CONFIG_ESP_BIST_CPU_REG_TEST
  ESP_LOGI(TAG, "CPU reg test... ");
  err = bist_cpu_regs_test();
  ESP_LOGI(TAG, "%s (%d)\r\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
  if (err == BIST_ESP_OK)
    {
      mask |= BIST_BIT_CPU_REG;
    }
#endif

#ifdef CONFIG_ESP_BIST_CPU_CSR_REG_TEST
  ESP_LOGI(TAG, "CPU CSR test... ");
  err = bist_cpu_csr_regs_test();
  ESP_LOGI(TAG, "%s (%d)\r\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
  if (err == BIST_ESP_OK)
    {
      mask |= BIST_BIT_CPU_CSR;
    }
#endif

#ifdef CONFIG_ESP_BIST_MEMORY_RAM_TEST
  ESP_LOGI(TAG, "RAM March-X test... ");
  err = bist_ram_test_march_x();
  ESP_LOGI(TAG, "%s (%d)\r\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
  if (err == BIST_ESP_OK)
    {
      mask |= BIST_BIT_RAM_X;
    }
#endif

#ifdef CONFIG_ESP_BIST_MEMORY_FLASH_TEST
  ESP_LOGI(TAG, "Flash CRC test... ");
  err = bist_flash_test();
  ESP_LOGI(TAG, "%s (%d)\r\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
  if (err == BIST_ESP_OK)
    {
      mask |= BIST_BIT_FLASH;
    }
#endif

  return mask;
}

static uint32_t run_runtime_tests(void)
{
  uint32_t mask = 0;
  bist_esp_err_t err;

#ifdef CONFIG_ESP_BIST_CPU_REG_TEST
  ESP_LOGI(TAG, "CPU reg test... ");
  err = bist_cpu_regs_test();
  ESP_LOGI(TAG, "%s (%d)\r\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
  if (err == BIST_ESP_OK)
    {
      mask |= BIST_BIT_CPU_REG;
    }
#endif

#ifdef CONFIG_ESP_BIST_CPU_CSR_REG_TEST
  ESP_LOGI(TAG, "CPU CSR test... ");
  err = bist_cpu_csr_regs_test();
  ESP_LOGI(TAG, "%s (%d)\r\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
  if (err == BIST_ESP_OK)
    {
      mask |= BIST_BIT_CPU_CSR;
    }
#endif

#ifdef CONFIG_ESP_BIST_MEMORY_RAM_TEST
  ESP_LOGI(TAG, "RAM March-A test... ");
  err = bist_ram_test_march_a();
  ESP_LOGI(TAG, "%s (%d)\r\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
  if (err == BIST_ESP_OK)
    {
      mask |= BIST_BIT_RAM_A;
    }
#endif

#ifdef CONFIG_ESP_BIST_STACK_TEST
  ESP_LOGI(TAG, "Stack overflow check... ");
  err = bist_cpu_stack_overflow_check();
  ESP_LOGI(TAG, "%s (%d)\r\n", err == BIST_ESP_OK ? "PASS" : "FAIL", err);
  if (err == BIST_ESP_OK)
    {
      mask |= BIST_BIT_STACK;
    }
#endif

  return mask;
}

int main(void)
{
  uint32_t result;
  uint32_t ready = 0;
  esp_err_t err;

  ESP_LOGI(TAG, "Starting ULP BIST sample\r\n");

  /* Software mailbox requires LP init before HP; do this first. */

  err = lp_core_mailbox_init(&mailbox, NULL);
  if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Mailbox init failed: %d\r\n", err);
      return 0;
    }

  bist_cpu_stack_overflow_init();

  ESP_LOGI(TAG, "Waiting for HP core ready signal...\r\n");
  if (mb_recv_u32(&ready) != ESP_OK || ready != BIST_MSG_READY)
    {
      ESP_LOGE(TAG, "Ready handshake failed: msg=0x%08x\r\n",
               (unsigned)ready);
      return 0;
    }

  ESP_LOGI(TAG, "HP core ready, starting tests\r\n");

  ESP_LOGI(TAG, "=== Post-boot tests ===\r\n");
  result = run_postboot_tests() | BIST_BIT_POSTBOOT;
  ESP_LOGI(TAG, "Post-boot result: 0x%08x\r\n", result);
  if (mb_send_u32(result) != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to send post-boot result\r\n");
      return 0;
    }

  lp_wdt_init(CONFIG_ESP_BIST_WDT_TIMEOUT_US);

  ESP_LOGI(TAG, "=== Runtime tests (periodic) ===\r\n");
  while (1)
    {
      ulp_lp_core_delay_us(CONFIG_ESP_BIST_RUNTIME_TEST_INTERVAL_US);
      lp_wdt_feed();
      result = run_runtime_tests() | BIST_BIT_RUNTIME;
      if (mb_send_u32(result) != ESP_OK)
        {
          ESP_LOGE(TAG, "Failed to send runtime result\r\n");
        }
    }

  return 0;
}

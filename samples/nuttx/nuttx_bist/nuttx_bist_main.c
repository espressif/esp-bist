/****************************************************************************
 * samples/nuttx/nuttx_bist/nuttx_bist_main.c
 *
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 *
 * HP-core side of the ESP-BIST NuttX sample.
 *
 * Loads the LP companion firmware and starts the Host Diagnostic Agent.
 * The companion runs LP BIST and reports status bitmasks; the agent
 * queues them for this app to print. With CONFIG_ESP_BIST_HD_AUDIT_QA,
 * the companion also issues Q&A challenges that the agent answers on
 * the worker path.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "bist_hd_agent.h"
#include "bist_hd_protocol.h"
#include "ulp/ulp/ulp_code.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAILBOX_TIMEOUT_MS  10000
#define RUNTIME_LOOPS       10

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void print_postboot_results(uint32_t result)
{
  printf("=== Post-boot BIST results ===\n");
  printf("test_BIST_cpu_reg:%s\n",
         (result & BIST_HD_BIT_CPU_REG) ? "PASS" : "FAIL");
  printf("test_BIST_cpu_csr:%s\n",
         (result & BIST_HD_BIT_CPU_CSR) ? "PASS" : "FAIL");
  printf("test_BIST_ram_march_x:%s\n",
         (result & BIST_HD_BIT_RAM_X) ? "PASS" : "FAIL");
  printf("test_BIST_ram_abraham:%s\n",
         (result & BIST_HD_BIT_ABRAHAM) ? "PASS" : "FAIL");
  printf("test_BIST_flash_crc:%s\n",
         (result & BIST_HD_BIT_FLASH) ? "PASS" : "FAIL");
}

static void print_runtime_results(uint32_t result)
{
  printf("=== Runtime BIST results ===\n");
  printf("test_BIST_runtime_cpu_reg:%s\n",
         (result & BIST_HD_BIT_CPU_REG) ? "PASS" : "FAIL");
  printf("test_BIST_runtime_cpu_csr:%s\n",
         (result & BIST_HD_BIT_CPU_CSR) ? "PASS" : "FAIL");
  printf("test_BIST_runtime_ram_march_a:%s\n",
         (result & BIST_HD_BIT_RAM_A) ? "PASS" : "FAIL");
  printf("test_BIST_runtime_ram_abraham:%s\n",
         (result & BIST_HD_BIT_ABRAHAM) ? "PASS" : "FAIL");
  printf("test_BIST_runtime_stack_check:%s\n",
         (result & BIST_HD_BIT_STACK) ? "PASS" : "FAIL");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  uint32_t status;
  bool all_pass = true;
  bool runtime_ok = true;
  int ulp_fd;
  int lp_uart_fd;
  int err;

  UNUSED(argc);
  UNUSED(argv);

  lp_uart_fd = open("/dev/ttyS1", O_WRONLY);
  if (lp_uart_fd < 0)
    {
      printf("Failed to open LP-UART: %d\n", errno);
      return -1;
    }

  if (write(lp_uart_fd, "\n", 1) < 0)
    {
      printf("Failed to initialize LP-UART: %d\n", errno);
      return -1;
    }

  ulp_fd = open("/dev/ulp", O_WRONLY);
  if (ulp_fd < 0)
    {
      printf("Failed to open ULP: %d\n", errno);
      return -1;
    }

  if (write(ulp_fd, nuttx_bist_bin, nuttx_bist_bin_len) < 0)
    {
      printf("Failed to load ULP binary: %d\n", errno);
      close(ulp_fd);
      return -1;
    }

  close(ulp_fd);
  printf("LP core loaded with BIST firmware\n");

  /* Allow LP core to initialize the SW mailbox context. */

  sleep(1);

  err = bist_hd_agent_start();
  if (err != 0)
    {
      printf("Failed to start Host Diagnostic Agent: %d\n", err);
      printf("test_HD_agent_ready:FAIL\n");
      return -1;
    }

  printf("Host Diagnostic Agent started\n");
  printf("test_HD_agent_ready:PASS\n");

  err = bist_hd_agent_wait_lp_status(&status,
                                      BIST_HD_BIT_POSTBOOT,
                                      MAILBOX_TIMEOUT_MS);
  if (err == 0)
    {
      print_postboot_results(status);
    }
  else
    {
      printf("test_BIST_postboot:TIMEOUT\n");
      all_pass = false;
    }

  for (int i = 1; i <= RUNTIME_LOOPS; i++)
    {
      err = bist_hd_agent_wait_lp_status(&status,
                                          BIST_HD_BIT_RUNTIME,
                                          MAILBOX_TIMEOUT_MS);
      if (err != 0)
        {
          printf("test_BIST_runtime:TIMEOUT (loop %d)\n", i);
          all_pass = false;
          runtime_ok = false;
          break;
        }

      printf("--- Runtime loop %d/%d ---\n", i, RUNTIME_LOOPS);
      print_runtime_results(status);
      if ((status & BIST_HD_RUNTIME_ALL_PASS) != BIST_HD_RUNTIME_ALL_PASS)
        {
          all_pass = false;
        }
    }

#ifdef CONFIG_ESP_BIST_HD_AUDIT_QA
  /* Completing all runtime loops proves Q&A rounds passed; a runtime test
   * failure does not invalidate Q&A, so only loop completion matters. */

  if (runtime_ok)
    {
      printf("test_HD_challenge:PASS\n");
    }
  else
    {
      printf("test_HD_challenge:FAIL\n");
    }
#endif

  printf("BIST_RESULT:%s\n", all_pass ? "PASS" : "FAIL");

  /* Agent task keeps receiving companion messages. Park main. */

  while (1)
    {
      sleep(1);
    }

  return all_pass ? 0 : 1;
}

/****************************************************************************
 * samples/nuttx/nuttx_bist/nuttx_bist_main.c
 *
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 *
 * HP-core side of the ESP-BIST NuttX sample.
 *
 * Loads the LP core firmware, signals start over /dev/lp_mailbox, then
 * receives post-boot and runtime BIST result bitmasks from the LP core.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "bist_protocol.h"
#include "ulp/ulp/ulp_code.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RUNTIME_LOOPS 10

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void print_postboot_results(uint32_t result)
{
  printf("=== Post-boot BIST results ===\n");
  printf("test_BIST_cpu_reg:%s\n",
         (result & BIST_BIT_CPU_REG) ? "PASS" : "FAIL");
  printf("test_BIST_cpu_csr:%s\n",
         (result & BIST_BIT_CPU_CSR) ? "PASS" : "FAIL");
  printf("test_BIST_ram_march_x:%s\n",
         (result & BIST_BIT_RAM_X) ? "PASS" : "FAIL");
  printf("test_BIST_flash_crc:%s\n",
         (result & BIST_BIT_FLASH) ? "PASS" : "FAIL");
}

static void print_runtime_results(uint32_t result)
{
  printf("=== Runtime BIST results ===\n");
  printf("test_BIST_runtime_cpu_reg:%s\n",
         (result & BIST_BIT_CPU_REG) ? "PASS" : "FAIL");
  printf("test_BIST_runtime_cpu_csr:%s\n",
         (result & BIST_BIT_CPU_CSR) ? "PASS" : "FAIL");
  printf("test_BIST_runtime_ram_march_a:%s\n",
         (result & BIST_BIT_RAM_A) ? "PASS" : "FAIL");
  printf("test_BIST_runtime_stack_check:%s\n",
         (result & BIST_BIT_STACK) ? "PASS" : "FAIL");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ulp_fd;
  int mb_fd;
  int lp_uart_fd;
  uint32_t ready = BIST_MSG_READY;
  uint32_t postboot = 0;
  uint32_t runtime = 0;
  bool all_pass = true;
  int i;

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

  mb_fd = open("/dev/lp_mailbox", O_RDWR);
  if (mb_fd < 0)
    {
      printf("Failed to open LP Mailbox: %d\n", errno);
      return -1;
    }

  if (write(mb_fd, &ready, 4) != 4)
    {
      printf("Failed to signal HP ready: %d\n", errno);
      close(mb_fd);
      return -1;
    }

  if (read(mb_fd, &postboot, 4) == 4 && (postboot & BIST_BIT_POSTBOOT))
    {
      print_postboot_results(postboot);
      if ((postboot & BIST_POSTBOOT_ALL_PASS) != BIST_POSTBOOT_ALL_PASS)
        {
          all_pass = false;
        }
    }
  else
    {
      printf("test_BIST_postboot:FAIL (mailbox read)\n");
      all_pass = false;
    }

  for (i = 1; i <= RUNTIME_LOOPS; i++)
    {
      if (read(mb_fd, &runtime, 4) != 4)
        {
          printf("test_BIST_runtime:FAIL (mailbox read, loop %d)\n", i);
          all_pass = false;
          break;
        }

      printf("--- Runtime loop %d/%d ---\n", i, RUNTIME_LOOPS);
      print_runtime_results(runtime);
      if ((runtime & BIST_RUNTIME_ALL_PASS) != BIST_RUNTIME_ALL_PASS)
        {
          all_pass = false;
        }
    }

  printf("BIST_RESULT:%s\n", all_pass ? "PASS" : "FAIL");

  /* Keep acknowledging LP runtime messages so synchronous LP send does not
   * block forever and trip the LP WDT.
   */

  while (1)
    {
      if (read(mb_fd, &runtime, 4) != 4)
        {
          break;
        }
    }

  close(mb_fd);
  return all_pass ? 0 : 1;
}

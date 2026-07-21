/****************************************************************************
 * samples/nuttx/nuttx_bist/nuttx_bist_main.c
 *
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 *
 * HP-core side of the ESP-BIST NuttX sample.
 *
 * Loads the LP core firmware and signals it to start BIST tests.
 * After the LP core completes each test phase, results are read from
 * ULP shared variables and printed on the main UART.
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
#include <sys/ioctl.h>

#include <nuttx/fs/ioctl.h>
#include <nuttx/symtab.h>

#include "bist_protocol.h"
#include "ulp/ulp/ulp_code.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define POLL_INTERVAL_US  100000
#define POLL_TIMEOUT_US   10000000
#define RUNTIME_LOOPS     10

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int ulp_read_u32(int fd, const char *name, uint32_t *value)
{
  struct symtab_s sym =
    {
      .sym_name = name,
      .sym_value = value,
    };

  return ioctl(fd, FIONREAD, &sym);
}

static int ulp_write_u32(int fd, const char *name, uint32_t value)
{
  struct symtab_s sym =
    {
      .sym_name = name,
      .sym_value = &value,
    };

  return ioctl(fd, FIONWRITE, &sym);
}

static int wait_for_postboot(int fd, uint32_t *result)
{
  int remaining_us = POLL_TIMEOUT_US;
  uint32_t value = 0;

  while (remaining_us > 0)
    {
      if (ulp_read_u32(fd, "nuttx_bist_postboot_result", &value) < 0)
        {
          return -1;
        }

      if (value & BIST_BIT_POSTBOOT)
        {
          *result = value;
          return 0;
        }

      usleep(POLL_INTERVAL_US);
      remaining_us -= POLL_INTERVAL_US;
    }

  return -1;
}

static int wait_for_runtime_count(int fd, uint32_t target)
{
  int remaining_us = POLL_TIMEOUT_US;
  uint32_t count = 0;

  while (remaining_us > 0)
    {
      if (ulp_read_u32(fd, "nuttx_bist_runtime_count", &count) < 0)
        {
          return -1;
        }

      if (count >= target)
        {
          return 0;
        }

      usleep(POLL_INTERVAL_US);
      remaining_us -= POLL_INTERVAL_US;
    }

  return -1;
}

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
  int fd;
  uint32_t postboot = 0;
  uint32_t runtime = 0;
  bool all_pass = true;
  int i;

  UNUSED(argc);
  UNUSED(argv);

  fd = open("/dev/ulp", O_RDWR);
  if (fd < 0)
    {
      printf("Failed to open ULP: %d\n", errno);
      return -1;
    }

  if (write(fd, nuttx_bist_bin, nuttx_bist_bin_len) < 0)
    {
      printf("Failed to load ULP binary: %d\n", errno);
      close(fd);
      return -1;
    }

  printf("LP core loaded with BIST firmware\n");

  /* Signal LP core to start BIST tests */

  if (ulp_write_u32(fd, "nuttx_bist_hp_ready", BIST_MSG_READY) < 0)
    {
      printf("Failed to signal HP ready: %d\n", errno);
      close(fd);
      return -1;
    }

  if (wait_for_postboot(fd, &postboot) == 0)
    {
      print_postboot_results(postboot);
      if ((postboot & BIST_POSTBOOT_ALL_PASS) != BIST_POSTBOOT_ALL_PASS)
        {
          all_pass = false;
        }
    }
  else
    {
      printf("test_BIST_postboot:TIMEOUT\n");
      all_pass = false;
    }

  for (i = 1; i <= RUNTIME_LOOPS; i++)
    {
      if (wait_for_runtime_count(fd, (uint32_t)i) != 0)
        {
          printf("test_BIST_runtime:TIMEOUT (loop %d)\n", i);
          all_pass = false;
          break;
        }

      if (ulp_read_u32(fd, "nuttx_bist_runtime_result", &runtime) < 0)
        {
          printf("Failed to read runtime result: %d\n", errno);
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
  close(fd);
  return all_pass ? 0 : 1;
}

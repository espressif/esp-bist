/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Shared protocol definitions between HP and LP cores.
 *
 * Bitmask layout (bit set = test passed):
 *   bit 0  - CPU register test
 *   bit 1  - CPU CSR register test
 *   bit 2  - RAM March-A test       (runtime)
 *   bit 3  - RAM March-X test       (post-boot)
 *   bit 4  - Flash CRC test         (post-boot)
 *   bit 5  - Stack overflow check   (runtime)
 *   bit 6  - RAM Abraham test       (post-boot full / runtime one-pair)
 *   bit 30 - runtime flag
 *   bit 31 - post-boot complete flag
 */

#pragma once

#include "esp_bit_defs.h"

#define BIST_BIT_CPU_REG  BIT(0)
#define BIST_BIT_CPU_CSR  BIT(1)
#define BIST_BIT_RAM_A    BIT(2)
#define BIST_BIT_RAM_X    BIT(3)
#define BIST_BIT_FLASH    BIT(4)
#define BIST_BIT_STACK    BIT(5)
#define BIST_BIT_ABRAHAM  BIT(6)
#define BIST_BIT_RUNTIME  BIT(30)
#define BIST_BIT_POSTBOOT BIT(31)

#define BIST_MSG_READY    0xCAFECAFE

#define BIST_POSTBOOT_ALL_PASS (BIST_BIT_CPU_REG | BIST_BIT_CPU_CSR | BIST_BIT_RAM_X | BIST_BIT_FLASH | BIST_BIT_ABRAHAM)
#define BIST_RUNTIME_ALL_PASS  (BIST_BIT_CPU_REG | BIST_BIT_CPU_CSR | BIST_BIT_RAM_A | BIST_BIT_STACK | BIST_BIT_ABRAHAM)

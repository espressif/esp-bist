/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

/** Simplified memory map for the bootloader.
 *  Make sure the bootloader can load into main memory without overwriting itself.
 *
 *  ESP32-C3 ROM static data usage is as follows:
 *  - 0x3fccae00 - 0x3fcdc710: Shared buffers, used in UART/USB/SPI download mode only
 *  - 0x3fcdc710 - 0x3fcde710: PRO CPU stack, can be reclaimed as heap after RTOS startup
 *  - 0x3fcde710 - 0x3fce0000: ROM .bss and .data (not easily reclaimable)
 *
 *  The 2nd stage bootloader can take space up to the end of ROM shared
 *  buffers area (0x3fcdc710).
 */

/* The offset between Dbus and Ibus.
 * Used to convert between 0x403xxxxx and 0x3fcxxxxx addresses.
 */
#define IRAM_DRAM_OFFSET        0x700000
#define DRAM_BUFFERS_START      0x3fccae00
#define DRAM_STACK_START        0x3fcdc710
#define DRAM_ROM_BSS_DATA_START 0x3fcde710

/* Base address used for calculating memory layout
 * counted from Dbus backwards and back to the Ibus
 */
#define BOOTLOADER_USABLE_DRAM_END DRAM_BUFFERS_START

/* For safety margin between bootloader data section and startup stacks */
#define BOOTLOADER_STACK_OVERHEAD 0x0
/* These lengths can be adjusted, if necessary: */
#define BOOTLOADER_DRAM_SEG_LEN        0x9000
#define BOOTLOADER_IRAM_LOADER_SEG_LEN 0x3000
#define BOOTLOADER_IRAM_SEG_LEN        0x7000

/* Start of the lower region is determined by region size and the end of the higher region */
#define BOOTLOADER_DRAM_SEG_END          (BOOTLOADER_USABLE_DRAM_END + BOOTLOADER_STACK_OVERHEAD)
#define BOOTLOADER_DRAM_SEG_START        (BOOTLOADER_DRAM_SEG_END - BOOTLOADER_DRAM_SEG_LEN)
#define BOOTLOADER_IRAM_LOADER_SEG_START (BOOTLOADER_DRAM_SEG_START - BOOTLOADER_IRAM_LOADER_SEG_LEN + IRAM_DRAM_OFFSET)
#define BOOTLOADER_IRAM_SEG_START        (BOOTLOADER_IRAM_LOADER_SEG_START - BOOTLOADER_IRAM_SEG_LEN)

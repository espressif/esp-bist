/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * This file is part of Espressif's BIST (Built-In Self Test) Library.
 *
 * BIST library is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * BIST library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with BIST library. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include "bist_ram.h"

#define BIST_ESP_RAM_BACKUP_CHUNK_SIZE 256 // 1024 bytes

extern uint32_t _heap_start;
extern uint32_t _heap_size;

// Buffer to backup and restore 1024 bytes at a time
volatile uint32_t __attribute__((section(".dram0.safe_ram"))) backup_chunk[BIST_ESP_RAM_BACKUP_CHUNK_SIZE];

bist_esp_err_t bist_ram_test_march_a(void)
{
    bool test_passed = true;
    volatile uint32_t *start_addr = (uint32_t *)&_heap_start;
    volatile uint32_t dram_test_size = (uint32_t)&_heap_size;

    for (size_t offset = 0; offset < dram_test_size; offset += BIST_ESP_RAM_BACKUP_CHUNK_SIZE) {
        size_t current_chunk_size = ((offset + BIST_ESP_RAM_BACKUP_CHUNK_SIZE) > dram_test_size)
                                        ? (dram_test_size - offset)
                                        : BIST_ESP_RAM_BACKUP_CHUNK_SIZE;

        // Save the current chunk
        for (size_t i = 0; i < current_chunk_size; i++) {
            backup_chunk[i] = start_addr[offset + i];
        }

        // March X Test Sequence
        // 1. ↑ {W0} (Write 0 in ascending order)
        for (size_t i = 0; i < current_chunk_size; i++) {
            start_addr[offset + i] = 0;
        }

        // 2. ↑ {R0, W1} (Read 0, Write 1 in ascending order)
        ASM(" bist_ram_test_march_a_step2:");
        for (size_t i = 0; i < current_chunk_size; i++) {
            if (start_addr[offset + i] != 0) {
                test_passed = false;
                goto restore_original_chunk;
            }
            start_addr[offset + i] = 0xFFFFFFFF;
        }

        // 3. ↑ {R1} (Read 1 in ascending order)
        for (size_t i = 0; i < current_chunk_size; i++) {
            if (start_addr[offset + i] != 0xFFFFFFFF) {
                test_passed = false;
                goto restore_original_chunk;
            }
        }

    restore_original_chunk:
        // Restore the original values of the current chunk
        for (size_t i = 0; i < current_chunk_size; i++) {
            start_addr[offset + i] = backup_chunk[i];
        }

        if (!test_passed) {
            return BIST_ESP_RAM_TEST_ERR;
        }
    }

    return BIST_ESP_OK;
}

bist_esp_err_t bist_ram_test_march_x(void)
{
    bool test_passed = true;
    volatile uint32_t *start_addr = (uint32_t *)&_heap_start;
    volatile uint32_t dram_test_size = (uint32_t)&_heap_size;

    for (size_t offset = 0; offset < dram_test_size; offset += BIST_ESP_RAM_BACKUP_CHUNK_SIZE) {
        size_t current_chunk_size = ((offset + BIST_ESP_RAM_BACKUP_CHUNK_SIZE) > dram_test_size)
                                        ? (dram_test_size - offset)
                                        : BIST_ESP_RAM_BACKUP_CHUNK_SIZE;

        // Save the current chunk
        for (size_t i = 0; i < current_chunk_size; i++) {
            backup_chunk[i] = start_addr[offset + i];
        }

        // March X Test Sequence
        // 1. ↑ {W0} (Write 0 in ascending order)
        for (size_t i = 0; i < current_chunk_size; i++) {
            start_addr[offset + i] = 0;
        }

        // 2. ↑ {R0, W1} (Read 0, Write 1 in ascending order)
        ASM("bist_ram_test_march_x_step2:");
        for (size_t i = 0; i < current_chunk_size; i++) {
            if (start_addr[offset + i] != 0) {
                test_passed = false;
                goto restore_original_chunk;
            }
            start_addr[offset + i] = 0xFFFFFFFF; // Write 1
        }

        // 3. ↓ {R1, W0} (Read 1, Write 0 in descending order)
        for (size_t i = current_chunk_size; i != 0; i--) {
            if (start_addr[offset + i - 1] != 0xFFFFFFFF) {
                test_passed = false;
                goto restore_original_chunk;
            }
            start_addr[offset + i - 1] = 0; // Write 0
        }

        // 4. ↑ {R0, W1} (Read 0, Write 1 in ascending order)
        for (size_t i = 0; i < current_chunk_size; i++) {
            if (start_addr[offset + i] != 0) {
                test_passed = false;
                goto restore_original_chunk;
            }
            start_addr[offset + i] = 0xFFFFFFFF; // Write 1
        }

        // 5. ↓ {R1, W0} (Read 1, Write 0 in descending order)
        for (size_t i = current_chunk_size; i != 0; i--) {
            if (start_addr[offset + i - 1] != 0xFFFFFFFF) {
                test_passed = false;
                goto restore_original_chunk;
            }
            start_addr[offset + i - 1] = 0; // Write 0
        }

    restore_original_chunk:
        // Restore the original values of the current chunk
        for (size_t i = 0; i < current_chunk_size; i++) {
            start_addr[offset + i] = backup_chunk[i];
        }

        if (!test_passed) {
            return BIST_ESP_RAM_TEST_ERR;
        }
    }

    return BIST_ESP_OK;
}

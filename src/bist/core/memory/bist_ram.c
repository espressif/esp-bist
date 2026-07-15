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

#if defined(CONFIG_ESP_BIST_MEMORY_RAM_TEST)

#ifdef CONFIG_ESP_BIST_RAM_PARTITION_SIZE
#define BIST_ESP_RAM_BACKUP_CHUNK_SIZE CONFIG_ESP_BIST_RAM_PARTITION_SIZE
#else
#define BIST_ESP_RAM_BACKUP_CHUNK_SIZE 512
#endif

#define MARCH_STACK_SIZE 256

extern uint32_t _bist_ram_test_start;
extern uint32_t _bist_ram_test_size;

// Buffer to backup and restore two partitions (2 x 1024 bytes) for Abraham
volatile uint32_t __attribute__((section(".dram0.safe_ram"))) backup_chunk[2 * BIST_ESP_RAM_BACKUP_CHUNK_SIZE];

// Abraham time-division pair state (validated on each use; NOLOAD-safe)
static volatile size_t __attribute__((section(".dram0.safe_ram"))) abraham_pair_i;
static volatile size_t __attribute__((section(".dram0.safe_ram"))) abraham_pair_j;

// Stack for the RAM test
static uint8_t __attribute__((section(".dram0.safe_ram"), aligned(16)))
    ram_test_stack[MARCH_STACK_SIZE];

/*
 * Run fn() with SP relocated to ram_test_stack (.dram0.safe_ram).
 * safe_ram sits below _bist_ram_test_start, so the march algorithms can
 * test the entire linker-defined region — including the normal stack —
 * without ever corrupting their own frame.
 *
 * The caller's stack frames ARE inside the test region, but the chunk-based
 * backup/restore cycle saves them to backup_chunk before each test pass and
 * restores them afterwards, so they are intact when control returns here.
 */
static bist_esp_err_t run_on_safe_stack(bist_esp_err_t (*fn)(void))
{
    bist_esp_err_t result;
    uint8_t *safe_sp = ram_test_stack + MARCH_STACK_SIZE;

    __asm__ volatile(
        "mv   t0, sp\n"
        "mv   sp, %[stk]\n"
        "addi sp, sp, -8\n"
        "sw   t0, 4(sp)\n"
        "sw   ra, 0(sp)\n"
        "jalr ra, 0(%[func])\n"
        "mv   %[ret], a0\n"
        "lw   ra, 0(sp)\n"
        "lw   t0, 4(sp)\n"
        "mv   sp, t0\n"
        : [ret] "=r"(result)
        : [stk] "r"(safe_sp), [func] "r"(fn)
        : "t0", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
          "t1", "t2", "t3", "t4", "t5", "t6", "ra", "memory"
    );

    return result;
}

static bist_esp_err_t __attribute__((noinline)) march_a_impl(void)
{
    bool test_passed = true;
    volatile uint32_t *start_addr = (uint32_t *)&_bist_ram_test_start;
    volatile uint32_t dram_test_size = (uint32_t)&_bist_ram_test_size / 4;

    for (size_t offset = 0; offset < dram_test_size; offset += BIST_ESP_RAM_BACKUP_CHUNK_SIZE) {
        size_t current_chunk_size = ((offset + BIST_ESP_RAM_BACKUP_CHUNK_SIZE) > dram_test_size)
                                        ? (dram_test_size - offset)
                                        : BIST_ESP_RAM_BACKUP_CHUNK_SIZE;

        // Save the current chunk
        for (size_t i = 0; i < current_chunk_size; i++) {
            backup_chunk[i] = start_addr[offset + i];
        }

        // March A Test Sequence
        // 1. ↑ {W0} (Write 0 in ascending order)
        for (size_t i = 0; i < current_chunk_size; i++) {
            start_addr[offset + i] = 0;
        }

        // 2. ↑ {R0, W1} (Read 0, Write 1 in ascending order)
        ASM("bist_ram_test_march_a_step2:");
        for (size_t i = 0; i < current_chunk_size; i++) {
            if (start_addr[offset + i] != 0) {
                test_passed = false;
                goto restore_a;
            }
            start_addr[offset + i] = 0xFFFFFFFF;
        }

        // 3. ↑ {R1} (Read 1 in ascending order)
        for (size_t i = 0; i < current_chunk_size; i++) {
            if (start_addr[offset + i] != 0xFFFFFFFF) {
                test_passed = false;
                goto restore_a;
            }
        }

    restore_a:
        for (size_t i = 0; i < current_chunk_size; i++) {
            start_addr[offset + i] = backup_chunk[i];
        }

        if (!test_passed) {
            return BIST_ESP_RAM_TEST_ERR;
        }
    }

    return BIST_ESP_OK;
}

static bist_esp_err_t __attribute__((noinline)) march_x_impl(void)
{
    bool test_passed = true;
    volatile uint32_t *start_addr = (uint32_t *)&_bist_ram_test_start;
    volatile uint32_t dram_test_size = (uint32_t)&_bist_ram_test_size / 4;

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
                goto restore_x;
            }
            start_addr[offset + i] = 0xFFFFFFFF; // Write 1
        }

        // 3. ↓ {R1, W0} (Read 1, Write 0 in descending order)
        for (size_t i = current_chunk_size; i != 0; i--) {
            if (start_addr[offset + i - 1] != 0xFFFFFFFF) {
                test_passed = false;
                goto restore_x;
            }
            start_addr[offset + i - 1] = 0; // Write 0
        }

        // 4. ↑ {R0, W1} (Read 0, Write 1 in ascending order)
        for (size_t i = 0; i < current_chunk_size; i++) {
            if (start_addr[offset + i] != 0) {
                test_passed = false;
                goto restore_x;
            }
            start_addr[offset + i] = 0xFFFFFFFF; // Write 1
        }

        // 5. ↓ {R1, W0} (Read 1, Write 0 in descending order)
        for (size_t i = current_chunk_size; i != 0; i--) {
            if (start_addr[offset + i - 1] != 0xFFFFFFFF) {
                test_passed = false;
                goto restore_x;
            }
            start_addr[offset + i - 1] = 0; // Write 0
        }

    restore_x:
        for (size_t i = 0; i < current_chunk_size; i++) {
            start_addr[offset + i] = backup_chunk[i];
        }

        if (!test_passed) {
            return BIST_ESP_RAM_TEST_ERR;
        }
    }

    return BIST_ESP_OK;
}

bist_esp_err_t bist_ram_test_march_a(void)
{
    return run_on_safe_stack(march_a_impl);
}

bist_esp_err_t bist_ram_test_march_x(void)
{
    return run_on_safe_stack(march_x_impl);
}

/*
 * Abraham algorithm (IEC 60730-1 Annex H H.2.19.1) — WOM variant.
 *
 * Operates on the logical concatenation of two partitions (part_a ∪ part_b).
 * Ascending traversal: part_a[0..size_a), then part_b[0..size_b).
 * Descending traversal: part_b[size_b-1..0], then part_a[size_a-1..0].
 *
 * Sequence (10 elements, 30 ops/cell):
 *   ↕(w0)
 *   ↓(r0,w1) ↑(r1)       — Seq 1
 *   ↓(r1,w0) ↑(r0)       — Seq 2
 *   ↑(r0,w1) ↓(r1)       — Seq 3
 *   ↑(r1,w0) ↓(r0)       — Seq 4
 *   ↓(r0,w1,w0) ↑(r0)    — Seq 5
 *   ↑(r0,w1,w0) ↑(r0)    — Seq 6
 *   ↕(w1)
 *   ↑(r1,w0,w1) ↑(r1)    — Seq 7
 *   ↓(r1,w0,w1) ↑(r1)    — Seq 8
 */
static bist_esp_err_t __attribute__((noinline)) abraham_impl(void)
{
    bool test_passed = true;
    volatile uint32_t *start_addr = (uint32_t *)&_bist_ram_test_start;
    volatile uint32_t dram_test_size = (uint32_t)&_bist_ram_test_size / 4;

    size_t num_partitions = (dram_test_size + BIST_ESP_RAM_BACKUP_CHUNK_SIZE - 1)
                            / BIST_ESP_RAM_BACKUP_CHUNK_SIZE;

    // Determine partition pointers and sizes
    volatile uint32_t *part_a;
    volatile uint32_t *part_b;
    size_t size_a, size_b;

    if (num_partitions <= 1) {
        part_a = start_addr;
        size_a = dram_test_size;
        part_b = start_addr; // unused
        size_b = 0;
    } else {
        // Validate pair state against NOLOAD garbage
        if (abraham_pair_i >= num_partitions - 1 ||
            abraham_pair_j >= num_partitions ||
            abraham_pair_j <= abraham_pair_i) {
            abraham_pair_i = 0;
            abraham_pair_j = 1;
        }

        size_t off_a = abraham_pair_i * BIST_ESP_RAM_BACKUP_CHUNK_SIZE;
        size_t off_b = abraham_pair_j * BIST_ESP_RAM_BACKUP_CHUNK_SIZE;

        size_a = ((off_a + BIST_ESP_RAM_BACKUP_CHUNK_SIZE) > dram_test_size)
                     ? (dram_test_size - off_a)
                     : BIST_ESP_RAM_BACKUP_CHUNK_SIZE;
        size_b = ((off_b + BIST_ESP_RAM_BACKUP_CHUNK_SIZE) > dram_test_size)
                     ? (dram_test_size - off_b)
                     : BIST_ESP_RAM_BACKUP_CHUNK_SIZE;

        part_a = &start_addr[off_a];
        part_b = &start_addr[off_b];
    }

    // Backup partition A
    for (size_t i = 0; i < size_a; i++) {
        backup_chunk[i] = part_a[i];
    }
    // Backup partition B
    for (size_t i = 0; i < size_b; i++) {
        backup_chunk[BIST_ESP_RAM_BACKUP_CHUNK_SIZE + i] = part_b[i];
    }

    // --- ↕(w0): Initialize all cells to 0 ---
    for (size_t i = 0; i < size_a; i++) part_a[i] = 0;
    for (size_t i = 0; i < size_b; i++) part_b[i] = 0;

    // --- Seq 1: ↓(r0,w1) ↑(r1) ---
    ASM("bist_ram_test_abraham_seq1:");
    // ↓(r0,w1)
    for (size_t i = size_b; i != 0; i--) {
        if (part_b[i - 1] != 0) { test_passed = false; goto restore_abraham; }
        part_b[i - 1] = 0xFFFFFFFF;
    }
    for (size_t i = size_a; i != 0; i--) {
        if (part_a[i - 1] != 0) { test_passed = false; goto restore_abraham; }
        part_a[i - 1] = 0xFFFFFFFF;
    }
    // ↑(r1)
    for (size_t i = 0; i < size_a; i++) {
        if (part_a[i] != 0xFFFFFFFF) { test_passed = false; goto restore_abraham; }
    }
    for (size_t i = 0; i < size_b; i++) {
        if (part_b[i] != 0xFFFFFFFF) { test_passed = false; goto restore_abraham; }
    }

    // --- Seq 2: ↓(r1,w0) ↑(r0) ---
    // ↓(r1,w0)
    for (size_t i = size_b; i != 0; i--) {
        if (part_b[i - 1] != 0xFFFFFFFF) { test_passed = false; goto restore_abraham; }
        part_b[i - 1] = 0;
    }
    for (size_t i = size_a; i != 0; i--) {
        if (part_a[i - 1] != 0xFFFFFFFF) { test_passed = false; goto restore_abraham; }
        part_a[i - 1] = 0;
    }
    // ↑(r0)
    for (size_t i = 0; i < size_a; i++) {
        if (part_a[i] != 0) { test_passed = false; goto restore_abraham; }
    }
    for (size_t i = 0; i < size_b; i++) {
        if (part_b[i] != 0) { test_passed = false; goto restore_abraham; }
    }

    // --- Seq 3: ↑(r0,w1) ↓(r1) ---
    // ↑(r0,w1)
    for (size_t i = 0; i < size_a; i++) {
        if (part_a[i] != 0) { test_passed = false; goto restore_abraham; }
        part_a[i] = 0xFFFFFFFF;
    }
    for (size_t i = 0; i < size_b; i++) {
        if (part_b[i] != 0) { test_passed = false; goto restore_abraham; }
        part_b[i] = 0xFFFFFFFF;
    }
    // ↓(r1)
    for (size_t i = size_b; i != 0; i--) {
        if (part_b[i - 1] != 0xFFFFFFFF) { test_passed = false; goto restore_abraham; }
    }
    for (size_t i = size_a; i != 0; i--) {
        if (part_a[i - 1] != 0xFFFFFFFF) { test_passed = false; goto restore_abraham; }
    }

    // --- Seq 4: ↑(r1,w0) ↓(r0) ---
    // ↑(r1,w0)
    for (size_t i = 0; i < size_a; i++) {
        if (part_a[i] != 0xFFFFFFFF) { test_passed = false; goto restore_abraham; }
        part_a[i] = 0;
    }
    for (size_t i = 0; i < size_b; i++) {
        if (part_b[i] != 0xFFFFFFFF) { test_passed = false; goto restore_abraham; }
        part_b[i] = 0;
    }
    // ↓(r0)
    for (size_t i = size_b; i != 0; i--) {
        if (part_b[i - 1] != 0) { test_passed = false; goto restore_abraham; }
    }
    for (size_t i = size_a; i != 0; i--) {
        if (part_a[i - 1] != 0) { test_passed = false; goto restore_abraham; }
    }

    // --- Seq 5: ↓(r0,w1,w0) ↑(r0) ---
    // ↓(r0,w1,w0)
    for (size_t i = size_b; i != 0; i--) {
        if (part_b[i - 1] != 0) { test_passed = false; goto restore_abraham; }
        part_b[i - 1] = 0xFFFFFFFF;
        part_b[i - 1] = 0;
    }
    for (size_t i = size_a; i != 0; i--) {
        if (part_a[i - 1] != 0) { test_passed = false; goto restore_abraham; }
        part_a[i - 1] = 0xFFFFFFFF;
        part_a[i - 1] = 0;
    }
    // ↑(r0)
    for (size_t i = 0; i < size_a; i++) {
        if (part_a[i] != 0) { test_passed = false; goto restore_abraham; }
    }
    for (size_t i = 0; i < size_b; i++) {
        if (part_b[i] != 0) { test_passed = false; goto restore_abraham; }
    }

    // --- Seq 6: ↑(r0,w1,w0) ↑(r0) ---
    // ↑(r0,w1,w0)
    for (size_t i = 0; i < size_a; i++) {
        if (part_a[i] != 0) { test_passed = false; goto restore_abraham; }
        part_a[i] = 0xFFFFFFFF;
        part_a[i] = 0;
    }
    for (size_t i = 0; i < size_b; i++) {
        if (part_b[i] != 0) { test_passed = false; goto restore_abraham; }
        part_b[i] = 0xFFFFFFFF;
        part_b[i] = 0;
    }
    // ↑(r0)
    for (size_t i = 0; i < size_a; i++) {
        if (part_a[i] != 0) { test_passed = false; goto restore_abraham; }
    }
    for (size_t i = 0; i < size_b; i++) {
        if (part_b[i] != 0) { test_passed = false; goto restore_abraham; }
    }

    // --- ↕(w1): Reset all cells to 1 ---
    for (size_t i = 0; i < size_a; i++) part_a[i] = 0xFFFFFFFF;
    for (size_t i = 0; i < size_b; i++) part_b[i] = 0xFFFFFFFF;

    // --- Seq 7: ↑(r1,w0,w1) ↑(r1) ---
    // ↑(r1,w0,w1)
    for (size_t i = 0; i < size_a; i++) {
        if (part_a[i] != 0xFFFFFFFF) { test_passed = false; goto restore_abraham; }
        part_a[i] = 0;
        part_a[i] = 0xFFFFFFFF;
    }
    for (size_t i = 0; i < size_b; i++) {
        if (part_b[i] != 0xFFFFFFFF) { test_passed = false; goto restore_abraham; }
        part_b[i] = 0;
        part_b[i] = 0xFFFFFFFF;
    }
    // ↑(r1)
    for (size_t i = 0; i < size_a; i++) {
        if (part_a[i] != 0xFFFFFFFF) { test_passed = false; goto restore_abraham; }
    }
    for (size_t i = 0; i < size_b; i++) {
        if (part_b[i] != 0xFFFFFFFF) { test_passed = false; goto restore_abraham; }
    }

    // --- Seq 8: ↓(r1,w0,w1) ↑(r1) ---
    // ↓(r1,w0,w1)
    for (size_t i = size_b; i != 0; i--) {
        if (part_b[i - 1] != 0xFFFFFFFF) { test_passed = false; goto restore_abraham; }
        part_b[i - 1] = 0;
        part_b[i - 1] = 0xFFFFFFFF;
    }
    for (size_t i = size_a; i != 0; i--) {
        if (part_a[i - 1] != 0xFFFFFFFF) { test_passed = false; goto restore_abraham; }
        part_a[i - 1] = 0;
        part_a[i - 1] = 0xFFFFFFFF;
    }
    // ↑(r1)
    for (size_t i = 0; i < size_a; i++) {
        if (part_a[i] != 0xFFFFFFFF) { test_passed = false; goto restore_abraham; }
    }
    for (size_t i = 0; i < size_b; i++) {
        if (part_b[i] != 0xFFFFFFFF) { test_passed = false; goto restore_abraham; }
    }

restore_abraham:
    // Restore partition A
    for (size_t i = 0; i < size_a; i++) {
        part_a[i] = backup_chunk[i];
    }
    // Restore partition B
    for (size_t i = 0; i < size_b; i++) {
        part_b[i] = backup_chunk[BIST_ESP_RAM_BACKUP_CHUNK_SIZE + i];
    }

    if (!test_passed) {
        return BIST_ESP_RAM_TEST_ERR;
    }
    return BIST_ESP_OK;
}

static void abraham_advance_pair(size_t num_partitions)
{
    if (num_partitions <= 1) {
        return;
    }
    abraham_pair_j++;
    if (abraham_pair_j >= num_partitions) {
        abraham_pair_i++;
        abraham_pair_j = abraham_pair_i + 1;
    }
    if (abraham_pair_i >= num_partitions - 1) {
        abraham_pair_i = 0;
        abraham_pair_j = 1;
    }
}

bist_esp_err_t bist_ram_test_abraham(void)
{
    bist_esp_err_t result = run_on_safe_stack(abraham_impl);
    volatile uint32_t dram_test_size = (uint32_t)&_bist_ram_test_size / 4;
    size_t num_partitions = (dram_test_size + BIST_ESP_RAM_BACKUP_CHUNK_SIZE - 1)
                            / BIST_ESP_RAM_BACKUP_CHUNK_SIZE;
    abraham_advance_pair(num_partitions);
    return result;
}

void bist_ram_test_abraham_reset(void)
{
    abraham_pair_i = 0;
    abraham_pair_j = 1;
}

bist_esp_err_t bist_ram_test_abraham_full(void)
{
    volatile uint32_t dram_test_size = (uint32_t)&_bist_ram_test_size / 4;
    size_t num_partitions = (dram_test_size + BIST_ESP_RAM_BACKUP_CHUNK_SIZE - 1)
                            / BIST_ESP_RAM_BACKUP_CHUNK_SIZE;
    size_t total_pairs = (num_partitions <= 1)
                             ? 1
                             : (num_partitions * (num_partitions - 1)) / 2;

    bist_ram_test_abraham_reset();

    for (size_t p = 0; p < total_pairs; p++) {
        bist_esp_err_t err = run_on_safe_stack(abraham_impl);
        if (err != BIST_ESP_OK) {
            return err;
        }
        abraham_advance_pair(num_partitions);
    }
    return BIST_ESP_OK;
}

#endif // CONFIG_ESP_BIST_MEMORY_RAM_TEST

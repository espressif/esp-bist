/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "sdkconfig.h"
#include "soc/soc.h"
#include "ld.common"

#if CONFIG_ESP_ROM_HAS_LP_ROM
/* With LP-ROM memory layout is different due to LP ROM stack/data */
/* For P4 ECO5 we also reserve some RTC MEM at the first for MSPI workaround */
#define ULP_MEM_START_ADDRESS SOC_RTC_DRAM_LOW + RESERVE_RTC_MEM + MSPI_WORKAROUND_SIZE
#else
#define ULP_MEM_START_ADDRESS (SOC_RTC_DRAM_LOW)
#endif

#define ALIGN_DOWN(SIZE, AL)   (SIZE & ~(AL - 1))
/* Ensure the end where the shared memory starts is aligned to 8 bytes
   if updating this also update the same in ulp_lp_core_memory_shared.c
 */
#define ALIGNED_COPROC_MEM ALIGN_DOWN(CONFIG_ULP_COPROC_RESERVE_MEM, 0x8)

ENTRY(reset_vector)

MEMORY
{
    /*first 128byte for exception/interrupt vectors*/
    vector_table(RX) :   ORIGIN = ULP_MEM_START_ADDRESS, LENGTH = 0x80
    ram(RWX) :           ORIGIN = ULP_MEM_START_ADDRESS + 0x80, LENGTH = ALIGNED_COPROC_MEM - 0x80 - CONFIG_ULP_SHARED_MEM
    shared_mem_ram(RW) : ORIGIN = ULP_MEM_START_ADDRESS + ALIGNED_COPROC_MEM - CONFIG_ULP_SHARED_MEM, LENGTH = CONFIG_ULP_SHARED_MEM
}

SECTIONS
{
    .vector.text :
    {
        /*exception/interrupt vectors*/
        __mtvec_base = .;
        KEEP (*(.init.vector .init.vector.*))
    } > vector_table

    . = ORIGIN(ram);

    .text ALIGN(4):
    {
        _flash_text_start = ABSOLUTE(.);
        *(.text.vectors) /* Default reset vector must link to offset 0x80 */
        *(.text)
        *(.text.*)
        _flash_text_end = ABSOLUTE(.);
    } > ram

    .rodata ALIGN(4):
    {
        _flash_rodata_start = ABSOLUTE(.);
        *(.rodata)
        *(.rodata*)
        _flash_rodata_end = ABSOLUTE(.);
    } > ram

    .crc_section_text ALIGN(4):
    {
        _crc_section_text_start = ABSOLUTE(.);
        LONG(0x00000000)
        KEEP(*(.crc_section_text))
        _crc_section_text_end = ABSOLUTE(.);
    } > ram

    .crc_section_data ALIGN(4):
    {
        _crc_section_data_start = ABSOLUTE(.);
        LONG(0x00000000)
        KEEP(*(.crc_section_data))
        _crc_section_data_end = ABSOLUTE(.);
    } > ram

    /*
     * Safe RAM sits below the marched region (matches bist_ram.c): backup
     * buffer + March stack, plus the software-mailbox context. The mailbox
     * must not be marched while the HP agent is online.
     */
    .dram0.safe_ram (NOLOAD):
    {
        . = ALIGN(4);
        _safe_ram_start = ABSOLUTE(.);
        KEEP(*(.dram0.safe_ram));
        *lp_core_mailbox_impl_sw*(.bss .bss.* .sbss .sbss.*);
        _safe_ram_end = ABSOLUTE(.);
    } > ram

    /*
     * Under-test RAM: live .data/.bss and the unused stack gap up to the
     * top of the LP ram region (March runs on the safe stack above).
     */
    _bist_ram_test_start = ABSOLUTE(.);

    .data ALIGN(4):
    {
        *(.data)
        *(.data*)
        *(.sdata)
        *(.sdata*)
    } > ram

    .bss ALIGN(4) :
    {
        *(EXCLUDE_FILE(*lp_core_mailbox_impl_sw*) .bss .bss.* .sbss .sbss.*)
        PROVIDE(end = .);
        . = ALIGN(4);
    } > ram

    /* define the stack overflow section */
    .dram0.overflow (NOLOAD) :
    {
        . = ALIGN(4);
        /* define the reserved block for stack overflow protection */
        . += 0x8;
        _stack_overflow_protection_start = .;
        . += 0x4; /* assuming 4 bytes for pattern check */
        _stack_overflow_protection_end = .;
        __data_end = ABSOLUTE(.);
    } > ram

    __stack_top = ORIGIN(ram) + LENGTH(ram);
    __stack_size = __stack_top - __data_end;

    _bist_ram_test_end = __stack_top;

    . = ORIGIN(shared_mem_ram);
    .shared_mem (ALIGN(4)) :
    {
        KEEP(*(.shared_mem))
    } > shared_mem_ram
}

/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "sdkconfig.h"
#include "soc/soc.h"
#include "ld.common"

#if CONFIG_ESP_ROM_HAS_LP_ROM
/* With LP-ROM memory layout is different due to LP ROM stack/data */
#define ULP_MEM_START_ADDRESS SOC_RTC_DRAM_LOW + RESERVE_RTC_MEM
#else
#define ULP_MEM_START_ADDRESS (SOC_RTC_DRAM_LOW)
#endif

#define ALIGN_DOWN(SIZE, AL)   (SIZE & ~(AL - 1))
/* Ensure the end where the shared memory starts is aligned to 8 bytes
   if updating this also update the same in ulp_lp_core_memory_shared.c
 */
#define ALIGNED_COPROC_MEM ALIGN_DOWN(CONFIG_ULP_COPROC_RESERVE_MEM, 0x8)

/* Fixed addr */
_pc_test_0_addr = ULP_MEM_START_ADDRESS + 0x20D8; /* 0x500020D8 */
_pc_test_1_addr = ULP_MEM_START_ADDRESS + 0x3F24; /* 0x50003F24 */
_pc_test_length = 0x38;
_text_length = _pc_test_0_addr - ULP_MEM_START_ADDRESS + 0x80; /* 0x2058 */
_bist_addr = _pc_test_0_addr + _pc_test_length; /* 0x50002110 */
_bist_length = 0x1400;
_data_addr = _bist_addr + _bist_length;
_data_length = _pc_test_1_addr - _data_addr;
_shared_mem_addr = _pc_test_1_addr + _pc_test_length; /* 0x50003F5C */

ENTRY(reset_vector)

MEMORY
{
    /*first 128byte for exception/interrupt vectors*/
    vector_table(RX) :    ORIGIN = ULP_MEM_START_ADDRESS , LENGTH = 0x80
    text(RWX) :           ORIGIN = ULP_MEM_START_ADDRESS + 0x80, LENGTH = _text_length
    bist_pc_test_0(RWX) : ORIGIN = _pc_test_0_addr, LENGTH = _pc_test_length
    bist(RWX) :           ORIGIN = _bist_addr, LENGTH = _bist_length
    data(RW) :            ORIGIN = _data_addr, LENGTH = _data_length
    bist_pc_test_1(RWX) : ORIGIN = _pc_test_1_addr, LENGTH = _pc_test_length
    shared_mem_ram(RW) :  ORIGIN = _shared_mem_addr, LENGTH = 0x14
}

SECTIONS
{
    .vector.text :
    {
        /*exception/interrupt vectors*/
        __mtvec_base = .;
        KEEP (*(.init.vector .init.vector.*))
    } > vector_table

    . = ORIGIN(text);

    .text ALIGN(4):
    {
        *(.text.vectors) /* Default reset vector must link to offset 0x80 */
        *(EXCLUDE_FILE(*libbist_esp.a:*) .text)
        *(EXCLUDE_FILE(*libbist_esp.a:*) .text.*)
        __text_end = .;
    } > text


    .pc_test_0 : ALIGN(4)
    {
        _pc_test_0_start = ABSOLUTE(.);
    *(.pc_test_0 .pc_test_0.*)
        _pc_test_0_end = ABSOLUTE(.);
    } > bist_pc_test_0

    .pc_test_1 : ALIGN(4)
    {
        _pc_test_1_start = ABSOLUTE(.);
    *(.pc_test_1 .pc_test_1.*)
        _pc_test_1_end = ABSOLUTE(.);
    } > bist_pc_test_0

    .bist.text ALIGN(4):
    {
        _text_bist_start = ABSOLUTE(.);
        *libbist_esp.a:*(.text .text.*)
        _text_bist_end = ABSOLUTE(.);
    } > bist

    . = ORIGIN(data);
    .rodata ALIGN(4):
    {
        _bist_ram_test_start = .;
        *(.rodata)
        *(.rodata*)
    } > data

    .data ALIGN(4):
    {
        *(.data)
        *(.data*)
        *(.sdata)
        *(.sdata*)
    } > data

    .bss ALIGN(4) :
    {
        *(.bss)
        *(.bss*)
        *(.sbss)
        *(.sbss*)
        PROVIDE(end = .);
        . = ALIGN(4);
        _bist_ram_test_end = .;
    } > data

    _bist_ram_test_size = _bist_ram_test_end - _bist_ram_test_start;

    .dram0.safe_ram (NOLOAD):
    {
        . = ALIGN(4);
        _safe_ram_start = ABSOLUTE(.);
        KEEP(*(.dram0.safe_ram));
        _safe_ram_end = ABSOLUTE(.);
    } > data

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
    } > data

    __stack_top = ORIGIN(data) + LENGTH(data);
    __stack_size = __stack_top - __data_end;

    .pc_test_2 : ALIGN(4)
    {
        _pc_test_2_start = ABSOLUTE(.);
        *(.pc_test_2 .pc_test_2.*)
        _pc_test_2_end = ABSOLUTE(.);
    } > bist_pc_test_1

    .pc_test_3 : ALIGN(4)
    {
        _pc_test_3_start = ABSOLUTE(.);
        *(.pc_test_3 .pc_test_3.*)
        _pc_test_3_end = ABSOLUTE(.);
    } > bist_pc_test_1

    . = ORIGIN(shared_mem_ram);
    .shared_mem (ALIGN(4)) :
    {
        KEEP(*(.shared_mem))
    } > shared_mem_ram
}

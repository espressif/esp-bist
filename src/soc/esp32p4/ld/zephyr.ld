/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/devicetree.h>
#include <zephyr/linker/sections.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#include "memory.h"

#define ALIGN_DOWN(SIZE, AL)   (SIZE & ~(AL - 1))
/* Ensure the end where the shared memory starts is aligned to 8 bytes
   if updating this also update the same in ulp_lp_core_memory_shared.c
 */
#define ALIGNED_COPROC_MEM ALIGN_DOWN(ESP32_ULP_COPROC_RESERVE_MEM, 0x8)

#define RODATA_REGION      ram
#define RAMABLE_REGION     ram
#define ROMABLE_REGION     ram

/* User available memory segments  */
_aligned_coproc_mem = ALIGNED_COPROC_MEM;
_vector_table_org = LPSRAM_IRAM_START;
_vector_table_len = 0x80;
_ram_org = _vector_table_org + _vector_table_len;
_ram_len = _aligned_coproc_mem - _vector_table_len - ESP32_ULP_SHARED_MEM_SIZE;
_shared_mem_org = ESP32_ULP_SHARED_MEM_ADDR;
_shared_mem_len = ESP32_ULP_SHARED_MEM_SIZE;

ENTRY(reset_vector)

MEMORY
{
    /*first 128byte for exception/interrupt vectors*/
    vector_table(RX) :   ORIGIN = _vector_table_org , LENGTH = _vector_table_len
    ram(RWX) :           ORIGIN = _ram_org, LENGTH = _ram_len
    shared_mem_ram(RW) : ORIGIN = _shared_mem_org, LENGTH = _shared_mem_len
}

SECTIONS
{
    .vector.text :
    {
        __mtvec_base = .;
        KEEP (*(.init.vector .init.vector.*))
    } > vector_table

    . = ORIGIN(ram);

    .text ALIGN(4):
    {
        _flash_text_start = ABSOLUTE(.);
        __text_region_start = ABSOLUTE(.);
        *(.text.vectors) /* Default reset vector must link to offset 0x80 */
        *(.text)
        *(.text*)
        *(.iram1)
        *(.iram1.*)
        . = ALIGN(4);
        __text_region_end = ABSOLUTE(.);
        _flash_text_end = ABSOLUTE(.);
    } >ram

  #include <zephyr/linker/rel-sections.ld>

    .rodata ALIGN(4):
    {
        __rodata_region_start = ABSOLUTE(.);
        _flash_rodata_start = ABSOLUTE(.);
        _bist_ram_test_start = .;
        *(.rodata .srodata)
        *(.rodata* .srodata*)
        . = ALIGN(4);
        __rodata_region_end = .;
        _flash_rodata_end = ABSOLUTE(.);

    } > ram

    .crc_section_text : ALIGN(4)
    {
        _crc_section_text_start = ABSOLUTE(.);
        LONG(0x00000000)
        KEEP(*(.crc_section_text))
        _crc_section_text_end = ABSOLUTE(.);
    } > ram

    .crc_section_data : ALIGN(4)
    {
        _crc_section_data_start = ABSOLUTE(.);
        LONG(0x00000000)
        KEEP(*(.crc_section_data))
        _crc_section_data_end = ABSOLUTE(.);
    } > ram

#include <zephyr/linker/common-rom/common-rom-kernel-devices.ld>

    .rodata.end ALIGN(4):
    {
        _rodata_reserved_end = ABSOLUTE(.);
    } > ram

    .data ALIGN(4):
    {
        _image_ram_start = ABSOLUTE(.);
        *(.data)
        *(.data*)
        *(.sdata)
        *(.sdata*)

    } > ram

  #include <snippets-data-sections.ld>
  #include <zephyr/linker/common-ram.ld>
  #include <snippets-ram-sections.ld>

    .data.end ALIGN(4):
    {
        _image_ram_end = ABSOLUTE(.);
    } > ram

    .data.noinit (NOLOAD):
    {
        . = ALIGN(4);
        *(.noinit)
        *(.noinit.*)
        . = ALIGN(4);
    } > ram

    .bss ALIGN(4) :
    {
        __bss_start = .;
        *(.bss)
        *(.bss*)
        *(.sbss)
        *(.sbss*)
        PROVIDE(end = .);
        __bss_end = .;
        _heap_sentry = .;
    } >ram

#include <zephyr/linker/ram-end.ld>

    _bist_ram_test_end = _image_ram_end;

    .dram0.safe_ram (NOLOAD):
    {
        . = ALIGN(4);
        _safe_ram_start = ABSOLUTE(.);
        KEEP(*(.dram0.safe_ram));
        _safe_ram_end = ABSOLUTE(.);
    } > ram

    .dram0.overflow (NOLOAD) :
    {
        . = ALIGN(4);
        _stack_overflow_protection_end = .;
        . += 0x100;
        _stack_overflow_protection_start = .;
    } > ram

    _stack_top = ORIGIN(ram) + LENGTH(ram);
    __stack_top = _stack_top;

#include <zephyr/linker/debug-sections.ld>

    /DISCARD/ : { *(.note.GNU-stack) }

    SECTION_PROLOGUE(.riscv.attributes, 0,)
    {
    KEEP(*(.riscv.attributes))
    KEEP(*(.gnu.attributes))
    }

    . = ORIGIN(shared_mem_ram);
    .shared_mem (NOLOAD) :
    {
        KEEP(*(.shared_mem))
    } > shared_mem_ram
}

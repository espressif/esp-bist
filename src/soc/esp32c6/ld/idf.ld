/*
 * SPDX-FileCopyrightText: 2023-2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 *
 * BIST custom LP-core linker layout for ESP32-C6.
 * Replaces the default lp_core_riscv_default_layout.ld.in when passed via the
 * LINKER_LAYOUT option to ulp_add_project(). The base part
 * (lp_core_riscv_base.ld.in) provides the section-boundary macros, vector
 * table, shared memory, and stack window.
 */

#if CONFIG_ESP_BIST_PROGRAM_COUNTER_TEST
/*
 * PC register stuck-at test: place functions at fixed addresses chosen so
 * that bits 2-13 of the PC appear as both 0 and 1 across the set.
 *
 *   pc_test_1 @ USER_START + 0x2A28 -> absolute 0x50002AA8
 *              bits 13:2 = 101010101010
 *   pc_test_2 @ USER_START + 0x34D4 -> absolute 0x50003554
 *              bits 13:2 = 110101010101
 *
 * Together with pc_test_0 (natural position in .text, bit 13 = 0), all 12
 * testable PC bits within the 16KB LP RAM window are exercised.
 */
_pc_test_1_addr = LP_CORE_USER_MEMORY_REGION_START + 0x2A28;
_pc_test_2_addr = LP_CORE_USER_MEMORY_REGION_START + 0x34D4;
#endif

MEMORY
{
#if CONFIG_ESP_BIST_PROGRAM_COUNTER_TEST
    lp_text(RWX)     : ORIGIN = LP_CORE_USER_MEMORY_REGION_START, LENGTH = _pc_test_1_addr - LP_CORE_USER_MEMORY_REGION_START
    pc_region_1(RX)  : ORIGIN = _pc_test_1_addr, LENGTH = 0x20
    lp_text_mid(RWX) : ORIGIN = _pc_test_1_addr + 0x20, LENGTH = _pc_test_2_addr - (_pc_test_1_addr + 0x20)
    pc_region_2(RX)  : ORIGIN = _pc_test_2_addr, LENGTH = 0x20
    lp_data(RW)      : ORIGIN = _pc_test_2_addr + 0x20, LENGTH = LP_CORE_USER_MEMORY_REGION_END - (_pc_test_2_addr + 0x20)
#else
    lp_text(RWX)     : ORIGIN = LP_CORE_USER_MEMORY_REGION_START, LENGTH = LP_CORE_USER_MEMORY_REGION_END - LP_CORE_USER_MEMORY_REGION_START
    lp_data(RW)      : ORIGIN = LP_CORE_USER_MEMORY_REGION_START, LENGTH = LP_CORE_USER_MEMORY_REGION_END - LP_CORE_USER_MEMORY_REGION_START
#endif
}

SECTIONS
{
    LP_CORE_TEXT_START(lp_text)

    .text ALIGN(4) :
    {
        _flash_text_start = ABSOLUTE(.);
        *(.text)
        *(.text.*)

#if CONFIG_ESP_BIST_PROGRAM_COUNTER_TEST
        . = ALIGN(4);
        _pc_test_0_start = ABSOLUTE(.);
        KEEP(*(.pc_test_0 .pc_test_0.*))
        _pc_test_0_end = ABSOLUTE(.);
#endif
        _flash_text_end = ABSOLUTE(.);
    } > lp_text

    .rodata ALIGN(4) :
    {
        _flash_rodata_start = ABSOLUTE(.);
        *(.rodata)
        *(.rodata*)
        _flash_rodata_end = ABSOLUTE(.);
    } > lp_text

    LP_CORE_TEXT_END()

#if CONFIG_ESP_BIST_PROGRAM_COUNTER_TEST
    . = ORIGIN(pc_region_1);
    .pc_test_1 ALIGN(4) :
    {
        _pc_test_1_start = ABSOLUTE(.);
        KEEP(*(.pc_test_1 .pc_test_1.*))
        _pc_test_1_end = ABSOLUTE(.);
    } > pc_region_1

    . = ORIGIN(pc_region_2);
    .pc_test_2 ALIGN(4) :
    {
        _pc_test_2_start = ABSOLUTE(.);
        KEEP(*(.pc_test_2 .pc_test_2.*))
        _pc_test_2_end = ABSOLUTE(.);
    } > pc_region_2

    .pc_test_3 ALIGN(4) :
    {
        _pc_test_3_start = ABSOLUTE(.);
        KEEP(*(.pc_test_3 .pc_test_3.*))
        _pc_test_3_end = ABSOLUTE(.);
    } > pc_region_2
#endif

#if CONFIG_ESP_BIST_PROGRAM_COUNTER_TEST
    . = ORIGIN(lp_data);
#endif

    .crc_section_text ALIGN(4) :
    {
        _crc_section_text_start = ABSOLUTE(.);
        LONG(0x00000000)
        KEEP(*(.crc_section_text))
        _crc_section_text_end = ABSOLUTE(.);
    } > lp_data

    .crc_section_data ALIGN(4) :
    {
        _crc_section_data_start = ABSOLUTE(.);
        LONG(0x00000000)
        KEEP(*(.crc_section_data))
        _crc_section_data_end = ABSOLUTE(.);
    } > lp_data

    LP_CORE_DATA_START()

    /*
     * Safe RAM sits below the marched region (matches bist_ram.c): backup
     * buffer + March stack, plus the software-mailbox context. The mailbox
     * must not be marched while the HP agent is online.
     */
    .dram0.safe_ram (NOLOAD) :
    {
        . = ALIGN(4);
        _safe_ram_start = ABSOLUTE(.);
        KEEP(*(.dram0.safe_ram));
        *lp_core_mailbox_impl_sw*(.bss .bss.* .sbss .sbss.*);
        _safe_ram_end = ABSOLUTE(.);
    } > lp_data

    /*
     * Under-test RAM: live .data/.bss and the unused stack gap up to the
     * top of the LP ram region (March runs on the safe stack above).
     */
    _bist_ram_test_start = ABSOLUTE(.);

    .data ALIGN(4) :
    {
        *(.data)
        *(.data*)
        *(.sdata)
        *(.sdata*)
    } > lp_data

    .bss ALIGN(4) :
    {
        *(EXCLUDE_FILE(*lp_core_mailbox_impl_sw*) .bss .bss.* .sbss .sbss.*)
        PROVIDE(end = .);
        . = ALIGN(4);
    } > lp_data

    /* define the stack overflow section */
    .dram0.overflow (NOLOAD) :
    {
        . = ALIGN(4);
        /* define the reserved block for stack overflow protection */
        . += 0x8;
        _stack_overflow_protection_start = .;
        . += 0x4; /* assuming 4 bytes for pattern check */
        _stack_overflow_protection_end = .;
    } > lp_data

    LP_CORE_DATA_END()

    _bist_ram_test_end = __stack_top;
}

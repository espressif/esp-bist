/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../include/sdkconfig.h"
#include "bist_conf.h"

/* Flash */
#ifdef CONFIG_ESP_BIST_FLASH_SIZE
#define FLASH_SIZE         CONFIG_ESP_BIST_FLASH_SIZE
#else
#define FLASH_SIZE         (0x400000)
#endif

#define SRAM_SEG_START       (0x40810000)
#define SRAM_SEG_END         (0x4084f350)
#define SRAM_SEG_LEN         (SRAM_SEG_END - SRAM_SEG_START)

#define ICACHE_SIZE           (0x8000) // 32kB

#define CACHE_ALIGN  CONFIG_MMU_PAGE_SIZE
#define IROM_SEG_ORG 0x42000000
#define IROM_SEG_LEN FLASH_SIZE
#define DROM_SEG_ORG 0x42000000
#define DROM_SEG_LEN FLASH_SIZE

/* Global symbols required for espressif hal build */
MEMORY
{
  mcuboot_hdr (R): org = 0x0, len = 0x20
  metadata (R): org = 0x20, len = 0x60

  ROM (RX): org = 0x80, len = FLASH_SIZE - 0x80

  sram(RX): org = SRAM_SEG_START, len = SRAM_SEG_LEN
  irom0_0_seg(RX): org = IROM_SEG_ORG, len = IROM_SEG_LEN
  drom0_0_seg(R):  org = DROM_SEG_ORG, len = DROM_SEG_LEN

}

/*  Default entry point:  */
ENTRY(__start)

SECTIONS
{
  /* Reserve space for MCUboot header in the binary */
  .mcuboot_header :
  {
    QUAD(0x0)
    QUAD(0x0)
    QUAD(0x0)
    QUAD(0x0)
  } > mcuboot_hdr

  .metadata :
  {
    /* 0. Magic byte for load header */
    LONG(0xace637d3)

    /* 1. Application entry point address */
    KEEP(*(.entry_addr))

    /* IRAM metadata:
     * 2. Destination address (VMA) for IRAM region
     * 3. Flash offset (LMA) for start of IRAM region
     * 4. Size of IRAM region
     */
    LONG(ADDR(.iram0.text))
    LONG(LOADADDR(.iram0.text))
    LONG(LOADADDR(.iram0.data) - LOADADDR(.iram0.text))
    /* DRAM metadata:
     * 5. Destination address (VMA) for DRAM region
     * 6. Flash offset (LMA) for start of DRAM region
     * 7. Size of DRAM region
     */
    LONG(ADDR(.dram0.data))
    LONG(LOADADDR(.dram0.data))
    LONG(LOADADDR(.dram0.end) - LOADADDR(.dram0.data))

  } > metadata

  /* --- END OF RTC --- */

  /* --- START OF IRAM --- */

  .iram0.text : ALIGN(4)
  {
    _iram_start = ABSOLUTE(.);
    _init_start = ABSOLUTE(.);

    KEEP(*(.exception_vectors_table.text));
    KEEP(*(.exception_vectors.text));
    . = ALIGN(256);

    _invalid_pc_placeholder = ABSOLUTE(.);

    *(.entry.text)
    *(.init.literal)
    *(.init)
    . = ALIGN(4);

    _init_end = ABSOLUTE(.);
    _iram_text_start = ABSOLUTE(.);

    *(.iram1 .iram1.*)
    *(.iram0.literal .iram.literal .iram.text.literal .iram0.text .iram.text)
    *start.*(.literal .text .literal.* .text.*)
    *panic.*(.literal .text .literal.* .text.*)
    *panic_handler.*(.literal .text .literal.* .text.*)
    *loader.*(.literal .text .literal.* .text.*)
    *clock_init.*(.literal .text .literal.* .text.*)
    *libgcc.a:lib2funcs.*(.literal .text .literal.* .text.*)
    *libgcc.a:lib2funcs.*(.literal .text .literal.* .text.*)
    *log_noos.*(.literal .text .literal.* .text.*)
    *rtc_*.*(.literal .text .literal.* .text.*)
    *periph_ctrl.*(.literal .text .literal.* .text.*)
    *bootloader_esp32h4.*(.literal .text .literal.* .text.* .iram1 .iram1.*)

    *interrupt.*(.text .text.* .literal .literal.*)

    *cache_hal.*(.text .text.* .literal .literal.*)
    *mmu_hal.*(.text .text.* .literal .literal.*)
    *wdt_hal_iram.*(.literal .text .literal.* .text.*)

    *(.literal.esp_log_write .text.esp_log_write)
    *(.literal.esp_log_timestamp .text.esp_log_timestamp)
    *(.literal.esp_log_early_timestamp .text.esp_log_early_timestamp)
    *(.literal.esp_log_impl_lock .text.esp_log_impl_lock)
    *(.literal.esp_log_impl_lock_timeout .text.esp_log_impl_lock_timeout)
    *(.literal.esp_log_impl_unlock .text.esp_log_impl_unlock)

    *(.literal.esp_system_abort .text.esp_system_abort)

    *(.literal.esp_cpu_stall .text.esp_cpu_stall)
    *(.literal.esp_cpu_unstall .text.esp_cpu_unstall)
    *(.literal.esp_cpu_reset .text.esp_cpu_reset)
    *(.literal.esp_cpu_wait_for_intr .text.esp_cpu_wait_for_intr)
    *(.literal.esp_cpu_compare_and_set .text.esp_cpu_compare_and_set)
    *(.literal.esp_gpio_reserve_pins .text.esp_gpio_reserve_pins)
    *(.literal.esp_gpio_is_pin_reserved .text.esp_gpio_is_pin_reserved)
    *(.literal.rtc_vddsdio_get_config .text.rtc_vddsdio_get_config)
    *(.literal.rtc_vddsdio_set_config .text.rtc_vddsdio_set_config)
    *esp_memory_utils.*(.literal .literal.* .text .text.*)
    *rtc_clk.*(.literal .literal.* .text .text.*)
    *rtc_clk_init.*(.literal .literal.* .text .text.*)
    *rtc_time.*(.literal .literal.* .text .text.*)
    *rtc_sleep.*(.literal .literal.* .text .text.*)
    *systimer.*(.literal .literal.* .text .text.*)
    *mspi_timing_config.*(.literal .literal.* .text .text.*)
    *mspi_timing_tuning.*(.literal .literal.* .text .text.*)
    *(.literal.sar_periph_ctrl_power_enable .text.sar_periph_ctrl_power_enable)

    *(.literal.GPIO_HOLD_MASK .text.GPIO_HOLD_MASK)

    /* newlib memcpy/memset must be in IRAM: map_rtc_segment calls memcpy
     * before map_rom_segments has set up the flash (IROM) mapping. */
    *libc.a:libc_a-memcpy.*(.text .text.* .literal .literal.*)
    *libc.a:libc_a-memset.*(.text .text.* .literal .literal.*)

    *libbist_esp.a:*(.text .text.*)

    /* Align to 8 so pcTestFunction0 has bit 2 = 0; the GDB fault-injection
       test relies on XOR ^ 4 landing inside the function body, not before it. */
    . = ALIGN(8);
    _pc_test_0_start = ABSOLUTE(.);
    *(.pc_test_0 .pc_test_0.*)
    _pc_test_0_end = ABSOLUTE(.);

    . = ALIGN(4) + 16;

  } > sram AT>ROM

  .iram0.text_end (NOLOAD) :
  {
    . = ALIGN (16);
    _iram_text_end = ABSOLUTE(.);
  } > sram

  .iram0.data :
  {
    . = ALIGN(16);
    *(.iram.data)
    *(.iram.data*)
  } > sram AT>ROM

  .iram0.bss (NOLOAD) :
  {
    . = ALIGN(16);
    _iram_bss_start = ABSOLUTE(.);

    *(.iram.bss)
    *(.iram.bss*)

    _iram_bss_end = ABSOLUTE(.);
    . = ALIGN(16);
    _iram_end = ABSOLUTE(.);
  } > sram

  /* --- END OF IRAM --- */

  /* --- START OF DRAM --- */
  .dram0.data :
  {
    . = ALIGN(4);
    _data_start = ABSOLUTE(.);
    __data_start = ABSOLUTE(.);

    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d.*)
    *(.data1)

    . = ALIGN(8);
    __global_pointer$ = . + 0x800;

    *(.sdata)
    *(.sdata.*)
    *(.gnu.linkonce.s.*)
    *(.sdata2)
    *(.sdata2.*)
    *(.gnu.linkonce.s2.*)

    *start.*(.rodata .rodata.* .srodata .srodata.*)
    *panic_handler.*(.rodata .rodata.* .srodata .srodata.*)
    *loader.*(.rodata .rodata.* .srodata .srodata.*)
    *clock_init.*(.rodata .rodata.* .srodata .srodata.*)
    *bare_metal_stubs.*(.rodata .rodata.* .srodata .srodata.*)
    *periph_ctrl.*(.rodata .rodata.* .srodata .srodata.*)
    *rv_utils.*(.rodata .rodata.* .srodata .srodata.*)
    *interrupt_clic.*(.rodata .rodata.* .srodata .srodata.*)
    *pmu_init.*(.rodata .rodata.* .srodata .srodata.*)
    *pmu_param.*(.rodata .rodata.* .srodata .srodata.*)
    *modem_clock_hal.*(.rodata .rodata.* .srodata .srodata.*)

    /* [mapping:hal] */
    *mmu_hal.*(.rodata .rodata.* .srodata .srodata.*)
    *cache_hal.*(.rodata .rodata.* .srodata .srodata.*)
    *wdt_hal_iram.*(.rodata .rodata.* .srodata .srodata.*)

    /* [mapping:log] */
    *(.rodata.esp_log_write)
    *(.rodata.esp_log_timestamp)
    *(.rodata.esp_log_early_timestamp)
    *(.rodata.esp_log_impl_lock)
    *(.rodata.esp_log_impl_lock_timeout)
    *(.rodata.esp_log_impl_unlock)

    /* [mapping:esp_hw_support] */
    *(.rodata.esp_cpu_stall)
    *(.rodata.esp_cpu_unstall)
    *(.rodata.esp_cpu_reset)
    *(.rodata.esp_cpu_wait_for_intr)
    *(.rodata.esp_cpu_compare_and_set)
    *(.rodata.esp_gpio_reserve_pins)
    *(.rodata.esp_gpio_is_pin_reserved)
    *(.rodata.rtc_vddsdio_get_config)
    *(.rodata.rtc_vddsdio_set_config)
    *esp_memory_utils.*(.rodata .rodata.* .srodata .srodata.*)
    *rtc_clk.*(.rodata .rodata.* .srodata .srodata.*)
    *rtc_clk_init.*(.rodata .rodata.* .srodata .srodata.*)
    *rtc_time.*(.rodata .rodata.* .srodata .srodata.*)
    *systimer.*(.rodata .rodata.* .srodata .srodata.*)
    *mspi_timing_config.*(.rodata .rodata.* .srodata .srodata.*)
    *mspi_timing_tuning.*(.rodata .rodata.* .srodata .srodata.*)
    *(.rodata.sar_periph_ctrl_power_enable)

    /* [mapping:esp_system] */
    *(.rodata.esp_system_abort)

    *libbist_esp.a:*(.rodata .rodata.*)

    . = ALIGN(4);

    KEEP(*(.jcr))
    *(.dram1 .dram1.*)
    . = ALIGN(4);
  } >sram AT>ROM

  .dram0.end :
  {
    . = ALIGN(4);
    _end = ABSOLUTE(.);
    _data_end = ABSOLUTE(.);
    __data_end = ABSOLUTE(.);
  } >sram AT>ROM

  .dram0.noinit (NOLOAD):
  {
    . = ALIGN(4);
    _noinit_start = ABSOLUTE(.);
    *(.noinit)
    *(.noinit.*)
    . = ALIGN(4);
    _noinit_end = ABSOLUTE(.);
  } >sram

  /* Shared RAM */
  .dram0.bss (NOLOAD) :
  {
    . = ALIGN (8);
    _bss_start = ABSOLUTE(.);

    *(.dynsbss)
    *(.sbss)
    *(.sbss.*)
    *(.gnu.linkonce.sb.*)
    *(.scommon)
    *(.sbss2)
    *(.sbss2.*)
    *(.gnu.linkonce.sb2.*)
    *(.dynbss)
    *(.bss)
    *(.bss.*)
    *(.share.mem)
    *(.gnu.linkonce.b.*)

    . = ALIGN (8);
    _bss_end = ABSOLUTE(.);
  } >sram

  .dram0.safe_ram (NOLOAD):
  {
    . = ALIGN(4);
    _safe_ram_start = ABSOLUTE(.);
    KEEP(*(.dram0.safe_ram))
    _safe_ram_end = ABSOLUTE(.);
  } >sram

  .dram0.heap_start (NOLOAD):
  {
    . = ALIGN (4);
    _heap_start = ABSOLUTE(.);
    _bist_ram_test_start = ABSOLUTE(.);
    /* define the reserved block for heap */
    . += CONFIG_ESP_BIST_HEAP_SIZE;
    _heap_end = ABSOLUTE(.);
    KEEP(*(.dram0.heap_start))
  } > sram

  /* define the stack overflow section */
  .dram0.overflow (NOLOAD) :
  {
    . = ALIGN(4);
    _stack_overflow_protection_end = .;
    . += CONFIG_ESP_BIST_STACK_PROTECTION_BLOCK_SIZE;
    _stack_overflow_protection_start = .;
  } >sram

  _sram_end = ORIGIN(sram) + LENGTH(sram);
  _bist_ram_test_end = _sram_end;
  _stack_top = _sram_end;

  _image_ram_start = _iram_start;

  ASSERT(((_heap_end - ORIGIN(sram)) <= LENGTH(sram)), "DRAM segment data does not fit.")

  /* --- END OF DRAM --- */

  /* --- START OF .rodata --- */

  .flash.rodata_dummy (NOLOAD) :
  {
    . = ALIGN(CACHE_ALIGN);
  } >ROM

  _image_drom_start = LOADADDR(.flash.rodata);
  _image_drom_size = LOADADDR(.flash.rodata_end) + SIZEOF(.flash.rodata_end) - _image_drom_start;
  _image_drom_vaddr = ADDR(.flash.rodata);

  .flash.rodata : ALIGN(0x10)
  {
    _rodata_reserved_start = ABSOLUTE(.);
    _rodata_start = ABSOLUTE(.);
    _flash_rodata_start = ABSOLUTE(.);

    *(.rodata_desc .rodata_desc.*)
    *(.rodata_custom_desc .rodata_custom_desc.*)

    __rodata_region_start = .;

    . = ALIGN(4);

    *(.irom1.text) /* catch stray ICACHE_RODATA_ATTR */
    *(.gnu.linkonce.r.*)
    *(.rodata1)
    __XT_EXCEPTION_TABLE_ = ABSOLUTE(.);
    *(.xt_except_table)
    *(.gcc_except_table .gcc_except_table.*)
    *(.gnu.linkonce.e.*)
    *(.gnu.version_r)
    . = (. + 3) & ~ 3;
    __eh_frame = ABSOLUTE(.);
    KEEP(*(.eh_frame))
    . = (. + 7) & ~ 3;

    /* C++ exception handlers table: */
    __XT_EXCEPTION_DESCS_ = ABSOLUTE(.);
    *(.xt_except_desc)
    *(.gnu.linkonce.h.*)
    __XT_EXCEPTION_DESCS_END__ = ABSOLUTE(.);
    *(.xt_except_desc_end)
    *(.dynamic)
    *(.gnu.version_d)
    __rodata_region_end = .;
    _rodata_end = ABSOLUTE(.);
    _lit4_start = ABSOLUTE(.);
    *(*.lit4)
    *(.lit4.*)
    *(.gnu.linkonce.lit4.*)
    _lit4_end = ABSOLUTE(.);
    . = ALIGN(4);
    *(.srodata)
    *(.srodata.*)
    *(.rodata)
    *(.rodata.*)
    *(.rodata_wlog)
    *(.rodata_wlog*)
    . = ALIGN(4);
    _flash_rodata_end = ABSOLUTE(.);
  } >drom0_0_seg AT>ROM

  .crc_section_text : ALIGN(4)
  {
    _crc_section_text_start = ABSOLUTE(.);
    LONG(0x00000000)
    KEEP(*(.crc_section_text))
    _crc_section_text_end = ABSOLUTE(.);
  } >drom0_0_seg AT>ROM

  .crc_section_data : ALIGN(4)
  {
    _crc_section_data_start = ABSOLUTE(.);
    LONG(0x00000000)
    KEEP(*(.crc_section_data))
    _crc_section_data_end = ABSOLUTE(.);
  } >drom0_0_seg AT>ROM

  .flash.rodata_end : ALIGN(0x10)
  {
    . = ALIGN(4);
    _rodata_reserved_end = ABSOLUTE(.);
    _image_rodata_end = ABSOLUTE(.);
  } >drom0_0_seg AT>ROM

  /* --- END OF .rodata --- */

  /* --- START OF .flash.text --- */

  _image_irom_start = LOADADDR(.flash.text);
  _image_irom_size = SIZEOF(.flash.text);
  _image_irom_vaddr = ADDR(.flash.text);

  .flash.text_dummy (NOLOAD):
  {
    . += _image_rodata_end - _rodata_start;
    . = ALIGN(CACHE_ALIGN);
  } >irom0_0_seg AT>ROM

  .flash.text : ALIGN(CACHE_ALIGN)
  {
    _stext = .;
    _instruction_reserved_start = ABSOLUTE(.);
    _text_start = ABSOLUTE(.);
    _instruction_reserved_start = ABSOLUTE(.);
    _flash_text_start = ABSOLUTE(.);

#if CONFIG_ESP_BIST_PROGRAM_COUNTER_TEST
    _pc_test_1_start = ABSOLUTE(.);
    *(.pc_test_1 .pc_test_1.*)
    _pc_test_1_end = ABSOLUTE(.);

    . = 0xFFF8;
    _pc_test_2_start = ABSOLUTE(.);
    *(.pc_test_2 .pc_test_2.*)
    _pc_test_2_end = ABSOLUTE(.);
#endif

    *(.literal .text .literal.* .text.*)
    *(.stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    *(.irom0.text) /* catch stray ICACHE_RODATA_ATTR */
    *(.fini.literal)
    *(.fini)
    *(.gnu.version)

    . += 16;

    _instruction_reserved_end = ABSOLUTE(.);
    _text_end = ABSOLUTE(.);
    _instruction_reserved_end = ABSOLUTE(.);
    _etext = .;

    _flash_cache_start = ABSOLUTE(0);
    _flash_text_end = ABSOLUTE(.);

  } >irom0_0_seg AT>ROM

  /* --- END OF .flash.text --- */
}

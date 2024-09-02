/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "soc_memory.h"
#include "../include/sdkconfig.h"

/* SRAM0 (16kB) memory */
#define SRAM0_IRAM_START   0x4037c000
#define SRAM0_SIZE         0x4000
/* SRAM1 (384kB) memory */
#define SRAM1_DRAM_START   0x3fc80000
#define SRAM1_IRAM_START   0x40380000
/* ICache size is fixed to 16KB on ESP32-C3 */
#define ICACHE_SIZE        SRAM0_SIZE

/* USER_IRAM_END is equivalent 2nd stage bootloader "iram_loader_seg"
 * start address (that should not be overlapped). If no bootloader
 * is used we can move this boundary further */
#define USER_IRAM_END      BOOTLOADER_IRAM_LOADER_SEG_START

/* User available SRAM memory segments */
#define USER_DRAM_SEG_ORG  SRAM1_DRAM_START
#define USER_IRAM_SEG_ORG  SRAM1_IRAM_START
#define USER_DRAM_END      (USER_IRAM_END - IRAM_DRAM_OFFSET)
#define USER_IDRAM_SIZE    (USER_DRAM_END - USER_DRAM_SEG_ORG)
#define USER_IRAM_SEG_LEN  USER_IDRAM_SIZE
#define USER_DRAM_SEG_LEN  USER_IDRAM_SIZE

/* Flash */
#ifdef CONFIG_FLASH_SIZE
#define FLASH_SIZE         CONFIG_FLASH_SIZE
#else
#define FLASH_SIZE         0x400000
#endif

/* Cached memory */
#define CACHE_ALIGN        0x10000
#define IROM_SEG_ORG       0x42000000
#define IROM_SEG_LEN       FLASH_SIZE
#define DROM_SEG_ORG       0x3c000000
#define DROM_SEG_LEN       FLASH_SIZE

/* Aliases */
#define FLASH_CODE_REGION  irom0_0_seg
#define RODATA_REGION      drom0_0_seg
#define IRAM_REGION        iram0_0_seg
#define DRAM_REGION        dram0_0_seg
#define RAMABLE_REGION     dram0_0_seg
#define ROMABLE_REGION     ROM

/* Global symbols required for espressif hal build */
MEMORY
{
  mcuboot_hdr (R): org = 0x0, len = 0x20
  metadata (R): org = 0x20, len = 0x20

  ROM (RX): org = 0x40, len = FLASH_SIZE - 0x40

  iram0_0_seg(RX): org = USER_IRAM_SEG_ORG, len = USER_IRAM_SEG_LEN
  dram0_0_seg(RW): org = USER_DRAM_SEG_ORG, len = USER_DRAM_SEG_LEN

  irom0_0_seg(RX): org = IROM_SEG_ORG, len = IROM_SEG_LEN
  drom0_0_seg (R): org = DROM_SEG_ORG, len = DROM_SEG_LEN

  rtc_iram_seg(RWX): org = 0x50000000, len = 0x2000
}

/* The line below defines location alias for .rtc.data section
 * As C3 only has RTC fast memory, this is not configurable like
 * on other targets.
 */
REGION_ALIAS("rtc_slow_seg", rtc_iram_seg);

/*  Default entry point:  */
ENTRY(__start)

_rom_store_table = 0;

_iram_dram_offset = IRAM_DRAM_OFFSET;
_user_iram_end = USER_IRAM_END;
_user_dram_end = USER_DRAM_END;

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

  iram_vma = ADDR(.iram0.text);
  iram_lma = LOADADDR(.iram0.text);
  iram_size_field = LOADADDR(.iram0.data) - LOADADDR(.iram0.text);

  dram_vma = ADDR(.dram0.data);
  dram_lma = LOADADDR(.dram0.data);
  dram_size_field = LOADADDR(.dram0.end) - LOADADDR(.dram0.data);

  /* --- START OF RTC --- */

  /* Symbols used during the application memory mapping */
  _image_rtc_start = LOADADDR(.rtc.text);
  _image_rtc_size = SIZEOF(.rtc.text);
  _image_rtc_vaddr = ADDR(.rtc.text);

  .rtc.text :
  {
    . = ALIGN(4);
    *(.rtc.literal .rtc.literal.*)
    *(.rtc.text .rtc.text.*)
    _pc_test_3_start = ABSOLUTE(.);
   *(.pc_test_3 .pc_test_3.*)
    _pc_test_3_end = ABSOLUTE(.);
    *rtc_wake_stub*.o(.literal .text .literal.* .text.*)
  } >rtc_iram_seg AT>ROM

  /* This section is required to skip rtc.text area because the text and
   * data segments reflect the same address space on different buses.
   */
  .rtc.dummy (NOLOAD):
  {
    . = SIZEOF(.rtc.text);
  } >rtc_iram_seg

  .rtc.data :
  {
    _rtc_data_start = ABSOLUTE(.);
    *(.rtc.data .rtc.data.*)
    *(.rtc.rodata .rtc.rodata.*)
    *rtc_wake_stub*.o(.data .rodata .data.* .rodata.* .bss .bss.*)
    _rtc_data_end = ABSOLUTE(.);
  } >rtc_iram_seg AT>ROM

  .rtc.bss (NOLOAD) :
  {
    _rtc_bss_start = ABSOLUTE(.);
    *rtc_wake_stub*.o(.bss .bss.*)
    *rtc_wake_stub*.o(COMMON)
    _rtc_bss_end = ABSOLUTE(.);
  } >rtc_iram_seg

  /* This section located in RTC SLOW Memory area.
   * It holds data marked with RTC_SLOW_ATTR attribute.
   * See the file "esp_attr.h" for more information.
   */
  .rtc.force_slow :
  {
    . = ALIGN(4);
    _rtc_force_slow_start = ABSOLUTE(.);
    *(.rtc.force_slow .rtc.force_slow.*)
    . = ALIGN(4) ;
    _rtc_force_slow_end = ABSOLUTE(.);
  } > rtc_slow_seg

  /* Get size of rtc slow data */
  _rtc_slow_length = (_rtc_force_slow_end - _rtc_data_start);

  /* --- END OF RTC --- */

  /* --- START OF IRAM --- */

  .iram0.text : ALIGN(4)
  {
    /* Vectors go to IRAM */
    _iram_start = ABSOLUTE(.);
    _init_start = ABSOLUTE(.);

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
    *loader.*(.literal .text .literal.* .text.*)
    *libarch__riscv__core.a:(.literal .text .literal.* .text.*)
    *libsubsys__net__l2__ethernet.a:(.literal .text .literal.* .text.*)
    *libsubsys__net__lib__config.a:(.literal .text .literal.* .text.*)
    *libsubsys__net__ip.a:(.literal .text .literal.* .text.*)
    *libsubsys__net.a:(.literal .text .literal.* .text.*)
    *libgcc.a:lib2funcs.*(.literal .text .literal.* .text.*)
    *libdrivers__flash.a:flash_esp32.*(.literal .text .literal.* .text.*)
    *log_noos.*(.literal .text .literal.* .text.*)
    *libdrivers__timer.a:esp32c3_sys_timer.*(.literal .text .literal.* .text.*)
    *log_core.*(.literal .text .literal.* .text.*)
    *cbprintf_complete.*(.literal .text .literal.* .text.*)
    *printk.*(.literal.printk .literal.vprintk .literal.char_out .text.printk .text.vprintk .text.char_out)
    *log_msg.*(.literal .text .literal.* .text.*)
    *log_list.*(.literal .text .literal.* .text.*)
    *libdrivers__console.a:uart_console.*(.literal.console_out .text.console_out)
    *log_output.*(.literal .text .literal.* .text.*)
    *log_backend_uart.*(.literal .text .literal.* .text.*)
    *rtc_*.*(.literal .text .literal.* .text.*)
    *periph_ctrl.*(.literal .text .literal.* .text.*)

    /* [mapping:riscv] */
    *interrupt.*(.text .text.* .literal .literal.*)

    /* [mapping:hal] */
    *cache_hal.*(.text .text.* .literal .literal.*)
    *mpu_hal.*(.text .text.* .literal .literal.*)
    *mmu_hal.*(.text .text.* .literal .literal.*)
    *spi_flash_hal_iram.*(.literal .literal.* .text .text.*)
    *spi_flash_encrypt_hal_iram.*(.literal .text .literal.* .text.*)
    *ledc_hal_iram.*(.literal .text .literal.* .text.*)
    *i2c_hal_iram.*(.literal .text .literal.* .text.*)
    *wdt_hal_iram.*(.literal .text .literal.* .text.*)
    *systimer_hal.*(.literal .text .literal.* .text.*)
    *spi_flash_hal_gpspi.*(.literal .literal.* .text .text.*)

    /* [mapping:soc] */
    *libzephyr.a:lldesc.*(.literal .literal.* .text .text.*)

    /* [mapping:log] */
    *(.literal.esp_log_write .text.esp_log_write)
    *(.literal.esp_log_timestamp .text.esp_log_timestamp)
    *(.literal.esp_log_early_timestamp .text.esp_log_early_timestamp)
    *(.literal.esp_log_impl_lock .text.esp_log_impl_lock)
    *(.literal.esp_log_impl_lock_timeout .text.esp_log_impl_lock_timeout)
    *(.literal.esp_log_impl_unlock .text.esp_log_impl_unlock)

    /* [mapping:spi_flash] */
    *spi_flash_chip_boya.*(.literal .literal.* .text .text.*)
    *spi_flash_chip_gd.*(.literal .literal.* .text .text.*)
    *spi_flash_chip_generic.*(.literal .literal.* .text .text.*)
    *spi_flash_chip_issi.*(.literal .literal.* .text .text.*)
    *spi_flash_chip_mxic.*(.literal .literal.* .text .text.*)
    *spi_flash_chip_mxic_opi.*(.literal .literal.* .text .text.*)
    *spi_flash_chip_th.*(.literal .literal.* .text .text.*)
    *spi_flash_chip_winbond.*(.literal .literal.* .text .text.*)
    *memspi_host_driver.*(.literal .literal.* .text .text.*)
    *flash_brownout_hook.*(.literal .literal.* .text .text.*)
    *spi_flash_wrap.*(.literal .literal.* .text .text.*)
    *spi_flash_hpm_enable.*(.literal .literal.* .text .text.*)
    *spi_flash_oct_flash_init*(.literal .literal.* .text .text.*)


    /* [mapping:esp_system] */
    *esp_err.*(.literal .literal.* .text .text.*)
    *(.literal.esp_system_abort .text.esp_system_abort)

    /* [mapping:esp_hw_support] */
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

    /* [mapping:soc_pm] */
    *(.literal.GPIO_HOLD_MASK .text.GPIO_HOLD_MASK)

    /* [mapping:esp_rom] */
    *esp_rom_spiflash.*(.literal .literal.* .text .text.*)
    *esp_rom_systimer.*(.literal .literal.* .text .text.*)
    *esp_rom_wdt.*(.literal .literal.* .text .text.*)
    *spi_flash_rom_patch.*(.literal .text .literal.* .text.*)

    /* [mapping:esp_mm] */
    *esp_cache.*(.literal .literal.* .text .text.*)
    *cache_utils.*(.literal .text .literal.* .text.*)

    . = ALIGN(4) + 16;

  } >iram0_0_seg AT>ROM

  .iram0.pc_test_0 : ALIGN(4)
  {
    _pc_test_0_start = ABSOLUTE(.);
   *(.pc_test_0 .pc_test_0.*)
    _pc_test_0_end = ABSOLUTE(.);
  } >iram0_0_seg AT>ROM

  /* We want to place pc_test_1 in 0x403Bxxxx, where x is the inverted LSB (0-15) from pc_test_0 */
  _pc_test_1_start_calc = (~_pc_test_0_start & 0xBFFFF) + SRAM1_IRAM_START;
  _pc_test_1_gap = _pc_test_1_start_calc - _pc_test_0_end - 2;

  .iram0.pc_test_1 :
  {
    . += _pc_test_1_gap;
    . = ALIGN(4);
    _pc_test_1_start = ABSOLUTE(.);
   *(.pc_test_1 .pc_test_1.*)
    _pc_test_1_end = ABSOLUTE(.);
  } >iram0_0_seg AT>ROM

  .iram0.text_end (NOLOAD) :
  {
    /* C3 memprot requires 512 B alignment for split lines */
    . = ALIGN (16);
    _iram_text_end = ABSOLUTE(.);
  } >iram0_0_seg

  .iram0.data :
  {
    . = ALIGN(16);
    *(.iram.data)
    *(.iram.data*)
  } >iram0_0_seg AT>ROM

  .iram0.bss (NOLOAD) :
  {
    . = ALIGN(16);
    *(.iram.bss)
    *(.iram.bss*)

    . = ALIGN(16);
    _iram_end = ABSOLUTE(.);
  } >iram0_0_seg

  /* --- END OF IRAM --- */

  /* --- START OF DRAM --- */

  .dram0.dummy (NOLOAD):
  {
    /* Spacer section is required to skip .iram0.text area because
     * iram0_0_seg and dram0_0_seg reflect the same address space on different buses.
     */
    . = ORIGIN(dram0_0_seg) + MAX(_iram_end, USER_IRAM_SEG_ORG) - USER_IRAM_SEG_ORG;
    . = ALIGN(16) + 16;
  } >dram0_0_seg

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

    /* All dependent functions should be placed in DRAM to avoid issue
     * when flash cache is disabled */
    *start.*(.rodata .rodata.* .srodata .srodata.*)
    *cbprintf_complete*(.rodata .rodata.* .srodata .srodata.*)
    *log_core.*(.rodata .rodata.* .srodata .srodata.*)
    *log_backend_uart.*(.rodata .rodata.* .srodata .srodata.*)
    *log_output.*(.rodata .rodata.* .srodata .srodata.*)
    *libdrivers__flash.a:flash_esp32.*(.rodata  .rodata.* .srodata .srodata.*)
    *libdrivers__serial.a:uart_esp32.*(.rodata .rodata.* .srodata .srodata.*)
    *spi_flash_rom_patch.*(.rodata  .rodata.* .srodata .srodata.*)
    *periph_ctrl.*(.rodata .rodata.* .srodata .srodata.*)
    *loader.*(.rodata .rodata.* .srodata .srodata.*)
    *cache_utils.*(.rodata .rodata.* .srodata .srodata.*)

    /* [mapping:hal] */
    *mmu_hal.*(.rodata .rodata.* .srodata .srodata.*)
    *mpu_hal.*(.rodata .rodata.* .srodata .srodata.*)
    *spi_flash_hal_iram.*(.rodata .rodata.* .srodata .srodata.*)
    *spi_flash_encrypt_hal_iram.*(.rodata .rodata.* .srodata .srodata.*)
    *cache_hal.*(.rodata .rodata.* .srodata .srodata.*)
    *ledc_hal_iram.*(.rodata .rodata.* .srodata .srodata.*)
    *i2c_hal_iram.*(.rodata .rodata.* .srodata .srodata.*)
    *wdt_hal_iram.*(.rodata .rodata.* .srodata .srodata.*)
    *systimer_hal.*(.rodata .rodata.* .srodata .srodata.*)
    *spi_flash_hal_gpspi.*(.rodata .rodata.* .srodata .srodata.*)

    /* [mapping:soc] */
    *lldesc.*(.rodata .rodata.* .srodata .srodata.*)

    /* [mapping:log] */
    *(.rodata.esp_log_write)
    *(.rodata.esp_log_timestamp)
    *(.rodata.esp_log_early_timestamp)
    *(.rodata.esp_log_impl_lock)
    *(.rodata.esp_log_impl_lock_timeout)
    *(.rodata.esp_log_impl_unlock)

    /* [mapping:spi_flash] */
    *spi_flash_chip_boya.*(.rodata .rodata.* .srodata .srodata.*)
    *spi_flash_chip_gd.*(.rodata .rodata.* .srodata .srodata.*)
    *spi_flash_chip_generic.*(.rodata .rodata.* .srodata .srodata.*)
    *spi_flash_chip_issi.*(.rodata .rodata.* .srodata .srodata.*)
    *spi_flash_chip_mxic.*(.rodata .rodata.* .srodata .srodata.*)
    *spi_flash_chip_mxic_opi.*(.rodata .rodata.* .srodata .srodata.*)
    *spi_flash_chip_th.*(.rodata .rodata.* .srodata .srodata.*)
    *spi_flash_chip_winbond.*(.rodata .rodata.* .srodata .srodata.*)
    *memspi_host_driver.*(.rodata .rodata.* .srodata .srodata.*)
    *flash_brownout_hook.*(.rodata .rodata.* .srodata .srodata.*)
    *spi_flash_wrap.*(.rodata .rodata.* .srodata .srodata.*)
    *spi_flash_hpm_enable.*(.rodata .rodata.* .srodata .srodata.*)
    *spi_flash_oct_flash_init.*(.rodata .rodata.* .srodata .srodata.*)

    /* [mapping:esp_mm] */
    *esp_cache.*(.rodata .rodata.* .srodata .srodata.*)

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
    *systimer.*(.rodata .rodata.* .srodata .srodata.*)
    *mspi_timing_config.*(.rodata .rodata.* .srodata .srodata.*)
    *mspi_timing_tuning.*(.rodata .rodata.* .srodata .srodata.*)
    *(.rodata.sar_periph_ctrl_power_enable)

    /* [mapping:esp_system] */
    *esp_err.*(.rodata .rodata.*)
    *(.rodata.esp_system_abort)

    . = ALIGN(4);

    KEEP(*(.jcr))
    *(.dram1 .dram1.*)
    . = ALIGN(4);
  } >dram0_0_seg AT>ROM

  .dram0.end :
  {
    . = ALIGN(4);
    _end = ABSOLUTE(.);
    _data_end = ABSOLUTE(.);
    __data_end = ABSOLUTE(.);
  } >dram0_0_seg AT>ROM

  .dram0.noinit (NOLOAD):
  {
    . = ALIGN(4);
    _noinit_start = ABSOLUTE(.);
    *(.noinit)
    *(.noinit.*)
    . = ALIGN(4);
    _noinit_end = ABSOLUTE(.);
  } >dram0_0_seg

  /* Shared RAM */
  .dram0.bss (NOLOAD) :
  {
    . = ALIGN (8);
    __bss_start = ABSOLUTE(.);
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
    *(COMMON)
    . = ALIGN (16);
    __bss_end = ABSOLUTE(.);
    _bss_end = ABSOLUTE(.);
  } >dram0_0_seg

  .dram0.safe_ram (NOLOAD):
  {
    . = ALIGN(4);
    _safe_ram_start = ABSOLUTE(.);
    KEEP(*(.dram0.safe_ram))
    _safe_ram_end = ABSOLUTE(.);
  } >dram0_0_seg

  /* define the stack overflow section */
  .dram0.overflow (NOLOAD) :
  {
    . = ALIGN(8);
    /* define the reserved block for stack overflow protection */
    . += 0x1000;
    _stack_overflow_protection_start = .;
    . += 0x4; /* assuming 4 bytes for pattern check */
    _stack_overflow_protection_end = .;
  } >dram0_0_seg

  .dram0.heap_start (NOLOAD):
  {
    . = ALIGN (4);
    _heap_start = ABSOLUTE(.);
    KEEP(*(.dram0.heap_start))
  } > dram0_0_seg

  _dram0_end = ORIGIN(dram0_0_seg) + LENGTH(dram0_0_seg);
  _heap_size = _dram0_end - _heap_start;

  /* Provide total SRAM usage, including IRAM and DRAM */
  _image_ram_start = _iram_start - IRAM_DRAM_OFFSET;

  ASSERT(((__bss_end - ORIGIN(dram0_0_seg)) <= LENGTH(dram0_0_seg)), "DRAM segment data does not fit.")

  /* --- END OF DRAM --- */

  /* --- START OF .rodata --- */

  /* Align next section to 64k to allow mapping */
  .flash.rodata_dummy (NOLOAD) :
  {
    . = ALIGN(CACHE_ALIGN);
  } >ROM

  /* Symbols used during the application memory mapping */
  _image_drom_start = LOADADDR(.flash.rodata);
  _image_drom_size = LOADADDR(.flash.rodata_end) + SIZEOF(.flash.rodata_end) - _image_drom_start;
  _image_drom_vaddr = ADDR(.flash.rodata);

  .flash.rodata : ALIGN(0x10)
  {
    _rodata_reserved_start = ABSOLUTE(.);
    _rodata_start = ABSOLUTE(.);
    _flash_rodata_start = ABSOLUTE(.);

    /* TODO */
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
    /* Literals are also RO data. */
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

  /* This section holds the post build CRC32 calculation of .flash.text */
  .crc_section_text : ALIGN(4)
  {
    _crc_section_text_start = ABSOLUTE(.);
    LONG(0x00000000)
    KEEP(*(.crc_section_text))
    _crc_section_text_end = ABSOLUTE(.);
  } >drom0_0_seg AT>ROM

  /* This section holds the post build CRC32 calculation of .flash.rodata */

  .crc_section_data : ALIGN(4)
  {
    _crc_section_data_start = ABSOLUTE(.);
    LONG(0x00000000)
    KEEP(*(.crc_section_data))
    _crc_section_data_end = ABSOLUTE(.);
  } >drom0_0_seg AT>ROM

  /* Create an explicit section at the end of all the data that shall be mapped into drom.
   * This is used to calculate the size of the _image_drom_size variable */
  .flash.rodata_end : ALIGN(0x10)
  {
    . = ALIGN(4);
    _rodata_reserved_end = ABSOLUTE(.);
    _image_rodata_end = ABSOLUTE(.);
  } >drom0_0_seg AT>ROM

  /* --- END OF .rodata --- */

  /* --- START OF .flash.text --- */

  /* Symbols used during the application memory mapping */
  _image_irom_start = LOADADDR(.flash.text);
  _image_irom_size = SIZEOF(.flash.text);
  _image_irom_vaddr = ADDR(.flash.text);

  .flash.text_dummy (NOLOAD):
  {
    /* Spacer in the IROM address to avoid interfering with the DROM address
     * because DROM and IROM regions share the same address space */
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

    *(.literal .text .literal.* .text.*)
    *(.stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    *(.irom0.text) /* catch stray ICACHE_RODATA_ATTR */
    *(.fini.literal)
    *(.fini)
    *(.gnu.version)

    _pc_test_2_start = ABSOLUTE(.);
    *(.pc_test_2 .pc_test_2.*)
    _pc_test_2_end = ABSOLUTE(.);

    /** CPU will try to prefetch up to 16 bytes of
      * of instructions. This means that any configuration (e.g. MMU, PMS) must allow
      * safe access to up to 16 bytes after the last real instruction, add
      * dummy bytes to ensure this
      */
    . += 16;

    _instruction_reserved_end = ABSOLUTE(.);
    _text_end = ABSOLUTE(.);
    _instruction_reserved_end = ABSOLUTE(.);
    _etext = .;

    /**
     * Similar to _iram_start, this symbol goes here so it is
     * resolved by addr2line in preference to the first symbol in
     * the flash.text segment.
     */
    _flash_cache_start = ABSOLUTE(0);
    _flash_text_end = ABSOLUTE(.);

  } >irom0_0_seg AT>ROM

  /* --- END OF .flash.text --- */
}

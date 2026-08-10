# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0
#
# Single Kconfig → STL source mapping for all integration shapes.
#
# Paths returned are relative to src/bist/.
# Kconfig (depends on / defaults) is the only filter for which tests build.
#
#   bist_include_dirs(out_var)
#   bist_collect_sources(out_var)

function(bist_include_dirs out_var)
    set(_incs
        "ulp_include"
        "include"
        "core/include"
        "core/cpu/include"
        "core/memory/include"
        "core/clock/include"
        "core/wdt/include"
        "core/io/include"
        "core/interrupt/include"
        "drivers/include"
    )
    set(${out_var} "${_incs}" PARENT_SCOPE)
endfunction()

function(bist_collect_sources out_var)
    set(_srcs "")

    if(CONFIG_ESP_BIST_CPU_REG_TEST)
        list(APPEND _srcs "core/cpu/bist_cpu_regs.c")
    endif()
    if(CONFIG_ESP_BIST_CPU_CSR_REG_TEST)
        list(APPEND _srcs "core/cpu/bist_cpu_csr_regs.c")
    endif()
    if(CONFIG_ESP_BIST_STACK_TEST)
        list(APPEND _srcs "core/cpu/bist_cpu_stack.c")
    endif()
    if(CONFIG_ESP_BIST_MEMORY_RAM_TEST)
        list(APPEND _srcs "core/memory/bist_ram.c")
    endif()
    if(CONFIG_ESP_BIST_MEMORY_FLASH_TEST)
        list(APPEND _srcs
            "bist_hd_crc.c"
            "core/memory/bist_flash.c"
        )
    endif()
    if(CONFIG_ESP_BIST_CLOCK_TEST)
        list(APPEND _srcs "core/clock/bist_clock_fail.c")
    endif()
    if(CONFIG_ESP_BIST_GPIO_TEST)
        list(APPEND _srcs "core/io/bist_gpio.c")
    endif()
    if(CONFIG_ESP_BIST_ADC_TEST)
        list(APPEND _srcs "core/io/bist_adc.c")
    endif()
    if(CONFIG_ESP_BIST_PROGRAM_COUNTER_TEST)
        list(APPEND _srcs "core/cpu/bist_pc.c")
    endif()
    if(CONFIG_ESP_BIST_WATCHDOG_TEST)
        list(APPEND _srcs "core/wdt/bist_wdt.c")
    endif()
    if(CONFIG_ESP_BIST_INTERRUPT_TEST)
    list(APPEND
         _srcs
         core/interrupt/bist_interrupt.c
         core/interrupt/bist_interrupt_sw.c
         )
    endif()
    set(${out_var} "${_srcs}" PARENT_SCOPE)
endfunction()

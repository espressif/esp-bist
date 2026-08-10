############################################################################
# src/bist/nuttx.mk
#
# NuttX LP-core build fragment for ESP-BIST.
# Include from a ULP app Makefile before esp_ulp.mk.
#
# Required:
#   ESP_BIST_ROOT  - path to esp-bist repository root
#
# SPDX-License-Identifier: LGPL-3.0-or-later
############################################################################

ifeq ($(ESP_BIST_ROOT),)
$(error ESP_BIST_ROOT must be set to the esp-bist repository root)
endif

ifeq ($(CHIP_SERIES),)
CHIP_SERIES = $(patsubst "%",%,$(CONFIG_ESPRESSIF_CHIP_SERIES))
endif

BIST_DIR = $(ESP_BIST_ROOT)/src/bist

# Absolute paths (esp_ulp.mk leaves absolute ULP_APP_C_SRCS unprefixed)

ULP_APP_C_SRCS += $(BIST_DIR)/drivers/lp_wdt.c

ifeq ($(CONFIG_ESP_BIST_CPU_REG_TEST),y)
ULP_APP_C_SRCS += $(BIST_DIR)/core/cpu/bist_cpu_regs.c
endif
ifeq ($(CONFIG_ESP_BIST_CPU_CSR_REG_TEST),y)
ULP_APP_C_SRCS += $(BIST_DIR)/core/cpu/bist_cpu_csr_regs.c
endif
ifeq ($(CONFIG_ESP_BIST_STACK_TEST),y)
ULP_APP_C_SRCS += $(BIST_DIR)/core/cpu/bist_cpu_stack.c
endif
ifeq ($(CONFIG_ESP_BIST_MEMORY_RAM_TEST),y)
ULP_APP_C_SRCS += $(BIST_DIR)/core/memory/bist_ram.c
endif
ifeq ($(CONFIG_ESP_BIST_MEMORY_FLASH_TEST),y)
ULP_APP_C_SRCS += $(BIST_DIR)/bist_hd_crc.c
ULP_APP_C_SRCS += $(BIST_DIR)/core/memory/bist_flash.c
endif

# Trailing / so $(dir ...) keeps the include directory

ULP_APP_INCLUDES += \
	$(BIST_DIR)/ulp_include/ \
	$(BIST_DIR)/include/ \
	$(BIST_DIR)/core/include/ \
	$(BIST_DIR)/core/cpu/include/ \
	$(BIST_DIR)/core/memory/include/ \
	$(BIST_DIR)/core/clock/include/ \
	$(BIST_DIR)/core/wdt/include/ \
	$(BIST_DIR)/core/io/include/ \
	$(BIST_DIR)/core/interrupt/include/ \
	$(BIST_DIR)/drivers/include/ \
	$(ESP_BIST_ROOT)/src/soc/$(CHIP_SERIES)/include/

ifeq ($(CONFIG_ESP_BIST_HOST_DIAGNOSTICS),y)
ULP_APP_C_SRCS += $(BIST_DIR)/bist_hd_protocol.c
ULP_APP_C_SRCS += $(BIST_DIR)/bist_hd_challenge.c
ULP_APP_C_SRCS += $(BIST_DIR)/companion/bist_hd_companion.c
ULP_APP_C_SRCS += $(BIST_DIR)/companion/nuttx/bist_hd_comp_port_nuttx.c
ULP_APP_INCLUDES += \
	$(BIST_DIR)/companion/include/
endif

ULP_CUSTOM_SECTIONS_LD = $(ESP_BIST_ROOT)/src/soc/$(CHIP_SERIES)/ld/nuttx.ld

ifeq ($(CONFIG_ESP_BIST_MEMORY_FLASH_TEST),y)
define ULP_POST_LINK
	$(Q) echo "Injecting BIST CRC32 into ULP ELF"
	$(Q) OBJCOPY=$(OBJCOPY) python3 $(ESP_BIST_ROOT)/scripts/calculate_crc32.py $(ULP_ELF_FILE) .text .crc_section_text
	$(Q) OBJCOPY=$(OBJCOPY) python3 $(ESP_BIST_ROOT)/scripts/calculate_crc32.py $(ULP_ELF_FILE) .rodata .crc_section_data
endef
endif

ULP_EXTRA_DEFINES += -DNUTTX_ESP_BIST_MODULE
ULP_EXTRA_DEFINES += -DSOC_TARGET_$(shell echo $(CHIP_SERIES) | tr 'a-z' 'A-Z')

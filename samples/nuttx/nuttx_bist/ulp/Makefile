############################################################################
# samples/nuttx/nuttx_bist/ulp/Makefile
#
# SPDX-License-Identifier: Apache-2.0
############################################################################

# esp-bist repository root (samples/nuttx/nuttx_bist/ulp -> ../../..)
ESP_BIST_ROOT = $(abspath $(CURDIR)/../../..)

ifeq ($(CHIP_SERIES),)
CHIP_SERIES = $(patsubst "%",%,$(CONFIG_ESPRESSIF_CHIP_SERIES))
endif

ULP_APP_NAME = nuttx_bist
ULP_APP_FOLDER = $(APPDIR)/esp_bist/nuttx_bist/ulp
ULP_APP_C_SRCS = main.c
ULP_APP_INCLUDES = $(APPDIR)/esp_bist/nuttx_bist/bist_protocol.h

include $(ESP_BIST_ROOT)/src/bist/nuttx.mk
include $(TOPDIR)/arch/$(CONFIG_ARCH)/src/common/espressif/esp_ulp.mk

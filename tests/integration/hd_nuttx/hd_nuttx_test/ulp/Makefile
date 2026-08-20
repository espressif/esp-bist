############################################################################
# tests/integration/hd_nuttx/hd_nuttx_test/ulp/Makefile
#
# SPDX-License-Identifier: Apache-2.0
############################################################################

# esp-bist repository root (tests/integration/hd_nuttx/hd_nuttx_test/ulp -> ../../../../..)
ESP_BIST_ROOT = $(abspath $(CURDIR)/../../../../..)

ifeq ($(CHIP_SERIES),)
CHIP_SERIES = $(patsubst "%",%,$(CONFIG_ESPRESSIF_CHIP_SERIES))
endif

ULP_APP_NAME = hd_nuttx_test
ULP_APP_FOLDER = $(APPDIR)/external/hd_nuttx_test/ulp
ULP_APP_C_SRCS = main.c
ULP_APP_INCLUDES = $(APPDIR)/external/hd_nuttx_test

include $(ESP_BIST_ROOT)/src/bist/nuttx.mk
include $(TOPDIR)/arch/$(CONFIG_ARCH)/src/common/espressif/esp_ulp.mk

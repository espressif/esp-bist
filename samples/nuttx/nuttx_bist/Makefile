############################################################################
# samples/nuttx/nuttx_bist/Makefile
#
# SPDX-License-Identifier: Apache-2.0
############################################################################

include $(APPDIR)/Make.defs

PROGNAME  = $(CONFIG_ESP_NUTTX_BIST_PROGNAME)
PRIORITY  = $(CONFIG_ESP_NUTTX_BIST_PRIORITY)
STACKSIZE = $(CONFIG_ESP_NUTTX_BIST_STACKSIZE)
MODULE    = $(CONFIG_ESP_NUTTX_BIST)

MAINSRC = nuttx_bist_main.c

CFLAGS += $(INCDIR_PREFIX)$(APPDIR)/esp_bist/nuttx_bist

include $(APPDIR)/Application.mk

include $(APPDIR)/Make.defs

include ulp/Makefile

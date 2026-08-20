############################################################################
# tests/integration/hd_nuttx/hd_nuttx_test/Makefile
#
# SPDX-License-Identifier: Apache-2.0
############################################################################

include $(APPDIR)/Make.defs

PROGNAME  = $(CONFIG_ESP_NUTTX_HD_TEST_PROGNAME)
PRIORITY  = $(CONFIG_ESP_BIST_HD_AGENT_TASK_PRIO_NUTTX)
STACKSIZE = $(CONFIG_ESP_BIST_HD_AGENT_TASK_STACK)
MODULE    = $(CONFIG_ESP_NUTTX_HD_TEST)

MAINSRC = hd_nuttx_test_main.c

CFLAGS += $(INCDIR_PREFIX)$(APPDIR)/external/hd_nuttx_test

include $(APPDIR)/Application.mk

include $(APPDIR)/Make.defs

include ulp/Makefile

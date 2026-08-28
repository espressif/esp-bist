# ##############################################################################
# tests/integration/hd_nuttx/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
# ##############################################################################

nuttx_add_subdirectory()
nuttx_generate_kconfig(MENUDESC "ESP-BIST HD fail-closed")

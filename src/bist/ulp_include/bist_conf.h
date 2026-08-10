/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * BIST Kconfig options come from the parent project's generated configuration
 * header. Standalone builds use a generated bist_conf.h from
 * cmake/kconfig.cmake instead.
 *
 * Zephyr: CONFIG_* macros are injected via -imacros by the build system,
 * so no explicit include is needed.
 * NuttX:  CONFIG_* macros come from nuttx/config.h.
 * IDF:    CONFIG_* macros come from sdkconfig.h (via SDKCONFIG_HEADER).
 */
#pragma once

#if !defined(__ZEPHYR__)
#if defined(NUTTX_ESP_BIST_MODULE)
#include "nuttx/config.h"
#else
#include "sdkconfig.h"
#endif
#endif

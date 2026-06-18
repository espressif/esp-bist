/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * This file is part of Espressif's BIST (Built-In Self Test) Library.
 *
 * BIST library is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * BIST library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with BIST library. If not, see
 * <https://www.gnu.org/licenses/lgpl-3.0.html>.
 */

#include "lp_wdt.h"
#include "hal/lpwdt_ll.h"
#include "soc/lp_wdt_struct.h"

/*
 * LP WDT for safety-critical systems (ESP32-C6):
 *
 * If the LP core fails to feed the watchdog within the configured timeout,
 * the LP WDT triggers a full system reset (WDT_STAGE_ACTION_RESET_SYSTEM).
 * This is the appropriate behavior for safety-critical applications — any
 * failure in the BIST execution path must bring the entire system to a
 * known-good state.
 *
 * Uses the lpwdt_ll API which correctly handles the eFuse-based implicit
 * multiplier on stage 0 timeout configuration.
 */

/* LP WDT is clocked by the RTC slow clock. The source depends on sdkconfig:
 *   - RC_SLOW (default): ~136 kHz (SOC_CLK_RC_SLOW_FREQ_APPROX)
 *   - XTAL32K or RC32K:   ~32 kHz */
#if defined(SOC_CLK_RC_SLOW_FREQ_APPROX) && !defined(CONFIG_RTC_CLK_SRC_EXT_CRYS) && !defined(CONFIG_RTC_CLK_SRC_INT_RC32K)
#define LP_SLOW_CLK_FREQ_HZ    SOC_CLK_RC_SLOW_FREQ_APPROX
#else
#define LP_SLOW_CLK_FREQ_HZ    32768
#endif

void lp_wdt_init(uint32_t timeout_us)
{
    uint32_t timeout_ticks = (uint32_t)((uint64_t)timeout_us * LP_SLOW_CLK_FREQ_HZ / 1000000);

    lpwdt_ll_write_protect_disable(&LP_WDT);
    lpwdt_ll_disable(&LP_WDT);

    lpwdt_ll_config_stage(&LP_WDT, WDT_STAGE0, timeout_ticks, WDT_STAGE_ACTION_RESET_SYSTEM);
    lpwdt_ll_disable_stage(&LP_WDT, WDT_STAGE1);
    lpwdt_ll_disable_stage(&LP_WDT, WDT_STAGE2);
    lpwdt_ll_disable_stage(&LP_WDT, WDT_STAGE3);

    lpwdt_ll_set_flashboot_en(&LP_WDT, false);
    lpwdt_ll_set_pause_in_sleep_en(&LP_WDT, false);

    lpwdt_ll_enable(&LP_WDT);
    lpwdt_ll_feed(&LP_WDT);
    lpwdt_ll_write_protect_enable(&LP_WDT);
}

void lp_wdt_feed(void)
{
    lpwdt_ll_write_protect_disable(&LP_WDT);
    lpwdt_ll_feed(&LP_WDT);
    lpwdt_ll_write_protect_enable(&LP_WDT);
}

void lp_wdt_disable(void)
{
    lpwdt_ll_write_protect_disable(&LP_WDT);
    lpwdt_ll_disable(&LP_WDT);
    lpwdt_ll_write_protect_enable(&LP_WDT);
}

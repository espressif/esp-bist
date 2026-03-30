#include <stdio.h>
#include "bist_esp.h"
#include "unity.h"
#include "wdt.h"
#include "rom/ets_sys.h"
#include "esp_rom_sys.h"

#define WINDOWED_WDT_OVERFLOW_TIMEOUT_US   50000
#define WINDOWED_WDT_UNDERFLOW_TIMEOUT_US  2000
#define WINDOWED_WDT_FEED_MARGIN_US        1000
#define WINDOWED_WDT_CONSECUTIVE_CYCLES    10

void setUp(void)
{
    TEST_ASSERT_EQUAL(0, wdt_init(WINDOWED_WDT_OVERFLOW_TIMEOUT_US));
    TEST_ASSERT_EQUAL(0, wdt_init_windowed(WINDOWED_WDT_UNDERFLOW_TIMEOUT_US));
}

void tearDown(void)
{
    wdt_windowed_deinit();
    wdt_deinit();
}

void test_BIST_WINDOWED_WDT_NORMAL(void)
{
    ets_delay_us(WINDOWED_WDT_UNDERFLOW_TIMEOUT_US + WINDOWED_WDT_FEED_MARGIN_US);
    wdt_feed();
    TEST_ASSERT_FALSE(wdt_is_underflow_detected());

    ets_delay_us(WINDOWED_WDT_UNDERFLOW_TIMEOUT_US + WINDOWED_WDT_FEED_MARGIN_US);
    wdt_feed();
    TEST_ASSERT_FALSE(wdt_is_underflow_detected());
}

void test_BIST_WINDOWED_WDT_UNDERFLOW(void)
{
    /*
     * First feed is permitted because wdt_init_windowed() starts with
     * window_open_flag = true. After this feed the flag is cleared and
     * the underflow timer is restarted.
     */
    wdt_feed();

    /*
     * Feed again immediately -- well before the underflow timer can fire.
     * The driver must detect this as an underflow violation.
     */
    ets_delay_us(100);
    wdt_feed();

    TEST_ASSERT_TRUE(wdt_is_underflow_detected());
}

void test_BIST_WINDOWED_WDT_CONSECUTIVE(void)
{
    for (int i = 0; i < WINDOWED_WDT_CONSECUTIVE_CYCLES; i++) {
        ets_delay_us(WINDOWED_WDT_UNDERFLOW_TIMEOUT_US + WINDOWED_WDT_FEED_MARGIN_US);
        wdt_feed();
        TEST_ASSERT_FALSE(wdt_is_underflow_detected());
    }
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_BIST_WINDOWED_WDT_NORMAL);
    RUN_TEST(test_BIST_WINDOWED_WDT_UNDERFLOW);
    RUN_TEST(test_BIST_WINDOWED_WDT_CONSECUTIVE);
    return UNITY_END();
}

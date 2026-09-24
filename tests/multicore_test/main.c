#include <stdint.h>
#include "rom/ets_sys.h"
#include "bist_esp.h"
#include "unity.h"

#if defined(SOC_TARGET_ESP32P4)
#include "bist_multicore.h"
#include "esp_cpu.h"

typedef struct {
    uint32_t input;
    uint32_t output;
    uint32_t callback_core;
} multicore_test_args_t;

static void multicore_test_callback(void *arg)
{
    multicore_test_args_t *test_args = (multicore_test_args_t *)arg;

    test_args->output = test_args->input ^ 0xa5a55a5aU;
    test_args->callback_core = esp_cpu_get_core_id();
}

void test_BIST_Multicore_Dispatch(void)
{
    multicore_test_args_t args;

    for (uint32_t i = 0; i < 64; i++) {
        args = (multicore_test_args_t) {
            .input = i,
            .output = 0,
            .callback_core = UINT32_MAX,
        };

        /* Give core 1 time to return to its WFI idle path between posts. */
        ets_delay_us(100);
        TEST_ASSERT_EQUAL(BIST_ESP_OK, bist_multicore_call(1, multicore_test_callback, &args));
        TEST_ASSERT_EQUAL_UINT32(i ^ 0xa5a55a5aU, args.output);
        TEST_ASSERT_EQUAL_UINT32(1, args.callback_core);
    }

    args.callback_core = UINT32_MAX;
    TEST_ASSERT_EQUAL(BIST_ESP_OK, bist_multicore_call(0, multicore_test_callback, &args));
    TEST_ASSERT_EQUAL_UINT32(0, args.callback_core);
}
#endif

void setUp(void) {}
void tearDown(void) {}

int main()
{
    UNITY_BEGIN();
#if defined(SOC_TARGET_ESP32P4)
    RUN_TEST(test_BIST_Multicore_Dispatch);
#endif
    return UNITY_END();
}

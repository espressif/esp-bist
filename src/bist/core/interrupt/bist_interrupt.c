#include <stdbool.h>
#include <stdint.h>
#include "bist_interrupt.h"
#include "esp_cpu.h"
#include "bist_log.h"
#include "hal/timer_hal.h"
#include "hal/timer_ll.h"
#include "hal/timg_ll.h"
#include "hal/timer_periph.h"
#include "esp_rom_sys.h"
#include "esp_private/periph_ctrl.h"

#if TIMG_LL_INST_NUM < 2
#  error "bist_hardware_interrupt_test requires two Timer Groups (TIMG0 and TIMG1)"
#endif

#define BIST_HARDWARE_INTERRUPT_PERIOD_US_1  500
#define BIST_HARDWARE_INTERRUPT_PERIOD_US_2  1000
#define BIST_HARDWARE_INTERRUPT_DELAY_US     (BIST_HARDWARE_INTERRUPT_PERIOD_US_2 * 5)

#define BIST_HW_TIMER_CPU_INTR_0 9
#define BIST_HW_TIMER_CPU_INTR_1 10
#define BIST_HW_TIMER_COUNT      TIMG_LL_INST_NUM
#define BIST_HW_CHECK_EVERY_N \
    (BIST_HARDWARE_INTERRUPT_DELAY_US / BIST_HARDWARE_INTERRUPT_PERIOD_US_2)

typedef struct {
    int group_id;
    int cpu_intr;
    uint32_t timer_id;
    uint32_t intr_source;
    uint32_t period_us;
    timer_hal_context_t hal;
} bist_hw_timer_t;

static const char *TAG = "bist_irq_hw";

static uint32_t hw_interrupt_count_1 = 0;
static uint32_t hw_interrupt_count_2 = 0;
static uint32_t hw_check_divider = 0;
static bist_hw_timer_t hw_timers[BIST_HW_TIMER_COUNT];
static bool hw_interrupt_test_failed = false;

static void bist_hw_timers_deinit(void);

static void bist_hw_interrupt_check_ratio(void)
{
    uint32_t expected = 2 * hw_interrupt_count_2;
    uint32_t diff = (hw_interrupt_count_1 > expected)
                    ? (hw_interrupt_count_1 - expected)
                    : (expected - hw_interrupt_count_1);
    if (diff > 1U) {
        hw_interrupt_test_failed = true;
    }
}

static void bist_hw_timer_ack(bist_hw_timer_t *timer)
{
    uint32_t event = TIMER_LL_EVENT_ALARM(timer->timer_id);

    if (timer_ll_get_intr_status(timer->hal.dev) & event) {
        timer_ll_clear_intr_status(timer->hal.dev, event);
        /* Alarm auto-disables in hardware; re-arm for periodic mode. */
        timer_ll_enable_alarm(timer->hal.dev, timer->timer_id, true);
    }
}

static void bist_hw_timer0_isr(void *arg)
{
    bist_hw_timer_t *timer = (bist_hw_timer_t *)arg;
    bist_hw_timer_ack(timer);
    hw_interrupt_count_1++;
}

static void bist_hw_timer1_isr(void *arg)
{
    bist_hw_timer_t *timer = (bist_hw_timer_t *)arg;
    bist_hw_timer_ack(timer);
    hw_interrupt_count_2++;
    hw_check_divider++;
    if (hw_check_divider >= BIST_HW_CHECK_EVERY_N) {
        hw_check_divider = 0;
        bist_hw_interrupt_check_ratio();
    }
}

static bist_esp_err_t bist_hw_timer_start(bist_hw_timer_t *timer)
{
    const uint32_t core_id = esp_cpu_get_core_id();
    const uint32_t divider = CONFIG_XTAL_FREQ; /* XTAL_MHz -> 1 MHz (1 tick = 1 us) */
    const uint32_t cpu_intr_mask = 1U << timer->cpu_intr;
    esp_cpu_intr_handler_t isr = (timer->group_id == 0) ? bist_hw_timer0_isr : bist_hw_timer1_isr;

    if (divider < 2U) {
        ESP_LOGE(TAG, "Invalid XTAL divider %u", (unsigned)divider);
        return BIST_ESP_INTERRUPT_TEST_ERR;
    }

#ifdef __PERIPH_CTRL_ALLOW_LEGACY_API
    periph_module_enable((timer->group_id == 0) ? PERIPH_TIMG0_MODULE : PERIPH_TIMG1_MODULE);
#else
    /* Avoid resetting the group: TIMG0 may already host MWDT. */
    _timg_ll_enable_bus_clock(timer->group_id, true);
#endif

    timer_hal_init(&timer->hal, timer->group_id, timer->timer_id);

    PERIPH_RCC_ATOMIC() {
        timer_ll_enable_clock(timer->group_id, timer->timer_id, true);
        timer_ll_set_clock_source(timer->group_id, timer->timer_id, GPTIMER_CLK_SRC_XTAL);
    }
    timer_ll_set_clock_prescale(timer->hal.dev, timer->timer_id, divider);
    timer_ll_set_count_direction(timer->hal.dev, timer->timer_id, GPTIMER_COUNT_UP);
    timer_ll_enable_auto_reload(timer->hal.dev, timer->timer_id, true);
    timer_ll_set_reload_value(timer->hal.dev, timer->timer_id, 0);
    timer_hal_set_counter_value(&timer->hal, 0);
    timer_ll_set_alarm_value(timer->hal.dev, timer->timer_id, timer->period_us);
    timer_ll_enable_intr(timer->hal.dev, TIMER_LL_EVENT_ALARM(timer->timer_id), false);
    timer_ll_clear_intr_status(timer->hal.dev, TIMER_LL_EVENT_ALARM(timer->timer_id));

    esp_cpu_intr_disable(cpu_intr_mask);
    esp_rom_route_intr_matrix(core_id, timer->intr_source, timer->cpu_intr);
    esp_cpu_intr_set_type(timer->cpu_intr, ESP_CPU_INTR_TYPE_LEVEL);
    esp_cpu_intr_set_priority(timer->cpu_intr, SOC_INTERRUPT_LEVEL_MEDIUM);
    esp_cpu_intr_set_handler(timer->cpu_intr, isr, timer);
    esp_cpu_intr_enable(cpu_intr_mask);

    timer_ll_enable_intr(timer->hal.dev, TIMER_LL_EVENT_ALARM(timer->timer_id), true);
    timer_ll_enable_alarm(timer->hal.dev, timer->timer_id, true);
    timer_ll_enable_counter(timer->hal.dev, timer->timer_id, true);

    ESP_LOGD(TAG, "TIMG%d T%d period %u us -> CPU intr %d",
             timer->group_id, (int)timer->timer_id, (unsigned)timer->period_us, timer->cpu_intr);
    return BIST_ESP_OK;
}

static void bist_hw_timer_deinit(bist_hw_timer_t *timer)
{
    const uint32_t cpu_intr_mask = 1U << timer->cpu_intr;

    if (timer->hal.dev == NULL) {
        return;
    }

    timer_ll_enable_counter(timer->hal.dev, timer->timer_id, false);
    timer_ll_enable_alarm(timer->hal.dev, timer->timer_id, false);
    timer_ll_clear_intr_status(timer->hal.dev, TIMER_LL_EVENT_ALARM(timer->timer_id));
    timer_ll_enable_intr(timer->hal.dev, TIMER_LL_EVENT_ALARM(timer->timer_id), false);
    esp_cpu_intr_disable(cpu_intr_mask);
    esp_rom_route_intr_matrix(esp_cpu_get_core_id(), timer->intr_source, ETS_INVALID_INUM);
    timer_hal_deinit(&timer->hal);
}

static void bist_hw_timers_deinit(void)
{
    for (int i = 0; i < BIST_HW_TIMER_COUNT; i++) {
        bist_hw_timer_deinit(&hw_timers[i]);
    }
}

static bist_esp_err_t bist_hardware_interrupt_test_init(void)
{
    bist_esp_err_t ret;
    hw_check_divider = 0;
    hw_interrupt_count_1 = 0;
    hw_interrupt_count_2 = 0;
    hw_interrupt_test_failed = false;

    hw_timers[0] = (bist_hw_timer_t) {
        .group_id = 0,
        .cpu_intr = BIST_HW_TIMER_CPU_INTR_0,
        .timer_id = 0,
        .intr_source = soc_timg_gptimer_signals[0][0].irq_id,
        .period_us = BIST_HARDWARE_INTERRUPT_PERIOD_US_1,
    };
    hw_timers[1] = (bist_hw_timer_t) {
        .group_id = 1,
        .cpu_intr = BIST_HW_TIMER_CPU_INTR_1,
        .timer_id = 0,
        .intr_source = soc_timg_gptimer_signals[1][0].irq_id,
        .period_us = BIST_HARDWARE_INTERRUPT_PERIOD_US_2,
    };

    for (int i = 0; i < BIST_HW_TIMER_COUNT; i++) {
        ret = bist_hw_timer_start(&hw_timers[i]);
        if (ret != BIST_ESP_OK) {
            ESP_LOGE(TAG, "Failed to start TIMG hardware timer %d", i);
            bist_hw_timers_deinit();
            return BIST_ESP_INTERRUPT_TEST_ERR;
        }
    }

    ESP_LOGD(TAG, "Hardware interrupt setup complete: TIMG0/TIMG1, periods %u/%u us",
             (unsigned)BIST_HARDWARE_INTERRUPT_PERIOD_US_1,
             (unsigned)BIST_HARDWARE_INTERRUPT_PERIOD_US_2);

    return BIST_ESP_OK;
}

bist_esp_err_t bist_hardware_interrupt_test(void)
{
    int ret = BIST_ESP_OK;

    ret = bist_hardware_interrupt_test_init();
    if (ret != BIST_ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize hardware interrupt test");
        return BIST_ESP_INTERRUPT_TEST_ERR;
    }

    do {
        esp_rom_delay_us(10);
        if (hw_interrupt_test_failed) {
            break;
        }
    } while (hw_interrupt_count_1 < 1000 && hw_interrupt_count_2 < 1000);

    bist_hw_timers_deinit();

    if (hw_interrupt_test_failed) {
        ESP_LOGE(TAG, "Hardware interrupt test failed: cnt1 %d, cnt2 %d",
                 hw_interrupt_count_1, hw_interrupt_count_2);
        return BIST_ESP_INTERRUPT_TEST_ERR;
    }

    return BIST_ESP_OK;
}

/*
 * SPDX-FileCopyrightText: 2017-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/param.h>
#include <string.h>
#include "soc/soc.h"
#include "esp_types.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "bist_log.h"
#include "bist_conf.h"
#include "esp_ipc.h"
#include "esp_timer.h"

#include "esp_private/startup_internal.h"
#include "esp_private/system_internal.h"

#include "sdkconfig.h"

#if CONFIG_IDF_TARGET_ESP32C3
#include "esp32c3/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32C6
#include "esp32c6/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32H2
#include "esp32h2/rtc.h"
#endif

#include "esp_private/systimer.h"
#include "hal/systimer_ll.h"
#include "hal/systimer_types.h"
#include "hal/systimer_hal.h"

#include "soc/periph_defs.h"
#include "esp_private/periph_ctrl.h"

#define ETS_INTERNAL_TIMER0_INTR_NO 6

/* Systimer HAL layer object */
static systimer_hal_context_t systimer_hal;
uint64_t timestamp_id[2] = { UINT64_MAX, UINT64_MAX };


#ifndef NDEBUG
// Enable built-in checks in queue.h in debug builds
#define INVARIANTS
#endif
#include "sys/queue.h"

#define EVENT_ID_DELETE_TIMER   0xF0DE1E1E

typedef enum {
    FL_ISR_DISPATCH_METHOD   = (1 << 0),  //!< 0=Callback is called from timer task, 1=Callback is called from timer ISR
    FL_SKIP_UNHANDLED_EVENTS = (1 << 1),  //!< 0=NOT skip unhandled events for periodic timers, 1=Skip unhandled events for periodic timers
} flags_t;

struct esp_timer {
    uint64_t alarm;
    uint64_t period:56;
    flags_t flags:8;
    union {
        esp_timer_cb_t callback;
        uint32_t event_id;
    };
    void* arg;
    LIST_ENTRY(esp_timer) list_entry;
};

static esp_err_t timer_insert(esp_timer_handle_t timer, bool without_update_alarm);
static esp_err_t timer_remove(esp_timer_handle_t timer);
static bool timer_armed(esp_timer_handle_t timer);

__attribute__((unused)) static const char* TAG = "esp_timer";

// lists of currently armed timers for two dispatch methods: ISR and TASK
static LIST_HEAD(esp_timer_list, esp_timer) s_timers[ESP_TIMER_MAX] = {
    [0 ... (ESP_TIMER_MAX - 1)] = LIST_HEAD_INITIALIZER(s_timers)
};


esp_err_t esp_timer_create(const esp_timer_create_args_t* args,
                           esp_timer_handle_t* out_handle)
{

    if (args == NULL || args->callback == NULL || out_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_timer_handle_t result = (esp_timer_handle_t) calloc(1, sizeof(*result));
    if (result == NULL) {
        return ESP_ERR_NO_MEM;
    }

    result->callback = args->callback;
    result->arg = args->arg;
    result->flags = (args->dispatch_method ? FL_ISR_DISPATCH_METHOD : 0) |
                    (args->skip_unhandled_events ? FL_SKIP_UNHANDLED_EVENTS : 0);
    *out_handle = result;
    return ESP_OK;
}

static uint64_t IRAM_ATTR esp_timer_impl_get_min_period_us(void)
{
    return 50;
}

/*
 * We have placed this function in IRAM to ensure consistency with the esp_timer API.
 * esp_timer_start_once, esp_timer_start_periodic and esp_timer_stop are in IRAM.
 * But actually in IDF esp_timer_restart is used only in one place, which requires keeping
 * in IRAM when PM_SLP_IRAM_OPT = y and ESP_TASK_WDT USE ESP_TIMER = y.
*/
esp_err_t IRAM_ATTR esp_timer_restart(esp_timer_handle_t timer, uint64_t timeout_us)
{
    esp_err_t ret = ESP_OK;

    if (timer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!timer_armed(timer)) {
        return ESP_ERR_INVALID_STATE;
    }

    const int64_t now = systimer_hal.ticks_to_us(systimer_hal_get_counter_value(&systimer_hal, SYSTIMER_COUNTER_ESPTIMER));
    const uint64_t period = timer->period;

    /* We need to remove the timer to the list of timers and reinsert it at
     * the right position. In fact, the timers are sorted by their alarm value
     * (earliest first) */
    ret = timer_remove(timer);

    if (ret == ESP_OK) {
        /* Two cases here:
         * - if the alarm was a periodic one, i.e. `period` is not 0, the given timeout_us becomes the new period
         * - if the alarm was a one-shot one, i.e. `period` is 0, it remains non-periodic. */
        if (period != 0) {
            /* Remove function got rid of the alarm and period fields, restore them */
            const uint64_t new_period = MAX(timeout_us, esp_timer_impl_get_min_period_us());
            timer->alarm = now + new_period;
            timer->period = new_period;
        } else {
            /* The new one-shot alarm shall be triggered timeout_us after the current time */
            timer->alarm = now + timeout_us;
            timer->period = 0;
        }
        ret = timer_insert(timer, false);
    }

    return ret;
}

esp_err_t IRAM_ATTR esp_timer_start_once(esp_timer_handle_t timer, uint64_t timeout_us)
{
    if (timer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int64_t alarm = systimer_hal.ticks_to_us(systimer_hal_get_counter_value(&systimer_hal, SYSTIMER_COUNTER_ESPTIMER)) + timeout_us;
    esp_err_t err;

    /* Check if the timer is armed once the list is locked.
     * Otherwise another task may arm the timer inbetween the check
     * and us locking the list, resulting in us inserting the
     * timer to s_timers a second time. This will create a loop
     * in s_timers. */
    if (timer_armed(timer)) {
        err = ESP_ERR_INVALID_STATE;
    } else {
        timer->alarm = alarm;
        timer->period = 0;
        err = timer_insert(timer, false);
    }
    return err;
}

esp_err_t esp_timer_start_periodic(esp_timer_handle_t timer, uint64_t period_us)
{
    if (timer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    period_us = MAX(period_us, esp_timer_impl_get_min_period_us());
    int64_t alarm = systimer_hal.ticks_to_us(systimer_hal_get_counter_value(&systimer_hal, SYSTIMER_COUNTER_ESPTIMER)) + period_us;
    esp_err_t err;

    /* Check if the timer is armed once the list is locked to avoid a data race */
    if (timer_armed(timer)) {
        err = ESP_ERR_INVALID_STATE;
    } else {
        timer->alarm = alarm;
        timer->period = period_us;
        err = timer_insert(timer, false);
    }
    return err;
}

esp_err_t IRAM_ATTR esp_timer_stop(esp_timer_handle_t timer)
{
    if (timer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err;

    /* Check if the timer is armed once the list is locked to avoid a data race */
    if (!timer_armed(timer)) {
        err = ESP_ERR_INVALID_STATE;
    } else {
        err = timer_remove(timer);
    }
    return err;
}

esp_err_t esp_timer_delete(esp_timer_handle_t timer)
{
    if (timer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int64_t alarm = systimer_hal.ticks_to_us(systimer_hal_get_counter_value(&systimer_hal, SYSTIMER_COUNTER_ESPTIMER));
    esp_err_t err;

    /* Check if the timer is armed once the list is locked to avoid a data race */
    if (timer_armed(timer)) {
        err = ESP_ERR_INVALID_STATE;
    } else {
        // A case for the timer with ESP_TIMER_ISR:
        // This ISR timer was removed from the ISR list in esp_timer_stop() or in timer_process_alarm() -> LIST_REMOVE(it, list_entry)
        // and here this timer will be added to another the TASK list, see below.
        // We do this because we want to free memory of the timer in a task context instead of an isr context.
        timer->flags &= ~FL_ISR_DISPATCH_METHOD;
        timer->event_id = EVENT_ID_DELETE_TIMER;
        timer->alarm = alarm;
        timer->period = 0;
        err = timer_insert(timer, false);
    }
    return err;
}

static void IRAM_ATTR esp_timer_impl_set_alarm_id(uint64_t timestamp, unsigned alarm_id)
{
    assert(alarm_id < sizeof(timestamp_id) / sizeof(timestamp_id[0]));
    timestamp_id[alarm_id] = timestamp;
    timestamp = MIN(timestamp_id[0], timestamp_id[1]);
    systimer_hal_set_alarm_target(&systimer_hal, SYSTIMER_ALARM_ESPTIMER, timestamp);
}

static IRAM_ATTR esp_err_t timer_insert(esp_timer_handle_t timer, bool without_update_alarm)
{
    esp_timer_handle_t it, last = NULL;
    esp_timer_dispatch_t dispatch_method = timer->flags & FL_ISR_DISPATCH_METHOD;
    if (LIST_FIRST(&s_timers[dispatch_method]) == NULL) {
        LIST_INSERT_HEAD(&s_timers[dispatch_method], timer, list_entry);
    } else {
        LIST_FOREACH(it, &s_timers[dispatch_method], list_entry) {
            if (timer->alarm < it->alarm) {
                LIST_INSERT_BEFORE(it, timer, list_entry);
                break;
            }
            last = it;
        }
        if (it == NULL) {
            assert(last);
            LIST_INSERT_AFTER(last, timer, list_entry);
        }
    }
    if (without_update_alarm == false && timer == LIST_FIRST(&s_timers[dispatch_method])) {
        esp_timer_impl_set_alarm_id(timer->alarm, dispatch_method);
    }
    return ESP_OK;
}

static IRAM_ATTR esp_err_t timer_remove(esp_timer_handle_t timer)
{
    esp_timer_dispatch_t dispatch_method = timer->flags & FL_ISR_DISPATCH_METHOD;

    esp_timer_handle_t first_timer = LIST_FIRST(&s_timers[dispatch_method]);
    LIST_REMOVE(timer, list_entry);
    timer->alarm = 0;
    timer->period = 0;
    if (timer == first_timer) { // if this timer was the first in the list.
        uint64_t next_timestamp = UINT64_MAX;
        first_timer = LIST_FIRST(&s_timers[dispatch_method]);
        if (first_timer) { // if after removing the timer from the list, this list is not empty.
            next_timestamp = first_timer->alarm;
        }
        esp_timer_impl_set_alarm_id(next_timestamp, dispatch_method);
    }

    return ESP_OK;
}

static IRAM_ATTR bool timer_armed(esp_timer_handle_t timer)
{
    return timer->alarm > 0;
}


static IRAM_ATTR bool timer_process_alarm(esp_timer_dispatch_t dispatch_method)
{
    bool processed = false;
    esp_timer_handle_t it;
    while (1) {
        it = LIST_FIRST(&s_timers[dispatch_method]);
        int64_t now = systimer_hal.ticks_to_us(systimer_hal_get_counter_value(&systimer_hal, SYSTIMER_COUNTER_ESPTIMER));
        if (it == NULL || it->alarm > now) {
            break;
        }
        processed = true;
        LIST_REMOVE(it, list_entry);
        if (it->event_id == EVENT_ID_DELETE_TIMER) {
            // It is handled only by ESP_TIMER_TASK (see esp_timer_delete()).
            // All the ESP_TIMER_ISR timers which should be deleted are moved by esp_timer_delete() to the ESP_TIMER_TASK list.
            // We want to free memory of the timer in a task context instead of an isr context.
            free(it);
            it = NULL;
        } else {
            if (it->period > 0) {
                int skipped = (now - it->alarm) / it->period;
                if ((it->flags & FL_SKIP_UNHANDLED_EVENTS) && (skipped > 1)) {
                    it->alarm = now + it->period;
                } else {
                    it->alarm += it->period;
                }
                timer_insert(it, true);
            } else {
                it->alarm = 0;
            }
            esp_timer_cb_t callback = it->callback;
            void* arg = it->arg;
            (*callback)(arg);
        }
    } // while(1)
    if (it) {
        if (dispatch_method == ESP_TIMER_TASK || (dispatch_method != ESP_TIMER_TASK && processed == true)) {
            esp_timer_impl_set_alarm_id(it->alarm, dispatch_method);
        }
    } else {
        if (processed) {
            esp_timer_impl_set_alarm_id(UINT64_MAX, dispatch_method);
        }
    }
    return processed;
}

static void IRAM_ATTR esp_timer_impl_try_to_set_next_alarm(void) {
    unsigned now_alarm_idx;  // ISR is called due to this current alarm
    unsigned next_alarm_idx; // The following alarm after now_alarm_idx
    if (timestamp_id[0] < timestamp_id[1]) {
        now_alarm_idx = 0;
        next_alarm_idx = 1;
    } else {
        now_alarm_idx = 1;
        next_alarm_idx = 0;
    }

    if (timestamp_id[next_alarm_idx] != UINT64_MAX) {
        // The following alarm is valid and can be used.
        // Remove the current alarm from consideration.
        esp_timer_impl_set_alarm_id(UINT64_MAX, now_alarm_idx);
    } else {
        // There is no the following alarm.
        // Remove the current alarm from consideration as well.
        timestamp_id[now_alarm_idx] = UINT64_MAX;
    }
}

static void IRAM_ATTR timer_alarm_isr(void *arg)
{
    // clear the interrupt
    systimer_ll_clear_alarm_int(systimer_hal.dev, SYSTIMER_ALARM_ESPTIMER);

    esp_timer_impl_try_to_set_next_alarm();

    // process timers with ISR dispatch method
    timer_process_alarm(ESP_TIMER_ISR);
}

esp_err_t esp_timer_init(void)
{
    periph_module_enable(PERIPH_SYSTIMER_MODULE);
    systimer_hal_tick_rate_ops_t ops = {
        .ticks_to_us = systimer_ticks_to_us,
        .us_to_ticks = systimer_us_to_ticks,
    };
    systimer_hal_init(&systimer_hal);
    systimer_hal_set_tick_rate_ops(&systimer_hal, &ops);

    systimer_hal_enable_counter(&systimer_hal, SYSTIMER_COUNTER_ESPTIMER);
    systimer_hal_select_alarm_mode(&systimer_hal, SYSTIMER_ALARM_ESPTIMER, SYSTIMER_ALARM_MODE_ONESHOT);
    systimer_hal_connect_alarm_counter(&systimer_hal, SYSTIMER_ALARM_ESPTIMER, SYSTIMER_COUNTER_ESPTIMER);

    esp_cpu_intr_disable(1 << ETS_INTERNAL_TIMER0_INTR_NO);
    esp_rom_route_intr_matrix(esp_cpu_get_core_id(), ETS_SYSTIMER_TARGET2_EDGE_INTR_SOURCE, ETS_INTERNAL_TIMER0_INTR_NO);

    esp_cpu_intr_set_type(ETS_INTERNAL_TIMER0_INTR_NO, 0);
    esp_cpu_intr_set_priority(ETS_INTERNAL_TIMER0_INTR_NO, 2);
    esp_cpu_intr_set_handler(ETS_INTERNAL_TIMER0_INTR_NO, timer_alarm_isr, NULL);

    systimer_hal_enable_alarm_int(&systimer_hal, SYSTIMER_ALARM_ESPTIMER);


    esp_cpu_intr_enable(1 << ETS_INTERNAL_TIMER0_INTR_NO);

    return ESP_OK;

}

esp_err_t esp_timer_deinit(void)
{
    /* Check if there are any active timers */
    for (esp_timer_dispatch_t dispatch_method = ESP_TIMER_TASK; dispatch_method < ESP_TIMER_MAX; ++dispatch_method) {
        if (!LIST_EMPTY(&s_timers[dispatch_method])) {
            return ESP_ERR_INVALID_STATE;
        }
    }

    systimer_ll_enable_alarm(systimer_hal.dev, SYSTIMER_ALARM_ESPTIMER, false);
    systimer_ll_enable_alarm_int(systimer_hal.dev, SYSTIMER_ALARM_ESPTIMER, false);
    esp_cpu_intr_disable(1 << ETS_INTERNAL_TIMER0_INTR_NO);

    return ESP_OK;
}

int64_t IRAM_ATTR esp_timer_get_next_alarm(void)
{
    int64_t next_alarm = INT64_MAX;
    for (esp_timer_dispatch_t dispatch_method = ESP_TIMER_TASK; dispatch_method < ESP_TIMER_MAX; ++dispatch_method) {
        esp_timer_handle_t it = LIST_FIRST(&s_timers[dispatch_method]);
        if (it) {
            if (next_alarm > it->alarm) {
                next_alarm = it->alarm;
            }
        }
    }
    return next_alarm;
}

int64_t IRAM_ATTR esp_timer_get_next_alarm_for_wake_up(void)
{
    int64_t next_alarm = INT64_MAX;
    for (esp_timer_dispatch_t dispatch_method = ESP_TIMER_TASK; dispatch_method < ESP_TIMER_MAX; ++dispatch_method) {
        esp_timer_handle_t it = NULL;
        LIST_FOREACH(it, &s_timers[dispatch_method], list_entry) {
            // timers with the SKIP_UNHANDLED_EVENTS flag do not want to wake up CPU from a sleep mode.
            if ((it->flags & FL_SKIP_UNHANDLED_EVENTS) == 0) {
                if (next_alarm > it->alarm) {
                    next_alarm = it->alarm;
                }
                break;
            }
        }
    }
    return next_alarm;
}

esp_err_t IRAM_ATTR esp_timer_get_period(esp_timer_handle_t timer, uint64_t *period)
{
    if (timer == NULL || period == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *period = timer->period;

    return ESP_OK;
}

esp_err_t IRAM_ATTR esp_timer_get_expiry_time(esp_timer_handle_t timer, uint64_t *expiry)
{
    if (timer == NULL || expiry == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (timer->period > 0) {
        /* Return error for periodic timers */
        return ESP_ERR_NOT_SUPPORTED;
    }


    *expiry = timer->alarm;

    return ESP_OK;
}

bool IRAM_ATTR esp_timer_is_active(esp_timer_handle_t timer)
{
    if (timer == NULL) {
        return false;
    }
    return timer_armed(timer);
}

int64_t esp_timer_get_time(void)
{
    // we hope the execution time of this function won't > 1us
    // thus, to save one function call, we didn't use the existing `systimer_hal_get_time`
    return systimer_hal.ticks_to_us(systimer_hal_get_counter_value(&systimer_hal, SYSTIMER_COUNTER_ESPTIMER));
}

#include <stdint.h>
#include "bist_interrupt.h"
#include "esp_rom_sys.h"
#include "esp_cpu.h"
#include "bist_log.h"
#include "soc/system_reg.h"
#include "soc/system_intr.h"

/* Use interrupt number 9 for testing, which is not reserved. */

#define BIST_INTERRUPT_CPU_NUMBER 9
#define BIST_INTERRUPT_SOURCE_COUNT 4
#define BIST_INTERRUPT_LOOP_COUNT 8

static const char *TAG = "bist_irq_sw";
static volatile uint8_t interrupt_count = 0;

typedef struct {
    uint32_t reg;
    uint32_t source;
} test_interrupt_reg_and_source_t;

#if defined(SOC_TARGET_ESP32C5) || defined(SOC_TARGET_ESP32C6) || defined(SOC_TARGET_ESP32C61) || \
    defined(SOC_TARGET_ESP32H2) || defined(SOC_TARGET_ESP32H21) || defined(SOC_TARGET_ESP32H4)
static const test_interrupt_reg_and_source_t test_interrupt_sources[] = {
    {INTPRI_CPU_INTR_FROM_CPU_0_REG, SYS_CPU_INTR_FROM_CPU_0_SOURCE},
    {INTPRI_CPU_INTR_FROM_CPU_1_REG, SYS_CPU_INTR_FROM_CPU_1_SOURCE},
    {INTPRI_CPU_INTR_FROM_CPU_2_REG, SYS_CPU_INTR_FROM_CPU_2_SOURCE},
    {INTPRI_CPU_INTR_FROM_CPU_3_REG, SYS_CPU_INTR_FROM_CPU_3_SOURCE},
};
#elif defined(SOC_TARGET_ESP32P4)
static const test_interrupt_reg_and_source_t test_interrupt_sources[] = {
    {HP_SYSTEM_CPU_INT_FROM_CPU_0_REG, SYS_CPU_INTR_FROM_CPU_0_SOURCE},
    {HP_SYSTEM_CPU_INT_FROM_CPU_1_REG, SYS_CPU_INTR_FROM_CPU_1_SOURCE},
    {HP_SYSTEM_CPU_INT_FROM_CPU_2_REG, SYS_CPU_INTR_FROM_CPU_2_SOURCE},
    {HP_SYSTEM_CPU_INT_FROM_CPU_3_REG, SYS_CPU_INTR_FROM_CPU_3_SOURCE},
};
#elif defined(SOC_TARGET_ESP32C3)
static const test_interrupt_reg_and_source_t test_interrupt_sources[] = {
    {SYSTEM_CPU_INTR_FROM_CPU_0_REG, SYS_CPU_INTR_FROM_CPU_0_SOURCE},
    {SYSTEM_CPU_INTR_FROM_CPU_1_REG, SYS_CPU_INTR_FROM_CPU_1_SOURCE},
    {SYSTEM_CPU_INTR_FROM_CPU_2_REG, SYS_CPU_INTR_FROM_CPU_2_SOURCE},
    {SYSTEM_CPU_INTR_FROM_CPU_3_REG, SYS_CPU_INTR_FROM_CPU_3_SOURCE},
};
#endif

static void bist_interrupt_callback(void *arg)
{
    REG_WRITE((uint32_t)arg, 0U);
    interrupt_count++;
}

bist_esp_err_t bist_interrupt_source_map_test(void)
{
    const uint32_t core_id = esp_cpu_get_core_id();
    const uint32_t cpu_intr_mask = 1U << BIST_INTERRUPT_CPU_NUMBER;
    uint32_t intr_enabled_mask = 0x0;

    /* Keep the CPU interrupt disabled while sweeping sources so a pending
     * peripheral IRQ cannot fire while briefly routed to this line. */
    esp_cpu_intr_disable(cpu_intr_mask);
    esp_cpu_intr_set_type(BIST_INTERRUPT_CPU_NUMBER, ESP_CPU_INTR_TYPE_LEVEL);
    esp_cpu_intr_set_priority(BIST_INTERRUPT_CPU_NUMBER, SOC_INTERRUPT_LEVEL_MEDIUM);

    for (int src = 0; src < BIST_INTERRUPT_SOURCE_COUNT; src++) {
        ESP_LOGD(TAG, "Mapping interrupt source %d", src);
        interrupt_count = 0;
        bool error = false;
        uint32_t source = test_interrupt_sources[src].source;
        uint32_t reg = test_interrupt_sources[src].reg;

        esp_rom_route_intr_matrix(core_id, source, BIST_INTERRUPT_CPU_NUMBER);
        esp_cpu_intr_set_handler(BIST_INTERRUPT_CPU_NUMBER, bist_interrupt_callback, (void *)reg);
        esp_cpu_intr_enable(cpu_intr_mask);

        for (int i = 0; i < BIST_INTERRUPT_LOOP_COUNT; ++i) {
            REG_WRITE(reg, BIT(0));
            esp_rom_delay_us(100);
        }
        BIST_ADD_LABEL("bist_interrupt_source_map_count");

        intr_enabled_mask = esp_cpu_intr_get_enabled_mask();
        if (!(intr_enabled_mask & cpu_intr_mask)) {
            ESP_LOGE(TAG, "Enabled interrupts mismatch: expect bit %d set, got 0x%lx", BIST_INTERRUPT_CPU_NUMBER, intr_enabled_mask);
            error = true;
        }

        esp_cpu_intr_disable(cpu_intr_mask);

        /* Unmap the interrupt source */
        esp_rom_route_intr_matrix(core_id, source, ETS_INVALID_INUM);

        if (error) {
            return BIST_ESP_INTERRUPT_TEST_ERR;
        }

        intr_enabled_mask = esp_cpu_intr_get_enabled_mask();
        if (intr_enabled_mask & cpu_intr_mask) {
            ESP_LOGE(TAG, "Enabled interrupts mismatch: expect bit %d unset, got 0x%lx", BIST_INTERRUPT_CPU_NUMBER, intr_enabled_mask);
            return BIST_ESP_INTERRUPT_TEST_ERR;
        }

        if (interrupt_count != BIST_INTERRUPT_LOOP_COUNT) {
            ESP_LOGE(TAG, "Interrupt count mismatch (%d): %d != %d", src, interrupt_count, BIST_INTERRUPT_LOOP_COUNT);
            return BIST_ESP_INTERRUPT_TEST_ERR;
        }
        ESP_LOGD(TAG, "Interrupt source %d mapped to CPU interrupt %d -- count: %d == %d", src, BIST_INTERRUPT_CPU_NUMBER, interrupt_count, BIST_INTERRUPT_LOOP_COUNT);
    }

    ESP_LOGD(TAG, "Software IRQ sources successfully triggered CPU interrupt %d", BIST_INTERRUPT_CPU_NUMBER);
    return BIST_ESP_OK;
}

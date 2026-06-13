#include "ezbsp_time.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h" // Provides ets_delay_us

/**
 * @brief Get system uptime in microseconds
 */
uint64_t ezbsp_time_us(void)
{
    return (uint64_t)esp_timer_get_time();
}

/**
 * @brief Get system uptime in milliseconds
 */
uint32_t ezbsp_time_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

/**
 * @brief Millisecond delay (Non-blocking)
 * @note Yields CPU to allow FreeRTOS to schedule other tasks
 */
void ezbsp_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

/**
 * @brief Microsecond delay (Blocking busy-wait)
 * @warning Do not use for long delays (>10ms) to avoid triggering the watchdog
 */
void ezbsp_delay_us(uint32_t us)
{
    ets_delay_us(us);
}
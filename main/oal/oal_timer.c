#include "oal_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ===================================================================================
// Time API – ESP-IDF Implementation
// ===================================================================================

bool OAL_TimeInit(void)
{
    /* Nothing required for ESP-IDF.
     * FreeRTOS tick is already initialized by the kernel.
     */
    return true;
}

/*
 * Get system tick count.
 * Uses FreeRTOS tick counter.
 */
uint32_t OAL_GetTick(void)
{
    return (uint32_t)xTaskGetTickCount();
}

/*
 * Convert ticks to milliseconds.
 */
uint32_t OAL_TicksToMS(uint32_t ticks)
{
    return ticks * portTICK_PERIOD_MS;
}

/*
 * Convert milliseconds to ticks.
 */
uint32_t OAL_MSToTicks(uint32_t ms)
{
    return pdMS_TO_TICKS(ms);
}

/*
 * Delay the calling thread for a specified number of milliseconds.
 *
 * Blocking delay using FreeRTOS scheduler.
 */
void OAL_DelayMS(uint32_t ms)
{
    if (ms == 0)
    {
        taskYIELD();
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(ms));
}

/*
 * Delay the calling thread for a specified number of seconds.
 */
void OAL_DelaySec(uint32_t sec)
{
    OAL_DelayMS(sec * 1000U);
}

/*
 * Check if a timeout has expired.
 *
 * Handles FreeRTOS tick overflow correctly.
 */
bool OAL_IsTimeoutExpired(uint32_t start_tick, uint32_t timeout_ms)
{
    uint32_t elapsed_ticks =
        OAL_GetTick() - start_tick;

    return (OAL_TicksToMS(elapsed_ticks) >= timeout_ms);
}
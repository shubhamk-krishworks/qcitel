#ifndef OAL_TIME_H
#define OAL_TIME_H

#include <stdint.h>
#include <stdbool.h>

// ===================================================================================
// Time API for OS Abstraction Layer (OAL)
// ===================================================================================

/*
 * Initialize the OAL time subsystem.
 *
 * This should be called once during system startup.
 * Platform-specific timer or tick source should be initialized here.
 *
 * @return true on success, false on failure
 */
bool OAL_TimeInit(void);

/*
 * Get system tick count.
 *
 * Tick unit is platform-dependent but MUST be consistent across the system.
 * Common choices:
 *  - 1 tick = 1 ms
 *  - OS tick (e.g., FreeRTOS tick)
 *
 * @return Current tick count since system boot
 */
uint32_t OAL_GetTick(void);

/*
 * Convert ticks to milliseconds.
 *
 * @param ticks Tick count
 * @return Time in milliseconds
 */
uint32_t OAL_TicksToMS(uint32_t ticks);

/*
 * Convert milliseconds to ticks.
 *
 * @param ms Time in milliseconds
 * @return Equivalent tick count
 */
uint32_t OAL_MSToTicks(uint32_t ms);

/*
 * Delay the calling thread for a specified number of milliseconds.
 *
 * Blocking delay.
 *
 * @param ms Number of milliseconds to delay
 */
void OAL_DelayMS(uint32_t ms);

/*
 * Delay the calling thread for a specified number of seconds.
 *
 * Blocking delay.
 *
 * @param sec Number of seconds to delay
 */
void OAL_DelaySec(uint32_t sec);

/*
 * Check if a timeout has expired.
 *
 * Typical usage:
 *   start = OAL_GetTick();
 *   if (OAL_IsTimeoutExpired(start, timeout_ms)) { ... }
 *
 * @param start_tick Tick value captured earlier
 * @param timeout_ms Timeout duration in milliseconds
 * @return true if timeout expired, false otherwise
 */
bool OAL_IsTimeoutExpired(uint32_t start_tick, uint32_t timeout_ms);

#endif /* OAL_TIME_H */
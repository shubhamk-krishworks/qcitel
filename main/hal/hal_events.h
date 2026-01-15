#ifndef HAL_EVENTS_H_
#define HAL_EVENTS_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @defgroup gcd_hal_peripheral_events Generic Peripheral Event & Callback System
 * @brief Common interface for asynchronous event handling across peripherals (UART, SPI, etc.)
 * @{
 */

/**
 * @enum hal_event_type_t
 * @brief Common peripheral event types.
 *
 * Peripherals can define their own event codes starting from
 * HAL_EVENT_CUSTOM_BASE to avoid collisions.
 */
typedef enum
{
    HAL_EVENT_NONE = 0,            /*!< No event */
    HAL_EVENT_TX_DATA,             /*!< Transmission completed successfully */
    HAL_EVENT_RX_DATA,             /*!< Reception completed successfully */
    HAL_EVENT_ERROR,               /*!< Error occurred */
    HAL_EVENT_CUSTOM_BASE = 0x1000 /*!< Base for peripheral-specific events */
} hal_event_type_t;

/**
 * @typedef hal_callback_t
 * @brief Prototype for peripheral event callback function.
 *
 * @param[in] bus_handle      Pointer to the peripheral instance that triggered the event
 * @param[in] event_type      Type of the event (see hal_event_type_t)
 * @param[in] data            Pointer to event-specific data or NULL if none
 * @param[in] data_len        Length of the data pointed to by data, or 0 if none
 * @param[in] context
 */
typedef void (*hal_callback_t)(void *bus_handle,
                               hal_event_type_t event_type,
                               void *data,
                               uint32_t data_len, void *context);

/** @} */ /* end of group gcd_hal_peripheral_events */

#endif /* HAL_EVENTS_H_ */
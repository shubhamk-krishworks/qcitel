#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "hal_events.h"
#include "hal_config.h"

/**
 * @defgroup hal_uart_error_codes UART API error codes
 * @{
 */
#define GCD_HAL_UART_OK (0)               /**< Operation succeeded */
#define GCD_HAL_UART_ERR_INVALID_ARG (-1) /**< Invalid argument passed */
#define GCD_HAL_UART_ERR_NOT_INIT (-2)    /**< UART not initialized */
#define GCD_HAL_UART_ERR_HW_FAILURE (-3)  /**< Hardware failure or communication error */
/** @} */

/**
 * @typedef hal_uart_bus_handle_t
 * @brief Opaque pointer type for internal UART driver instance
 */
typedef void *hal_uart_bus_handle_t;

/**
 * @struct hal_uart_handle_t
 * @brief Public-facing UART handle for HAL interaction
 */
typedef struct
{
    hal_uart_config_t config;          /**< UART configuration settings */
    hal_uart_bus_handle_t uart_handle; /**< Internal opaque UART driver context */
} hal_uart_handle_t;

/**
 * Initialize the UART and store the internal handle.
 *
 * @param handle Pointer to the handle to initialize
 * @param config Pointer to the configuration to apply
 * @return GCD_HAL_UART_OK on success, or a negative error code
 */
int hal_uart_initialize(hal_uart_handle_t *handle);

/**
 * Shutdown and deinitialize the UART instance.
 *
 * @param handle Pointer to the initialized UART handle
 * @return GCD_HAL_UART_OK on success, or a negative error code
 */
int hal_uart_shutdown(hal_uart_handle_t *handle);

/**
 * Register a callback for UART events.
 *
 * This function registers a callback and a user context pointer for a given UART handle.
 * The callback will be called asynchronously on UART events (TX complete, RX complete, errors, etc.).
 *
 * @param handle   Pointer to the initialized UART handle
 * @param callback Callback function pointer
 * @param context  User-defined context pointer passed to the callback
 * @return GCD_HAL_UART_OK on success, or a negative error code
 */
int hal_uart_register_callback(hal_uart_handle_t *handle, hal_callback_t callback);

/**
 * @brief Write data over UART.
 *
 * Transmits up to `length` bytes from `tx_buffer` via UART.
 *
 * @param handle Pointer to the initialized UART HAL handle
 * @param tx_buffer Pointer to the buffer containing data to send
 * @param length Number of bytes to write
 * @return Number of bytes written on success, or negative error code on failure
 */
int hal_uart_write(hal_uart_handle_t *handle, const uint8_t *tx_buffer, uint32_t length);

/**
 * @brief Read data from UART.
 *
 * Receives up to `length` bytes into `rx_buffer` via UART.
 *
 * @param handle Pointer to the initialized UART HAL handle
 * @param rx_buffer Pointer to the buffer to store received data
 * @param length Number of bytes to read
 * @return Number of bytes read on success, or negative error code on failure
 */
int hal_uart_read(hal_uart_handle_t *handle, uint8_t *rx_buffer, uint32_t length);
#ifndef HAL_GPIO_H_
#define HAL_GPIO_H_

#include <stdint.h>
#include <stdbool.h>
#include "hal_events.h"
#include "hal_config.h"

/**
 * @defgroup hal_gpio_error_codes GPIO API error codes
 * @{
 */
#define GCD_HAL_GPIO_OK (0)               /**< Operation succeeded */
#define GCD_HAL_GPIO_ERR_INVALID_ARG (-1) /**< Invalid argument passed */
#define GCD_HAL_GPIO_ERR_NOT_INIT (-2)    /**< GPIO not initialized */
#define GCD_HAL_GPIO_ERR_HW_FAILURE (-3)  /**< Hardware failure */
/** @} */

/* =========================================================
 * GPIO HAL API
 * ========================================================= */

/**
 * @brief Initialize a GPIO pin.
 */
int hal_gpio_initialize(hal_gpio_handle_t *handle);

/**
 * @brief Deinitialize a GPIO pin.
 */
int hal_gpio_shutdown(hal_gpio_handle_t *handle);

/**
 * @brief Write logic level to GPIO output.
 */
int hal_gpio_write(hal_gpio_handle_t *handle, bool level);

/**
 * @brief Read logic level from GPIO input.
 */
int hal_gpio_read(hal_gpio_handle_t *handle, bool *level);

/**
 * @brief Toggle GPIO output.
 */
int hal_gpio_toggle(hal_gpio_handle_t *handle);

/**
 * @brief Register GPIO interrupt callback.
 */
int hal_gpio_register_callback(hal_gpio_handle_t *handle,
                               hal_callback_t callback,
                               void *context);

#endif /* HAL_GPIO_H_ */
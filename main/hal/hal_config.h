#ifndef HAL_CONFIG_H_
#define HAL_CONFIG_H_

#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * UART Configuration
 * ========================================================================== */

/**
 * @brief UART parity selection.
 */
typedef enum
{
    HAL_UART_PARITY_DISABLE = 0, /**< No parity */
    HAL_UART_PARITY_EVEN,        /**< Even parity */
    HAL_UART_PARITY_ODD          /**< Odd parity */
} hal_uart_parity_t;

/**
 * @brief UART word length options.
 */
typedef enum
{
    HAL_UART_DATA_5_BITS = 0x0,  /**< Word length: 5 bits */
    HAL_UART_DATA_6_BITS = 0x1,  /**< Word length: 6 bits */
    HAL_UART_DATA_7_BITS = 0x2,  /**< Word length: 7 bits */
    HAL_UART_DATA_8_BITS = 0x3,  /**< Word length: 8 bits */
    HAL_UART_DATA_BITS_MAX = 0x4 /**< Invalid/Max marker */
} hal_uart_word_length_t;

/**
 * @brief UART stop bit selection.
 */
typedef enum
{
    HAL_UART_STOP_BITS_1 = 0x1,   /**< 1 stop bit */
    HAL_UART_STOP_BITS_1_5 = 0x2, /**< 1.5 stop bits */
    HAL_UART_STOP_BITS_2 = 0x3,   /**< 2 stop bits */
    HAL_UART_STOP_BITS_MAX = 0x4  /**< Invalid/Max marker */
} hal_uart_stop_bits_t;

/**
 * @brief UART port type enumeration.
 */
typedef enum
{
    HAL_UART_PORT_NONE = 0, /**< No port defined */
    HAL_UART_PORT_TTYAMA,   /**< UART on ttyAMA (e.g., Raspberry Pi) */
    HAL_UART_PORT_TTYS,     /**< UART on ttyS (standard serial) */
    HAL_UART_PORT_TTYUSB    /**< UART on ttyUSB (USB to Serial) */
} hal_uart_port_type_t;

/**
 * @brief UART configuration structure.
 */
typedef struct
{
    int8_t rxPin;                     /* RX pin (-1 if unused) */
    int8_t txPin;                     /* TX pin (-1 if unused) */
    uint32_t baudRate;                /* Baud rate (e.g. 9600, 115200) */
    uint32_t portNumber;              /* UART port number (e.g. 0, 1) */
    hal_uart_port_type_t port_type;   /* Port type */
    hal_uart_parity_t parity;         /* Parity option */
    hal_uart_word_length_t data_bits; /* Number of data bits */
    hal_uart_stop_bits_t stop_bits;   /* Stop bits configuration */
    void *context;                    /* Pointer to user-defined context */
} hal_uart_config_t;

/**
 * @brief Default UART configuration macro.
 */
#define HAL_UART_DEFAULT_CONFIG()                 \
    {                                             \
        -1,                      /* rxPin */      \
        -1,                      /* txPin */      \
        9600U,                   /* baudRate */   \
        0U,                      /* portNumber */ \
        HAL_UART_PORT_NONE,      /* portType */   \
        HAL_UART_PARITY_DISABLE, /* parity */     \
        HAL_UART_DATA_8_BITS,    /* dataBits */   \
        HAL_UART_STOP_BITS_1,    /* stopBits */   \
        NULL                     /* context */    \
    }

/* ============================================================================
 * GPIO Configuration
 * ========================================================================== */

#define HAL_GPIO_INVALID_PIN (-1)

/**
 * @typedef hal_gpio_bus_handle_t
 * @brief Opaque pointer to internal GPIO driver instance
 */
typedef void *hal_gpio_bus_handle_t;

/**
 * @brief GPIO direction
 */
typedef enum
{
    HAL_GPIO_DIR_INPUT = 0,
    HAL_GPIO_DIR_OUTPUT
} hal_gpio_direction_t;

/**
 * @brief GPIO pull configuration
 */
typedef enum
{
    HAL_GPIO_PULL_NONE = 0,
    HAL_GPIO_PULL_UP,
    HAL_GPIO_PULL_DOWN
} hal_gpio_pull_t;

/**
 * @brief GPIO interrupt trigger type
 */
typedef enum
{
    HAL_GPIO_IRQ_NONE = 0,
    HAL_GPIO_IRQ_RISING,
    HAL_GPIO_IRQ_FALLING,
    HAL_GPIO_IRQ_BOTH
} hal_gpio_irq_trigger_t;

/**
 * @brief GPIO configuration structure
 */
typedef struct
{
    int8_t pin;                         /**< GPIO pin number */
    hal_gpio_direction_t direction;     /**< Input / Output */
    hal_gpio_pull_t pull;               /**< Pull-up / Pull-down */
    bool init_level;                    /**< Initial output level */
    hal_gpio_irq_trigger_t irq_trigger; /**< Interrupt trigger */
    void *context;                      /**< User context */
} hal_gpio_config_t;

/**
 * @brief Public GPIO handle
 */
typedef struct
{
    hal_gpio_config_t config;          /**< GPIO configuration */
    hal_gpio_bus_handle_t gpio_handle; /**< Internal opaque driver */
} hal_gpio_handle_t;

/**
 * @brief Default GPIO configuration macro
 */
#define HAL_GPIO_DEFAULT_CONFIG() \
    {                             \
        HAL_GPIO_INVALID_PIN,     \
        HAL_GPIO_DIR_INPUT,       \
        HAL_GPIO_PULL_NONE,       \
        false,                    \
        HAL_GPIO_IRQ_NONE,        \
        NULL\}

#endif /* HAL_CONFIG_H_ */
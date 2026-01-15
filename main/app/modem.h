#ifndef MODEM_INIT_H
#define MODEM_INIT_H

#include "hal_uart_bus.h"
#include "modem_datatypes.h"

/**
 * @brief Generic status type used across modem module
 *
 * true  -> success / ready / enabled
 * false -> failure / not ready / disabled
 */
typedef bool status_t;

/* ===================== MACROS ===================== */

/**
 * @brief Maximum number of retry attempts for modem operations
 */
#define MAX_RETRY 3

/* ===================== DATA TYPES ===================== */

/**
 * @brief Modem runtime status handler
 *
 * This structure maintains the current operational state of the modem,
 * including SIM readiness, network selection mode, registration status,
 * and signal quality.
 */
typedef struct
{
    status_t sim_status;                   /**< SIM card status (true = ready) */
    network_selection_mode_t net_sel_mode; /**< Network selection mode */
    network_reg_status_t net_reg_status;   /**< Network registration status */
    signal_quality_t sig_quality;          /**< Signal quality indicator */
} modem_handler_t;

typedef struct modem_wait_timer
{
    /* Data */
    bool modem_occupied;   /* True if modem is in use */
    uint32_t start_tick;   /* Tick when modem is occupied */
    uint32_t max_wait_sec; /* Max wait time in seconds */
    char wait_reason[128]; /* Occupy reason */

    /* Methods */
    void (*occupy)(struct modem_wait_timer *self,
                   uint32_t max_wait_sec,
                   const char *reason);

    void (*release)(struct modem_wait_timer *self);

} modem_wait_timer_t;

/* Bind function pointers */
void modem_wait_timer_bind(modem_wait_timer_t *timer);

/**
 * @brief Reset modem handler structure to default values
 *
 * Initializes all modem runtime parameters to their default
 * startup state.
 *
 * @param h Modem handler instance
 */
#define MODEM_HANDLER_RESET(h)                           \
    do                                                   \
    {                                                    \
        (h).sim_status = false;                          \
        (h).net_sel_mode = NETWORK_SELECTION_DEREGISTER; \
        (h).net_reg_status = NET_REG_SEARCHING;          \
        (h).sig_quality = SIGNAL_QUALITY_VERY_POOR;      \
    } while (0)

extern modem_wait_timer_t modem_wait_timer;

/* ===================== API DECLARATIONS ===================== */

/**
 * @brief Initialize modem and underlying UART interface
 *
 * Configures the UART interface, initializes modem hardware,
 * and prepares the modem for AT command communication.
 *
 * @param uart_cfg    UART configuration structure
 * @param reset_gpio  GPIO number used to reset the modem
 *
 * @return 0 on success, negative value on failure
 */
int modem_init(hal_uart_config_t uart_cfg, uint8_t reset_gpio);

/**
 * @brief Restart the modem hardware
 *
 * Performs a modem reset sequence to recover from error states
 * or reinitialize the modem.
 */
void restart_modem(void);

/**
 * @brief Perform modem setup and configuration
 *
 * Executes modem setup steps such as SIM check, network selection,
 * and registration when polling mode is used.
 *
 * @return 0 on success, negative value on failure
 */
int modem_setup(void);

#endif /* MODEM_INIT_H */
#include <string.h>
#include <stdio.h>
#include "modem.h"
#include "modem_cmd.h"
#include "modem_datatypes.h"

#include "hal_uart_bus.h"
#include "hal_config.h"
#include "hal_events.h"
#include "hal_gpio.h"
#include "oal_log.h"
#include "oal_queue.h"
#include "oal_timer.h"
#include "oal_thread.h"

#include "modem_mqtt.h"
#include "modem_http.h"

#define MODEM_TAG "Modem"

// Queue which store the data inside it
OAL_Queue_t uart_rx_data_queue;
OAL_Thread_t uart_proc_thread;

/* =========================================================
 * Static driver handles
 * ========================================================= */
static hal_uart_handle_t uart_driver_handle;
static hal_gpio_handle_t gpio_driver_handle;

/* =========================================================
 * Modem initialization status flags
 * Indexes correspond to modem_init_state_t
 * ========================================================= */
static modem_handler_t modem_handler = {
    .sim_status = false,
    .net_sel_mode = NETWORK_SELECTION_DEREGISTER,
    .net_reg_status = NET_REG_SEARCHING,
    .sig_quality = SIGNAL_QUALITY_VERY_POOR,
};

modem_wait_timer_t modem_wait_timer = {0};

/* =========================================================
 * Send AT command over UART
 * Small delay allows modem to process command
 * ========================================================= */
void send_at_command(const char *cmd)
{
    hal_uart_write(&uart_driver_handle, (const uint8_t *)cmd, strlen(cmd));
    OAL_DelayMS(500);
}

void uart_data_processor(void *arg)
{
    static char buffer[256];
    while (OAL_Queue_WaitPop(&uart_rx_data_queue, buffer) > 0)
    {
        char *data = buffer;
        // OAL_LOGI(MODEM_TAG, "%s", (char *)data);

        /* -------- SIM status -------- */
        if (strstr(data, AT_RES_SIM_STATUS))
        {
            /* SIM is ready only if response contains "READY" */
            modem_handler.sim_status = strstr(data, "READY") ? true : false;
        }
        /* -------- Operator selection -------- */
        else if (strstr(data, AT_RES_OPERATOR_QUERY))
        {
            int mode;
            data = strstr(data, AT_RES_OPERATOR_QUERY);

            /* Check automatic operator selection and command success */
            if (data && sscanf(data, "+COPS: %d,", &mode) == 1 && strstr(data, "OK"))
                modem_handler.net_sel_mode = mode;
        }
        /* -------- Network registration -------- */
        else if (strstr(data, AT_RES_SIM_REG_STATUS))
        {
            int n;
            int stat = 0;
            data = strstr(data, AT_RES_SIM_REG_STATUS);

            /* Registration is successful if stat indicates registered */
            if (data && sscanf(data, "+CREG: %d,%d", &n, &stat) == 2 && strstr(data, "OK"))
                modem_handler.net_reg_status = stat;
        }
        else if (strstr(data, AT_RES_SIGNAL_QUALITY))
        {
            data = strstr(data, AT_RES_SIGNAL_QUALITY);
            int signal_quality;
            /* Check signal quality and command success */
            if (data && sscanf(data, "+CQI: %d,", &signal_quality) == 1 && strstr(data, "OK"))
                modem_handler.sig_quality = signal_quality;
        }
        else if (strstr(data, "ERROR"))
            modem_wait_timer.release(&modem_wait_timer);
        else
        {
            mqtt_packet_type_t type = mqtt_classify_packet(data);
            if (type != MQTT_PKT_UNKNOWN)
            {
                mqtt_data_process(type, data);
            }
            else
            {
                http_packet_type_t type = http_classify_packet(data);
                if (type != HTTP_PKT_NONE)
                    process_http_data(type, data);
            }
        }

        OAL_DelayMS(10);
    }
}

/* =========================================================
 * UART receive callback
 * Parses modem responses and updates status flags
 * ========================================================= */
static void uart_callback(void *bus_handle,
                          hal_event_type_t event_type,
                          void *data,
                          uint32_t data_len,
                          void *context)
{
    (void)bus_handle;
    (void)context;

    /* Process only valid RX data */
    if ((event_type != HAL_EVENT_RX_DATA) || (data == NULL) || (data_len == 0))
        return;
    OAL_Queue_Push(&uart_rx_data_queue, data);
}

/* =========================================================
 * Modem hardware initialization
 * Resets modem and initializes UART
 * ========================================================= */
int modem_init(hal_uart_config_t uart_cfg, uint8_t reset_gpio)
{
    /* Initialize system tick timer (required for OAL delay & timing functions) */
    OAL_TimeInit();

    /* Create UART RX queue (256-byte buffer, 100 messages depth) */
    OAL_Queue_Create(&uart_rx_data_queue, sizeof(char) * 256, 100, false);

    /* Create UART processing thread */
    OAL_Thread_Init(&uart_proc_thread, "uart_proc", 1024 * 4, 15);

    /* Assign UART data processing function to the thread */
    OAL_Thread_SetTask(&uart_proc_thread, uart_data_processor, NULL);

    /* Start UART processing thread */
    OAL_Thread_Start(&uart_proc_thread);

    /* Bind modem wait timer for command timeout handling */
    modem_wait_timer_bind(&modem_wait_timer);

    /* Initialize modem handler structure with default values */
    MODEM_HANDLER_RESET(modem_handler);

    /* ---------------- GPIO RESET CONFIGURATION ---------------- */

    /* Configure modem reset GPIO pin */
    gpio_driver_handle.config.pin = reset_gpio;
    gpio_driver_handle.config.direction = HAL_GPIO_DIR_OUTPUT;
    gpio_driver_handle.config.pull = HAL_GPIO_PULL_NONE;
    gpio_driver_handle.config.init_level = true;
    gpio_driver_handle.config.irq_trigger = HAL_GPIO_IRQ_NONE;

    /* Initialize GPIO driver for modem reset */
    if (hal_gpio_initialize(&gpio_driver_handle) != GCD_HAL_GPIO_OK)
    {
        OAL_LOGE(MODEM_TAG, "Failed to initialize GPIO for reset");
        return -1;
    }

    /* Perform hardware reset sequence */
    hal_gpio_write(&gpio_driver_handle, true);  // Assert reset
    OAL_DelaySec(1);                            // Hold reset for 1 second
    hal_gpio_write(&gpio_driver_handle, false); // Deassert reset

    /* Wait for modem to complete boot sequence */
    OAL_DelaySec(15);

    /* ---------------- UART INITIALIZATION ---------------- */

    /* Configure and initialize UART for modem communication */
    uart_driver_handle.config = uart_cfg;
    if (hal_uart_initialize(&uart_driver_handle) != GCD_HAL_UART_OK)
    {
        OAL_LOGE(MODEM_TAG, "Failed to initialize UART for modem");
        return -2;
    }

    /* Register UART RX callback for incoming modem data */
    if (hal_uart_register_callback(&uart_driver_handle, uart_callback) != GCD_HAL_UART_OK)
    {
        OAL_LOGE(MODEM_TAG, "Failed to initialize UART callback");
        return -3;
    }

    /* Modem initialization successful */
    return 0;
}

void restart_modem()
{
    if (gpio_driver_handle.gpio_handle != NULL)
    {
        OAL_LOGW(MODEM_TAG, "Restarting modem...");
        /* Hardware reset sequence */
        hal_gpio_write(&gpio_driver_handle, true);
        OAL_DelaySec(1);
        hal_gpio_write(&gpio_driver_handle, false);
    }
    else
        OAL_LOGE(MODEM_TAG, "Gpio not initialized");
}

/* =========================================================
 * Modem setup sequence (polling based)
 * Checks SIM, operator selection, and network registration
 * ========================================================= */
int modem_setup(void)
{
    int retry;

    if (!modem_wait_timer.modem_occupied)
    {
        /* Occupied item, wait for it to be released */
        modem_wait_timer.occupy(&modem_wait_timer, 30, "Occupied by modem initializer");

        /* Disable AT command echo */
        send_at_command(AT_CMD_ECHO_OFF);

        /* -------- SIM readiness check -------- */
        retry = MAX_RETRY;
        while (retry-- > 0)
        {
            send_at_command(AT_CMD_SIM_STATUS);
            if (modem_handler.sim_status == true)
                break;

            OAL_DelaySec(2);
        }

        if (modem_handler.sim_status != true)
        {
            modem_wait_timer.release(&modem_wait_timer);
            return -1; /* SIM not ready */
        }

        send_at_command(AT_CMD_SMS_DISABLE_URC);

        /* -------- Operator selection (automatic) -------- */
        retry = MAX_RETRY;
        while (retry-- > 0)
        {
            send_at_command(AT_CMD_OPERATOR_QUERY);

            if (modem_handler.net_sel_mode == NETWORK_SELECTION_AUTOMATIC)
                break;
            else
                send_at_command(AT_CMD_OPERATOR_AUTO);

            OAL_DelaySec(2);
        }

        if (modem_handler.net_sel_mode != NETWORK_SELECTION_AUTOMATIC)
        {
            modem_wait_timer.release(&modem_wait_timer);
            return -2; /* Operator selection failed */
        }

        /* -------- Network registration check -------- */
        retry = MAX_RETRY;
        while (retry-- > 0)
        {
            send_at_command(AT_CMD_SIM_REG_STATUS);
            /* Registered on home or roaming network */
            if (modem_handler.net_reg_status == NET_REG_REGISTERED_HOME || modem_handler.net_reg_status == NET_REG_REGISTERED_ROAMING)
            {
                send_at_command(AT_CMD_SIGNAL_QUALITY);
                modem_wait_timer.release(&modem_wait_timer);
                return 0;
            }
            OAL_DelaySec(2);
        }

        modem_wait_timer.release(&modem_wait_timer);
        return -3; /* Network registration failed */
    }
    else
        OAL_LOGW(MODEM_TAG, "%s", modem_wait_timer.wait_reason);

    return -1;
}

/*
 * Occupy the modem wait timer.
 *
 * User provides only:
 *  - max wait time (seconds)
 *  - reason string
 */
void modem_wait_timer_occupy(modem_wait_timer_t *self,
                             uint32_t max_wait_sec,
                             const char *reason)
{
    if (self == NULL)
        return;

    self->modem_occupied = true;
    self->start_tick = OAL_GetTick();
    self->max_wait_sec = max_wait_sec;

    if (reason != NULL)
    {
        strncpy(self->wait_reason, reason, sizeof(self->wait_reason) - 1);
        self->wait_reason[sizeof(self->wait_reason) - 1] = '\0';
    }
    else
        self->wait_reason[0] = '\0';
}

/*
 * Release the modem wait timer.
 *
 * Clears all fields.
 */
void modem_wait_timer_release(modem_wait_timer_t *self)
{
    if (self == NULL)
        return;

    self->modem_occupied = false;
    self->start_tick = 0;
    self->max_wait_sec = 0;
    self->wait_reason[0] = '\0';
}

void modem_wait_timer_bind(modem_wait_timer_t *timer)
{
    if (timer == NULL)
        return;

    timer->occupy = modem_wait_timer_occupy;
    timer->release = modem_wait_timer_release;
}
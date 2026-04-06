#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "hal_uart_bus.h"
#include "hal_config.h"
#include "hal_events.h"
#include "modem.h"

#include "modem_mqtt_example.h"
#include "modem_http_example.h"

/* Application log tag */
#define TAG "APP_UART"

/* --------------------------------------------------------------------------
 * Application Entry Point
 * -------------------------------------------------------------------------- */
void app_main(void)
{
    /* ---------------- UART Configuration ---------------- */

    /* Create UART configuration with default values */
    hal_uart_config_t uart_config = HAL_UART_DEFAULT_CONFIG();

    /* Configure UART parameters */
    uart_config.portNumber = 1;    /* UART1 */
    uart_config.txPin = 13;        /* GPIO13 → TX */
    uart_config.rxPin = 12;        /* GPIO12 → RX */
    uart_config.baudRate = 115200; /* Baud rate */

    /* ---------------- Modem Initialization ---------------- */

    if (modem_init(uart_config, 9) != 0)
    {
        ESP_LOGE(TAG, "Failed to initialize modem");
        return;
    }

    ESP_LOGI(TAG, "Modem initialized successfully");

    if (modem_setup() != 0)
        ESP_LOGE(TAG, "Failed to setup modem");
    else
        ESP_LOGI(TAG, "Modem setup completed");

    /* ---------------- MQTT Example Code Run ---------------- */
    modem_mqtt_example();

    // modem_http_example();
}
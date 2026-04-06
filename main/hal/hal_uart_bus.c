#include "hal_uart_bus.h"
#include "hal_events.h"
#include "hal_config.h"

#include <string.h>
#include <stdlib.h>

/* hardware specific */
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/* ============================================================================
 * Configuration
 * ========================================================================== */
#define RX_BUFFER_SIZE (1024U)
#define UART_EVENT_QUEUE_LEN 256

/* ============================================================================
 * Internal driver structure (opaque)
 * ========================================================================== */
typedef struct
{
    uart_port_t port;
    QueueHandle_t event_queue;
    TaskHandle_t event_task;
    hal_callback_t callback;
    void *callback_context;
} esp_uart_internal_t;

/* ============================================================================
 * HAL → ESP-IDF enum mapping helpers
 * ========================================================================== */
static uart_word_length_t map_data_bits(hal_uart_word_length_t bits)
{
    switch (bits)
    {
    case HAL_UART_DATA_5_BITS:
        return UART_DATA_5_BITS;
    case HAL_UART_DATA_6_BITS:
        return UART_DATA_6_BITS;
    case HAL_UART_DATA_7_BITS:
        return UART_DATA_7_BITS;
    case HAL_UART_DATA_8_BITS:
        return UART_DATA_8_BITS;
    default:
        return UART_DATA_8_BITS;
    }
}

static uart_parity_t map_parity(hal_uart_parity_t parity)
{
    switch (parity)
    {
    case HAL_UART_PARITY_EVEN:
        return UART_PARITY_EVEN;
    case HAL_UART_PARITY_ODD:
        return UART_PARITY_ODD;
    default:
        return UART_PARITY_DISABLE;
    }
}

static uart_stop_bits_t map_stop_bits(hal_uart_stop_bits_t stop_bits)
{
    switch (stop_bits)
    {
    case HAL_UART_STOP_BITS_2:
        return UART_STOP_BITS_2;
    case HAL_UART_STOP_BITS_1_5:
        return UART_STOP_BITS_1_5;
    case HAL_UART_STOP_BITS_1:
    default:
        return UART_STOP_BITS_1;
    }
}

/* ============================================================================
 * UART event task
 * ========================================================================== */
static void uart_event_task(void *arg)
{
    esp_uart_internal_t *drv = (esp_uart_internal_t *)arg;
    uart_event_t event;

    for (;;)
    {
        if (xQueueReceive(drv->event_queue, &event, portMAX_DELAY) == pdTRUE)
        {
            if (event.type == UART_DATA)
            {
                vTaskDelay(pdMS_TO_TICKS(10));
                uint8_t rx_buffer[512];
                int len = uart_read_bytes(drv->port, rx_buffer, sizeof(rx_buffer) - 1, 0);
                rx_buffer[len] = '\0'; // Null-terminate the
                if (len > 0)
                    drv->callback(drv, HAL_EVENT_RX_DATA, rx_buffer, len, drv->callback_context);
            }
        }
    }
}

/* ============================================================================
 * Initialize UART
 * ========================================================================== */
int hal_uart_initialize(hal_uart_handle_t *handle)
{
    if (!handle)
        return GCD_HAL_UART_ERR_INVALID_ARG;

    esp_uart_internal_t *drv = calloc(1, sizeof(esp_uart_internal_t));
    if (!drv)
        return GCD_HAL_UART_ERR_HW_FAILURE;

    drv->port = (uart_port_t)handle->config.portNumber;

    uart_config_t cfg = {
        .baud_rate = handle->config.baudRate,
        .data_bits = map_data_bits(handle->config.data_bits),
        .parity = map_parity(handle->config.parity),
        .stop_bits = map_stop_bits(handle->config.stop_bits),
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    if (uart_driver_install(drv->port,
                            RX_BUFFER_SIZE,
                            RX_BUFFER_SIZE,
                            UART_EVENT_QUEUE_LEN,
                            &drv->event_queue,
                            0) != ESP_OK)
        goto fail;

    if (uart_param_config(drv->port, &cfg) != ESP_OK)
        goto fail;

    if (uart_set_pin(drv->port,
                     handle->config.txPin,
                     handle->config.rxPin,
                     UART_PIN_NO_CHANGE,
                     UART_PIN_NO_CHANGE) != ESP_OK)
        goto fail;

    handle->uart_handle = drv;
    return GCD_HAL_UART_OK;

fail:
    uart_driver_delete(drv->port);
    free(drv);
    return GCD_HAL_UART_ERR_HW_FAILURE;
}

/* ============================================================================
 * Shutdown UART
 * ========================================================================== */
int hal_uart_shutdown(hal_uart_handle_t *handle)
{
    if (!handle || !handle->uart_handle)
        return GCD_HAL_UART_ERR_INVALID_ARG;

    esp_uart_internal_t *drv = handle->uart_handle;

    if (drv->event_task != NULL)
        vTaskDelete(drv->event_task);

    uart_driver_delete(drv->port);

    free(drv);
    handle->uart_handle = NULL;

    return GCD_HAL_UART_OK;
}

/* ============================================================================
 * Register callback
 * ========================================================================== */
int hal_uart_register_callback(hal_uart_handle_t *handle, hal_callback_t callback)
{
    if (!handle || !handle->uart_handle || !callback)
        return GCD_HAL_UART_ERR_INVALID_ARG;

    esp_uart_internal_t *drv = handle->uart_handle;
    drv->callback = callback;
    drv->callback_context = handle->config.context;

    /* Create UART event task on Core 1 with MAX priority */
    if (drv->event_task == NULL)
    {
        xTaskCreatePinnedToCore(
            uart_event_task,          /* Task function */
            "uart_evt_task",          /* Task name */
            1024 * 5,                 /* Stack size */
            drv,                      /* Task parameter */
            configMAX_PRIORITIES - 1, /* MAX priority */
            &drv->event_task,         /* Task handle */
            1                         /* Core 1 */
        );
    }

    return GCD_HAL_UART_OK;
}

/* ============================================================================
 * Write data
 * ========================================================================== */
int hal_uart_write(hal_uart_handle_t *handle,
                   const uint8_t *tx_buffer,
                   uint32_t length)
{
    if (!handle || !handle->uart_handle || !tx_buffer || length == 0)
        return GCD_HAL_UART_ERR_INVALID_ARG;

    esp_uart_internal_t *drv = handle->uart_handle;

    int ret = uart_write_bytes(drv->port, tx_buffer, length);
    uart_wait_tx_done(drv->port, portMAX_DELAY);

    return (ret > 0) ? ret : GCD_HAL_UART_ERR_HW_FAILURE;
}

/* ============================================================================
 * Read data (polling)
 * ========================================================================== */
int hal_uart_read(hal_uart_handle_t *handle,
                  uint8_t *rx_buf,
                  uint32_t length)
{
    if (!handle || !handle->uart_handle || !rx_buf || length == 0)
        return GCD_HAL_UART_ERR_INVALID_ARG;

    esp_uart_internal_t *drv = handle->uart_handle;

    int len = uart_read_bytes(drv->port,
                              rx_buf,
                              length,
                              10 / portTICK_PERIOD_MS);

    return (len >= 0) ? len : GCD_HAL_UART_ERR_HW_FAILURE;
}
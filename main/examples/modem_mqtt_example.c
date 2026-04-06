#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "modem_datatypes.h"
#include "modem_mqtt.h"

#include "modem_mqtt_example.h"

/* Application log tag */
#define TAG "MODEM MQTT Example"

/* --------------------------------------------------------------------------
 * MQTT Event Callback
 * -------------------------------------------------------------------------- */
static void mqtt_cb(mqtt_instance_no_t inst,
                    modem_mqtt_event_t evt,
                    void *data,
                    uint32_t len,
                    const char *topic,
                    modem_mqtt_response_t response)
{
    switch (evt)
    {
    case MODEM_MQTT_EVT_CONN_OPEN:
        if (response == MQTT_RES_SUCCESS)
            ESP_LOGI(TAG, "MQTT OPEN | SUCCESS | Instance: %d", inst);
        else
            ESP_LOGE(TAG, "MQTT OPEN | FAILURE | Instance: %d", inst);
        break;

    case MODEM_MQTT_EVT_CONNECT:
        if (response == MQTT_RES_SUCCESS)
            ESP_LOGI(TAG, "MQTT CONNECT | SUCCESS | Instance: %d", inst);
        else
            ESP_LOGE(TAG, "MQTT CONNECT | FAILURE | Instance: %d", inst);
        break;

    case MODEM_MQTT_EVT_SUBSCRIBE:
        if (response == MQTT_RES_SUCCESS)
            ESP_LOGI(TAG, "MQTT SUBSCRIBE | SUCCESS | Instance: %d | Topic: %s", inst, topic);
        else
            ESP_LOGE(TAG, "MQTT SUBSCRIBE | FAILURE | Instance: %d | Topic: %s", inst, topic);
        break;

    case MODEM_MQTT_EVT_UNSUBSCRIBE:
        if (response == MQTT_RES_SUCCESS)
            ESP_LOGI(TAG, "MQTT UNSUBSCRIBE | SUCCESS | Instance: %d | Topic: %s", inst, topic);
        else
            ESP_LOGE(TAG, "MQTT UNSUBSCRIBE | FAILURE | Instance: %d | Topic: %s", inst, topic);
        break;

    case MODEM_MQTT_EVT_DATA_RX:
        ESP_LOGI(TAG,
                 "MQTT DATA RX | Instance: %d | Topic: %s | Length: %u | Payload: %.*s",
                 inst,
                 topic,
                 len,
                 (int)len,
                 (char *)data);
        break;

    case MODEM_MQTT_EVT_DISCONNECT:
        ESP_LOGW(TAG, "MQTT DISCONNECT | Instance: %d", inst);
        break;

    case MODEM_MQTT_EVT_ERROR:
        ESP_LOGE(TAG, "MQTT ERROR | Instance: %d", inst);
        break;

    default:
        ESP_LOGW(TAG, "MQTT UNKNOWN EVENT | Instance: %d | Event: %d", inst, evt);
        break;
    }
}

void modem_mqtt_example(void)
{
    /* ---------------- MQTT Configuration ---------------- */

    mqtt_config_t mqtt_config = {
        .broker_ip = "139.59.65.117",
        .broker_port = 1886,
        .client_id = "jhgfdsasdfghio98765432qwertyhgjbv",
        .username = "admin",
        .password = "Pass@2023",
    };

    /* Register MQTT callback */
    modem_mqtt_register_cb(mqtt_cb);

    /* Connect to MQTT broker */
    modem_mqtt_connect(MQTT_INSTANCE_0, mqtt_config);

    /* Subscribe to topic */
    modem_mqtt_sub(MQTT_INSTANCE_0, "/device/esp32_ec200u/device");

    /* Give some time for subscription to complete */
    vTaskDelay(pdMS_TO_TICKS(3000));

    /* ---------------- Publish Loop ---------------- */

    // for (size_t i = 0; i < 1000; i++)
    // {
    //     modem_mqtt_pub(MQTT_INSTANCE_0, "device", "Hello");
    //     vTaskDelay(pdMS_TO_TICKS(5 * 1000)); // Publish every 15 seconds
    // }

    // modem_mqtt_disconnect(MQTT_INSTANCE_0);
}
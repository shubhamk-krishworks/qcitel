#ifndef MODEM_MQTT_H
#define MODEM_MQTT_H

#include <stdint.h>
#include <stdbool.h>

#include "modem_datatypes.h"

/* =========================================================
 * MQTT Response Codes
 * ========================================================= */

typedef enum
{
    MQTT_RES_SUCCESS = 0, /* MQTT operation successful */
    MQTT_RES_FAIL         /* MQTT operation failed */
} modem_mqtt_response_t;

/* =========================================================
 * MQTT Event Types
 * ========================================================= */

typedef enum
{
    MODEM_MQTT_EVT_NONE = 0,

    MODEM_MQTT_EVT_CONN_OPEN,  /* MQTT socket opened */
    MODEM_MQTT_EVT_CONNECT,    /* MQTT connected */
    MODEM_MQTT_EVT_DISCONNECT, /* MQTT disconnected */

    MODEM_MQTT_EVT_SUBSCRIBE,   /* Subscribe success */
    MODEM_MQTT_EVT_UNSUBSCRIBE, /* Unsubscribe success */
    MODEM_MQTT_EVT_PUBLISH,     /* Publish success */

    MODEM_MQTT_EVT_DATA_RX, /* Data received on subscribed topic */
    MODEM_MQTT_EVT_ERROR    /* Any MQTT error */

} modem_mqtt_event_t;

/* =========================================================
 * MQTT Callback Prototype
 * ========================================================= */

typedef void (*modem_mqtt_cb_t)(
    mqtt_instance_no_t inst_no,    /* MQTT instance number */
    modem_mqtt_event_t event,      /* MQTT event type */
    void *data,                    /* Event data (payload / info) */
    uint32_t data_len,             /* Length of data */
    const char *topic_name,        /* Topic name (NULL if not applicable) */
    modem_mqtt_response_t response /* MQTT response code */
);

/* =========================================================
 * MQTT Configuration Structure
 * ========================================================= */

typedef struct
{
    const char *broker_ip;
    uint16_t broker_port;
    const char *client_id;
    const char *username;
    const char *password;
} mqtt_config_t;

/* =========================================================
 * MQTT Packet Processing
 * ========================================================= */

void mqtt_data_process(mqtt_packet_type_t type, char *data);

mqtt_packet_type_t mqtt_classify_packet(const char *data);

/* =========================================================
 * MQTT Callback Registration
 * ========================================================= */

void modem_mqtt_register_cb(modem_mqtt_cb_t callback);

/* =========================================================
 * Low-level MQTT APIs (Debug / Manual Control)
 * ========================================================= */

/**
 * @brief Open an MQTT socket connection to the broker.
 */
int modem_open_mqtt(mqtt_instance_no_t inst_no,
                    const char *host_name,
                    uint16_t port_no);

/**
 * @brief Connect to an MQTT broker using an already opened socket.
 */
int modem_connect_mqtt(mqtt_instance_no_t inst_no,
                       const char *client_id,
                       const char *username,
                       const char *password);

/* =========================================================
 * High-level MQTT APIs (Recommended for Production)
 * ========================================================= */

/**
 * @brief Open and connect to an MQTT broker using a configuration object.
 */
int modem_mqtt_connect(mqtt_instance_no_t inst_no,
                       mqtt_config_t mqtt_config);

/**
 * @brief Subscribe to an MQTT topic.
 */
int modem_mqtt_sub(mqtt_instance_no_t inst,
                   const char *topic);

/**
 * @brief Publish a message to an MQTT topic.
 */
int modem_mqtt_pub(mqtt_instance_no_t inst,
                   const char *topic,
                   const char *payload);

/**
 * @brief Unsubscribe from an MQTT topic.
 */
int modem_mqtt_unsub(mqtt_instance_no_t inst,
                     const char *topic);

/**
 * @brief Disconnect from MQTT broker.
 */
int modem_mqtt_disconnect(mqtt_instance_no_t inst_no);

#endif /* MODEM_MQTT_H */
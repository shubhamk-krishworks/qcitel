#include <string.h>
#include <stdio.h>

#include "oal_log.h"
#include "oal_timer.h"

#include "modem.h"
#include "modem_cmd.h"

#include "modem_mqtt.h"

/* =========================================================
 * Static State Variables
 * ========================================================= */

/* Registered MQTT event callback */
static modem_mqtt_cb_t mqtt_callback = NULL;

/* Current MQTT instance number */
static mqtt_instance_no_t mqtt_inst_no;

/* MQTT socket open status */
static bool mqtt_socket_is_open = false;

/* Last subscribed topic (used for callback reference) */
static char mqtt_sub_inproc[1024] = {0};

/* Publish flow control flags */
static bool mqtt_send_data_in_queue = false; /* Waiting for '>' prompt */
static bool mqtt_send_allow = false;         /* '>' prompt received */

/* Incoming MQTT payload handling */
static bool mqtt_data_in_progress = false; /* Payload is fragmented */
static uint16_t data_expected_len = 0;     /* Expected payload length */
static uint16_t data_rx_len = 0;           /* Received payload length */

/* Payload buffer and topic */
static char *data_payload_buf;
static char mqtt_topic[128];

/* Logging tag */
#define MODEM_MQTT "Modem MQTT"

/* External AT command sender */
extern void send_at_command(const char *cmd);

/* =========================================================
 * MQTT Packet Classifier
 * ========================================================= */

/**
 * @brief Identify MQTT packet type from modem UART data.
 */
mqtt_packet_type_t mqtt_classify_packet(const char *data)
{
    if (data == NULL)
        return MQTT_PKT_UNKNOWN;

    /* Payload is currently being received in fragments */
    if (mqtt_data_in_progress)
        return MQTT_PKT_DATA_IN_PROGRESS;

    /* Waiting for '>' prompt while publishing data */
    if (mqtt_send_data_in_queue && strstr(data, ">") != NULL)
        return MQTT_PKT_SEND_PROMPT;

    /* MQTT command responses and URCs */
    if (strstr(data, AT_RES_MQTT_OPEN) != NULL)
        return MQTT_PKT_OPEN_RES;

    if (strstr(data, AT_RES_MQTT_CONNECT) != NULL)
        return MQTT_PKT_CONNECT_RES;

    if (strstr(data, AT_RES_MQTT_SUBSCRIBE) != NULL)
        return MQTT_PKT_SUBSCRIBE_RES;

    if (strstr(data, AT_RES_MQTT_UNSUBSCRIBE) != NULL)
        return MQTT_PKT_UNSUBSCRIBE_RES;

    if (strstr(data, AT_RES_MQTT_RECV) != NULL)
        return MQTT_PKT_RECV_URC;

    if (strstr(data, AT_RES_MQTT_URC) != NULL)
        return MQTT_PKT_STATUS_URC;

    return MQTT_PKT_UNKNOWN;
}

/* =========================================================
 * MQTT Packet Processor
 * ========================================================= */

/**
 * @brief Process MQTT modem packets based on their type.
 */
void mqtt_data_process(mqtt_packet_type_t type, char *data)
{
    switch (type)
    {

    /* =====================================================
     * MQTT OPEN RESPONSE
     * ===================================================== */
    case MQTT_PKT_OPEN_RES:
    {
        data = strstr(data, AT_RES_MQTT_OPEN);

        mqtt_instance_no_t inst_no;
        mqtt_net_result_t result;

        int inst_no_i = -1;
        int result_i = -1;

        if (data && sscanf(data, "+QMTOPEN: %d,%d", &inst_no_i, &result_i) == 2)
        {
            inst_no = (mqtt_instance_no_t)inst_no_i;
            result = (mqtt_net_result_t)result_i;

            mqtt_socket_is_open = (result == MQTT_NET_RESULT_SUCCESS);

            if (mqtt_callback)
                mqtt_callback(inst_no,
                              MODEM_MQTT_EVT_CONN_OPEN,
                              NULL,
                              0,
                              NULL,
                              result == MQTT_NET_RESULT_SUCCESS ? MQTT_RES_SUCCESS : MQTT_RES_FAIL);
        }
        else
        {
            OAL_LOGW(MODEM_MQTT, "QMTOPEN parse failed: %s\n", data ? data : "NULL");
        }

        modem_wait_timer.release(&modem_wait_timer);
    }
    break;

    /* =====================================================
     * MQTT CONNECT RESPONSE
     * ===================================================== */
    case MQTT_PKT_CONNECT_RES:
    {
        data = strstr(data, AT_RES_MQTT_CONNECT);

        mqtt_instance_no_t inst_no;
        mqtt_cmd_result_t result;

        int inst_no_i = -1;
        int result_i = -1;

        if (data && sscanf(data, "+QMTCONN: %d,%d", &inst_no_i, &result_i) == 2)
        {
            inst_no = (mqtt_instance_no_t)inst_no_i;
            result = (mqtt_cmd_result_t)result_i;

            if (mqtt_callback)
                mqtt_callback(inst_no,
                              MODEM_MQTT_EVT_CONNECT,
                              NULL,
                              0,
                              NULL,
                              result == MQTT_CMD_RESULT_SUCCESS ? MQTT_RES_SUCCESS : MQTT_RES_FAIL);
        }
        else
        {
            OAL_LOGW(MODEM_MQTT, "QMTCONN parse failed: %s\n", data ? data : "NULL");
        }

        modem_wait_timer.release(&modem_wait_timer);
    }
    break;

    /* =====================================================
     * MQTT SUBSCRIBE RESPONSE
     * ===================================================== */
    case MQTT_PKT_SUBSCRIBE_RES:
    {
        char *p = strstr(data, AT_RES_MQTT_SUBSCRIBE);

        mqtt_instance_no_t inst_no;
        mqtt_cmd_result_t result;

        int inst_no_i = -1;
        int result_i = -1;

        if (p && sscanf(p, "+QMTSUB: %d,%*d,%d", &inst_no_i, &result_i) == 2)
        {
            inst_no = (mqtt_instance_no_t)inst_no_i;
            result = (mqtt_cmd_result_t)result_i;

            if (mqtt_callback)
                mqtt_callback(inst_no,
                              MODEM_MQTT_EVT_SUBSCRIBE,
                              NULL,
                              0,
                              mqtt_sub_inproc,
                              result == MQTT_CMD_RESULT_SUCCESS ? MQTT_RES_SUCCESS : MQTT_RES_FAIL);
        }

        modem_wait_timer.release(&modem_wait_timer);
    }
    break;

    /* =====================================================
     * MQTT UNSUBSCRIBE RESPONSE
     * ===================================================== */
    case MQTT_PKT_UNSUBSCRIBE_RES:
    {
        data = strstr(data, AT_RES_MQTT_UNSUBSCRIBE);

        mqtt_instance_no_t inst_no;
        mqtt_cmd_result_t result;

        int inst_no_i = -1;
        int result_i = -1;

        if (data && sscanf(data, "+QMTUNS: %d,%*d,%d", &inst_no_i, &result_i) == 2)
        {
            inst_no = (mqtt_instance_no_t)inst_no_i;
            result = (mqtt_cmd_result_t)result_i;

            if (mqtt_callback)
                mqtt_callback(inst_no,
                              MODEM_MQTT_EVT_UNSUBSCRIBE,
                              NULL,
                              0,
                              mqtt_sub_inproc,
                              result == MQTT_CMD_RESULT_SUCCESS ? MQTT_RES_SUCCESS : MQTT_RES_FAIL);
        }

        modem_wait_timer.release(&modem_wait_timer);
    }
    break;

    /* =====================================================
     * MQTT INCOMING PUBLISH (URC)
     * ===================================================== */
    case MQTT_PKT_RECV_URC:
    {
        /* Reset previous payload buffer */
        if (data_payload_buf)
            free(data_payload_buf);

        data_payload_buf = NULL;
        data_expected_len = 0;
        data_rx_len = 0;

        int inst_no_i;
        data = strstr(data, AT_RES_MQTT_RECV);

        /* Parse header only (payload handled separately) */
        if (sscanf(data, "+QMTRECV: %d,%*d,\"%127[^\"]\",%hu,", &inst_no_i, mqtt_topic, &data_expected_len) == 3 &&
            data_expected_len > 0)
        {
            data_payload_buf = (char *)malloc(data_expected_len + 1);
            mqtt_inst_no = (mqtt_instance_no_t)inst_no_i;

            /* Locate payload start */
            data = strchr(data, '"');
            data = strchr(data + 1, '"');
            data = strchr(data + 1, ',');
            if (data)
                data = strchr(data + 1, ',');
            data++;

            if (data && data_payload_buf)
            {
                if (*(char *)data == '"' && data++)
                {
                    uint16_t data_len = strlen(data);

                    if (data_len > data_expected_len)
                    {
                        stpncpy(data_payload_buf, data, data_expected_len);
                        data_rx_len += data_expected_len;
                    }
                    else
                    {
                        stpncpy(data_payload_buf, data, data_len);
                        data_rx_len += data_len;
                    }
                }

                /* Payload fully received */
                if (mqtt_callback && data_rx_len == data_expected_len)
                {
                    data_payload_buf[data_rx_len] = '\0';

                    mqtt_callback(mqtt_inst_no,
                                  MODEM_MQTT_EVT_DATA_RX,
                                  data_payload_buf,
                                  data_expected_len,
                                  mqtt_topic,
                                  MQTT_RES_SUCCESS);

                    free(data_payload_buf);
                    data_payload_buf = NULL;
                    data_expected_len = 0;
                    data_rx_len = 0;
                    mqtt_data_in_progress = false;
                }
                else if (data_rx_len < data_expected_len)
                {
                    mqtt_data_in_progress = true;
                }
            }
        }
        else
            OAL_LOGW(MODEM_MQTT, "QMTRECV header parse failed\n");
    }
    break;

    /* =====================================================
     * MQTT PAYLOAD CONTINUATION
     * ===================================================== */
    case MQTT_PKT_DATA_IN_PROGRESS:
    {
        if (data_payload_buf && data_rx_len < data_expected_len)
        {
            uint16_t remain_len = data_expected_len - data_rx_len;
            uint16_t recive_data_len = strlen(data);

            if (remain_len >= recive_data_len)
            {
                strncpy(data_payload_buf + data_rx_len, data, recive_data_len);
                data_rx_len += recive_data_len;
            }
            else
            {
                strncpy(data_payload_buf + data_rx_len, data, remain_len);
                data_rx_len += remain_len;
            }

            /* Payload completed */
            if (data_rx_len >= data_expected_len)
            {
                data_payload_buf[data_rx_len] = '\0';

                if (mqtt_callback)
                    mqtt_callback(mqtt_inst_no,
                                  MODEM_MQTT_EVT_DATA_RX,
                                  data_payload_buf,
                                  data_expected_len,
                                  mqtt_topic,
                                  MQTT_RES_SUCCESS);

                free(data_payload_buf);
                mqtt_data_in_progress = false;
                data_payload_buf = NULL;
                data_expected_len = 0;
                data_rx_len = 0;
            }
        }
        else
        {
            if (data_payload_buf)
                free(data_payload_buf);

            mqtt_data_in_progress = false;
            data_payload_buf = NULL;
            data_expected_len = 0;
            data_rx_len = 0;
        }
    }
    break;

    /* =====================================================
     * MQTT STATUS URC (Disconnect/Error)
     * ===================================================== */
    case MQTT_PKT_STATUS_URC:
    {
        data = strstr(data, AT_RES_MQTT_URC);

        int inst_no_i = -1;
        int result_i = -1;

        if (sscanf(data, "+QMTSTAT: %d,%d", &inst_no_i, &result_i) == 2)
        {
            mqtt_inst_no = (mqtt_instance_no_t)inst_no_i;
            mqtt_urc_error_code_t urc_error_code = (mqtt_urc_error_code_t)result_i;

            if (mqtt_callback &&
                (urc_error_code == MQTT_URC_ERR_CONN_CLOSED_BY_PEER ||
                 urc_error_code == MQTT_URC_ERR_SERVER_DISCONNECT ||
                 urc_error_code == MQTT_URC_ERR_CLIENT_CLOSED ||
                 urc_error_code == MQTT_URC_ERR_LINK_OR_SERVER_DOWN ||
                 urc_error_code == MQTT_URC_ERR_SEND_FAILURE_CLOSE))
            {
                mqtt_callback(mqtt_inst_no,
                              MODEM_MQTT_EVT_DISCONNECT,
                              NULL,
                              0,
                              mqtt_sub_inproc,
                              MQTT_RES_SUCCESS);
            }
        }
    }
    break;

    /* =====================================================
     * MQTT SEND PROMPT ('>')
     * ===================================================== */
    case MQTT_PKT_SEND_PROMPT:
        mqtt_send_allow = true;
        break;

    default:
        break;
    }
}

/* =========================================================
 * MQTT API Functions
 * ========================================================= */

/**
 * @brief Register MQTT event callback.
 */
void modem_mqtt_register_cb(modem_mqtt_cb_t callback)
{
    mqtt_callback = callback;
}

/**
 * @brief Open MQTT socket connection.
 */
int modem_open_mqtt(mqtt_instance_no_t inst_no, const char *host_name, uint16_t port_no)
{
    if (!modem_wait_timer.modem_occupied)
    {
        char command[256];

        modem_wait_timer.occupy(&modem_wait_timer, 120, "Occupied by MQTT OPEN Connection..");

        /* Enable payload length reporting */
        sprintf(command, AT_CMD_MQTT_ENABLE_PAYLOAD_LEN, inst_no);
        send_at_command(command);

        sprintf(command, AT_CMD_MQTT_AUTO_CLEANUP, inst_no);
        send_at_command(command);

        /* Open MQTT socket */
        sprintf(command, AT_CMD_MQTT_OPEN, inst_no, host_name, port_no);
        send_at_command(command);

        return 0;
    }
    else
    {
        OAL_LOGW(MODEM_MQTT, "%s", modem_wait_timer.wait_reason);
    }

    return -1;
}

/**
 * @brief Send MQTT CONNECT command.
 */
int modem_connect_mqtt(mqtt_instance_no_t inst_no,
                       const char *client_id,
                       const char *username,
                       const char *password)
{
    if (!modem_wait_timer.modem_occupied)
    {
        char command[256];

        modem_wait_timer.occupy(&modem_wait_timer, 30, "Occupied by MQTT CONNECT command");

        sprintf(command, AT_CMD_MQTT_CONNECT, inst_no, client_id, username, password);
        send_at_command(command);

        return 0;
    }
    else
    {
        OAL_LOGW(MODEM_MQTT, "%s", modem_wait_timer.wait_reason);
    }

    return -1;
}

/**
 * @brief Open socket and connect to MQTT broker.
 */
int modem_mqtt_connect(mqtt_instance_no_t inst_no, mqtt_config_t mqtt_config)
{
    if (!modem_wait_timer.modem_occupied)
    {
        char command[256];

        modem_wait_timer.occupy(&modem_wait_timer, 30, "Occupied by MQTT CONNECT command");

        /* Enable payload length reporting */
        sprintf(command, AT_CMD_MQTT_ENABLE_PAYLOAD_LEN, inst_no);
        send_at_command(command);

        sprintf(command, AT_CMD_MQTT_AUTO_CLEANUP, inst_no);
        send_at_command(command);

        sprintf(command, AT_CMD_MQTT_KEEPALIVE, inst_no);
        send_at_command(command);

        /* Open MQTT socket */
        sprintf(command, AT_CMD_MQTT_OPEN, inst_no,
                mqtt_config.broker_ip,
                mqtt_config.broker_port);
        send_at_command(command);

        /* Wait for socket open (max 120 seconds) */
        uint32_t waited_ms = 0;

        while (!mqtt_socket_is_open && waited_ms < 120000)
        {
            OAL_DelayMS(10);
            waited_ms += 10;
        }

        if (!mqtt_socket_is_open)
        {
            OAL_LOGE(MODEM_MQTT, "MQTT socket open timeout after 120 seconds");
            modem_wait_timer.release(&modem_wait_timer);
            return -1;
        }

        /* Connect to broker */
        sprintf(command, AT_CMD_MQTT_CONNECT,
                inst_no,
                mqtt_config.client_id,
                mqtt_config.username,
                mqtt_config.password);

        send_at_command(command);

        modem_wait_timer.release(&modem_wait_timer);
        return 0;
    }

    OAL_LOGW(MODEM_MQTT, "%s", modem_wait_timer.wait_reason);
    return -2;
}

/**
 * @brief Disconnect MQTT session.
 */
int modem_mqtt_disconnect(mqtt_instance_no_t inst_no)
{
    if (!modem_wait_timer.modem_occupied)
    {
        char command[256];

        modem_wait_timer.occupy(&modem_wait_timer, 10, "Occupied by MQTT Disconnect command");

        sprintf(command, AT_CMD_MQTT_DISCONNECT, inst_no);
        send_at_command(command);

        modem_wait_timer.release(&modem_wait_timer);
        return 0;
    }

    OAL_LOGW(MODEM_MQTT, "%s", modem_wait_timer.wait_reason);
    return -2;
}

/**
 * @brief Subscribe to MQTT topic.
 */
int modem_mqtt_sub(mqtt_instance_no_t inst, const char *topic)
{
    if (!modem_wait_timer.modem_occupied)
    {
        char command[256];

        strcpy(mqtt_sub_inproc, topic);

        modem_wait_timer.occupy(&modem_wait_timer, 30, "Occupied by MQTT SUBSCRIBE command");

        sprintf(command, AT_CMD_MQTT_SUBSCRIBE, inst, 1, topic, 0);
        send_at_command(command);

        return 0;
    }
    else
    {
        OAL_LOGW(MODEM_MQTT, "%s", modem_wait_timer.wait_reason);
    }

    return -1;
}

/**
 * @brief Unsubscribe from MQTT topic.
 */
int modem_mqtt_unsub(mqtt_instance_no_t inst, const char *topic)
{
    if (!modem_wait_timer.modem_occupied)
    {
        char command[256];

        modem_wait_timer.occupy(&modem_wait_timer, 15, "Occupied by MQTT unsubscribe command");

        sprintf(command, AT_CMD_MQTT_UNSUBSCRIBE, inst, topic);
        send_at_command(command);

        strcpy(mqtt_sub_inproc, topic);

        return 0;
    }
    else
    {
        OAL_LOGW(MODEM_MQTT, "%s", modem_wait_timer.wait_reason);
    }

    return -2;
}

/**
 * @brief Publish payload to MQTT topic.
 */
int modem_mqtt_pub(mqtt_instance_no_t inst, const char *topic, const char *payload)
{
    if (!modem_wait_timer.modem_occupied)
    {
        char command[256];

        mqtt_send_data_in_queue = true;

        modem_wait_timer.occupy(&modem_wait_timer, 15, "Occupied by MQTT send command");

        sprintf(command, AT_CMD_MQTT_PUBLISH, inst, topic, strlen(payload));
        send_at_command(command);

        /* Wait for '>' prompt (max 15 seconds) */
        uint32_t waited_ms = 0;

        while (!mqtt_send_allow && waited_ms < 15000)
        {
            OAL_DelayMS(10);
            waited_ms += 10;
        }

        if (mqtt_send_allow)
        {
            send_at_command(payload);

            modem_wait_timer.release(&modem_wait_timer);
            mqtt_send_allow = false;

            return 0;
        }
        else
        {
            mqtt_send_data_in_queue = false;
            mqtt_send_allow = false;

            modem_wait_timer.release(&modem_wait_timer);

            OAL_LOGE(MODEM_MQTT, "MQTT send timeout after 15 seconds");
            return -1;
        }
    }
    else
    {
        OAL_LOGW(MODEM_MQTT, "%s", modem_wait_timer.wait_reason);
    }

    return -2;
}
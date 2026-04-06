#include <string.h>
#include <stdio.h>

#include "oal_log.h"
#include "oal_timer.h"

#include "modem.h"
#include "modem_cmd.h"

#include "modem_http.h"

#define MODEM_HTTP "Modem HTTP"

extern void send_at_command(const char *);

static modem_http_cb_t http_callback = NULL;

static http_err_code_t http_modem_res = HTTP_ERR_UNKNOWN;
static bool http_data_in_progress = false;
static bool http_get_first_time = true;
static int http_call_res = -1;

/**
 * @brief Convert HTTP(S) error code to human-readable string
 *
 * @details
 * Maps a given HTTP(S) error code to a corresponding descriptive string.
 * This function is primarily used for logging and callback message generation.
 *
 * @param err HTTP(S) error code
 *
 * @return Pointer to constant null-terminated string describing the error
 */
static const char *http_err_to_string(http_err_code_t err)
{
    switch (err)
    {
    case HTTP_ERR_OK:
        return "Connected successfully with server";
    case HTTP_ERR_UNKNOWN:
        return "Unknown HTTP(S) error";
    case HTTP_ERR_TIMEOUT:
        return "HTTP(S) timeout";
    case HTTP_ERR_BUSY:
        return "HTTP(S) busy";
    case HTTP_ERR_UART_BUSY:
        return "UART busy";
    case HTTP_ERR_NO_REQUEST:
        return "No GET/POST request";

    case HTTP_ERR_NETWORK_BUSY:
        return "Network busy";
    case HTTP_ERR_NETWORK_OPEN_FAILED:
        return "Network open failed";
    case HTTP_ERR_NETWORK_NO_CONFIG:
        return "Network not configured";
    case HTTP_ERR_NETWORK_DEACTIVATED:
        return "Network deactivated";
    case HTTP_ERR_NETWORK_ERROR:
        return "Network error";

    case HTTP_ERR_URL_ERROR:
        return "URL error";
    case HTTP_ERR_EMPTY_URL:
        return "Empty URL";
    case HTTP_ERR_IP_ADDRESS_ERROR:
        return "IP address error";
    case HTTP_ERR_DNS_ERROR:
        return "DNS error";

    case HTTP_ERR_SOCKET_CREATE_ERROR:
        return "Socket create error";
    case HTTP_ERR_SOCKET_CONNECT_ERROR:
        return "Socket connect error";
    case HTTP_ERR_SOCKET_READ_ERROR:
        return "Socket read error";
    case HTTP_ERR_SOCKET_WRITE_ERROR:
        return "Socket write error";
    case HTTP_ERR_SOCKET_CLOSED:
        return "Socket closed";

    case HTTP_ERR_DATA_ENCODE_ERROR:
        return "Data encode error";
    case HTTP_ERR_DATA_DECODE_ERROR:
        return "Data decode error";

    case HTTP_ERR_READ_TIMEOUT:
        return "Read timeout";
    case HTTP_ERR_RESPONSE_FAILED:
        return "HTTP(S) response failed";

    case HTTP_ERR_INCOMING_CALL_BUSY:
        return "Incoming call busy";
    case HTTP_ERR_VOICE_CALL_BUSY:
        return "Voice call busy";

    case HTTP_ERR_INPUT_TIMEOUT:
        return "Input timeout";
    case HTTP_ERR_WAIT_DATA_TIMEOUT:
        return "Wait data timeout";
    case HTTP_ERR_WAIT_RESPONSE_TIMEOUT:
        return "Wait HTTP(S) response timeout";

    case HTTP_ERR_MEMORY_ALLOCATION_FAILED:
        return "Memory allocation failed";
    case HTTP_ERR_INVALID_PARAMETER:
        return "Invalid parameter";
    case HTTP_ERR_WOULDBLOCK:
        return "Operation would block";
    case HTTP_ERR_SSL_HANDSHAKE_FAILED:
        return "SSL handshake failed";

    default:
        return "Unknown error code";
    }
}

/**
 * @brief Classify incoming HTTP packet type
 *
 * @details
 * Determines the type of HTTP packet based on the content of the received
 * modem data. This function inspects known response patterns to identify
 * whether the packet corresponds to a GET response, data body, or completion.
 *
 * @param data Pointer to raw modem response buffer
 *
 * @return Identified HTTP packet type
 */
http_packet_type_t http_classify_packet(const char *data)
{

    if (strstr(data, AT_RES_HTTP_GET))
        return HTTP_PKT_GET;
    else if (strstr(data, AT_RES_HTTP_READ))
        return HTTP_PKT_BODY_DONE;
    else if (http_data_in_progress)
        return HTTP_PKT_BODY;

    return HTTP_PKT_NONE;
}

/**
 * @brief Extract HTTP payload from modem response buffer
 *
 * @details
 * Removes modem-specific response suffixes and extracts only the HTTP payload.
 * The function searches for known modem termination patterns and truncates
 * the buffer accordingly.
 *
 * Leading newline characters are also skipped to return a clean payload.
 *
 * @param buf Pointer to input buffer containing modem response
 *
 * @return Pointer to extracted payload string (may be empty string if invalid)
 */
char *extract_http_payload(char *buf)
{
    static char empty[] = "";

    if (!buf || *buf == '\0')
        return empty;

    char *end = NULL;

    /* Match full modem terminator */
    end = strstr(buf, "\r\nOK\r\n\r\n+QHTTPREAD: 0");
    if (!end)
        end = strstr(buf, "\nOK\n\n+QHTTPREAD: 0");
    if (!end)
        end = strstr(buf, "OK\r\n\r\n+QHTTPREAD: 0");
    if (!end)
        end = strstr(buf, "OK\n\n+QHTTPREAD: 0");

    if (!end)
        return empty;

    *end = '\0';

    /* Skip leading newline characters */
    while (*buf == '\r' || *buf == '\n')
        buf++;

    if (*buf == '\0')
        return empty;

    return buf;
}

/**
 * @brief Process incoming HTTP data based on packet type
 *
 * @details
 * Handles modem responses according to their classified packet type.
 * This includes parsing response codes, extracting payload data,
 * invoking registered callbacks, and managing internal state flags.
 *
 * @param type Packet type as identified by http_classify_packet()
 * @param data Pointer to raw modem response buffer
 */
void process_http_data(http_packet_type_t type, char *data)
{
    switch (type)
    {
    case HTTP_PKT_GET:
    {
        data = strstr(data, AT_RES_HTTP_GET);
        int modem_res = -1;

        /**
         * Parse modem response of format:
         * +QHTTPGET: <err>,<http_status>
         */
        if (sscanf(data, "+QHTTPGET: %d,%d", &modem_res, &http_call_res) >= 1)
        {
            http_modem_res = (http_err_code_t)modem_res;

            if (http_callback)
            {
                char msg[128];
                strcpy(msg, http_err_to_string(http_modem_res));
                if (http_call_res == 200)
                    http_data_in_progress = true;

                http_callback(http_modem_res == HTTP_ERR_OK ? MODEM_HTTP_EVT_CONNECT : MODEM_HTTP_EVT_ERROR,
                              msg,
                              http_call_res,
                              NULL);
            }

            if (http_modem_res != HTTP_ERR_OK || http_call_res != 200)
                modem_wait_timer.release(&modem_wait_timer);
        }
        else
        {
            /**
             * Handle parsing failure or unexpected modem response
             */
            modem_wait_timer.release(&modem_wait_timer);
            http_modem_res = HTTP_PKT_NONE;
            http_call_res = -1;
            http_data_in_progress = false;
            http_get_first_time = true;
        }
    }
    break;

    case HTTP_PKT_BODY:
    {
        /**
         * Handle initial data block after CONNECT indication
         */
        if (http_get_first_time)
        {
            data = strstr(data, "CONNECT");

            if (strncmp(data, "CONNECT\r\n", strlen("CONNECT\r\n")) == 0)
            {
                data += strlen("CONNECT\r\n");

                modem_http_payload_t payload = {};
                payload.data = data;
                payload.data_len = strlen(data);

                if (http_callback)
                    http_callback(MODEM_HTTP_EVT_DATA, NULL, http_call_res, &payload);
            }

            http_get_first_time = false;
        }
        else
        {
            /**
             * Handle subsequent data chunks
             */
            modem_http_payload_t payload = {};
            payload.data = data;
            payload.data_len = strlen(data);

            if (http_callback)
                http_callback(MODEM_HTTP_EVT_DATA, NULL, http_call_res, &payload);
        }
    }
    break;

    case HTTP_PKT_BODY_DONE:
        /**
         * Final payload extraction and completion notification
         */
        data = extract_http_payload(data);

        if (http_callback)
        {
            if (data && strlen(data) > 0)
            {
                modem_http_payload_t payload = {};
                payload.data = data;
                payload.data_len = strlen(data);

                http_callback(MODEM_HTTP_EVT_DATA, NULL, http_call_res, &payload);
            }

            http_callback(MODEM_HTTP_EVT_DATA_DONE, NULL, http_call_res, NULL);
        }

        modem_wait_timer.release(&modem_wait_timer);

        /**
         * Reset internal state after completion
         */
        http_modem_res = HTTP_PKT_NONE;
        http_call_res = -1;
        http_data_in_progress = false;
        http_get_first_time = true;
        break;

    default:
        /**
         * No action required for unrecognized packet types
         */
        break;
    }
}

/**
 * @brief Register HTTP callback function
 *
 * @details
 * Stores a user-provided callback function which will be invoked
 * on HTTP events such as connection, data reception, and errors.
 *
 * @param cb Callback function pointer
 */
void modem_http_register_cb(modem_http_cb_t cb)
{
    http_callback = cb;
}

/**
 * @brief Initialize modem HTTP service
 *
 * @details
 * Configures modem HTTP and SSL parameters by issuing a sequence of
 * AT commands. This function ensures that the modem is not occupied
 * before performing initialization.
 *
 * @retval true  Initialization successful
 * @retval false Initialization failed or modem busy
 */
bool modem_http_init(void)
{
    if (!modem_wait_timer.modem_occupied)
    {
        char command[256];

        modem_wait_timer.occupy(&modem_wait_timer, 3, "Occupied by HTTP init....");

        sprintf(command, AT_CMD_HTTP_CFG_CONTEXT);
        send_at_command(command);

        sprintf(command, AT_CMD_HTTP_CFG_SSLCTX);
        send_at_command(command);

        sprintf(command, AT_CMD_HTTP_CFG_RESPONSE_HDR);
        send_at_command(command);

        sprintf(command, AT_CMD_HTTP_CFG_AUTO_RES);
        send_at_command(command);

        sprintf(command, AT_CMD_SET_SSL_CIPHERSUITE);
        send_at_command(command);

        modem_wait_timer.release(&modem_wait_timer);

        return true;
    }
    else
        OAL_LOGW(MODEM_HTTP, "%s", modem_wait_timer.wait_reason);

    return false;
}

/**
 * @brief Execute HTTP GET request
 *
 * @details
 * Sends a sequence of AT commands to configure the URL, initiate
 * an HTTP GET request, and retrieve the response from the modem.
 *
 * The function assumes that the modem is available and manages
 * modem occupation during the request lifecycle.
 *
 * @param url Pointer to null-terminated URL string
 *
 * @return HTTP response code on success, or -1 on failure
 */
int modem_http_get(const char *url)
{
    if (!modem_wait_timer.modem_occupied)
    {
        char command[256];

        modem_wait_timer.occupy(&modem_wait_timer, 120, "Occupied by HTTP Get Request");

        sprintf(command, AT_CMD_HTTP_SET_URL, strlen(url), 80);
        send_at_command(command);

        OAL_DelaySec(4);

        send_at_command(url);

        sprintf(command, AT_CMD_HTTP_GET, 80);
        send_at_command(command);

        OAL_DelaySec(4);

        sprintf(command, AT_CMD_HTTP_READ, 80);
        send_at_command(command);

        return http_call_res;
    }
    else
        OAL_LOGW(MODEM_HTTP, "%s", modem_wait_timer.wait_reason);

    return -1;
}
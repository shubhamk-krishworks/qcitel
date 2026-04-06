#ifndef AT_DATA_TYPES_H
#define AT_DATA_TYPES_H

/* ============================================================
 * MODEM / NETWORK RELATED DATA TYPES
 * ============================================================ */

/**
 * @brief Network selection mode (AT+COPS)
 */
typedef enum
{
    NETWORK_SELECTION_AUTOMATIC = 0,           /* Automatic selection */
    NETWORK_SELECTION_MANUAL = 1,              /* Manual selection */
    NETWORK_SELECTION_DEREGISTER = 2,          /* Deregister from network */
    NETWORK_SELECTION_FORMAT_ONLY = 3,         /* Format query only */
    NETWORK_SELECTION_MANUAL_WITH_FALLBACK = 4 /* Manual with auto fallback */
} network_selection_mode_t;

/**
 * @brief Network registration status (AT+CREG, AT+CEREG)
 */
typedef enum
{
    NET_REG_NOT_REGISTERED = 0,      /* Not registered, not searching */
    NET_REG_REGISTERED_HOME = 1,     /* Registered, home network */
    NET_REG_SEARCHING = 2,           /* Not registered, searching */
    NET_REG_REGISTRATION_DENIED = 3, /* Registration denied */
    NET_REG_UNKNOWN = 4,             /* Unknown */
    NET_REG_REGISTERED_ROAMING = 5   /* Registered, roaming */
} network_reg_status_t;

/**
 * @brief Signal quality level (AT+CSQ)
 */
typedef enum
{
    SIGNAL_QUALITY_VERY_POOR = 0, /* ≤ -113 dBm */
    SIGNAL_QUALITY_POOR = 1,      /* -111 dBm */
    SIGNAL_QUALITY_WEAK = 2,      /* -109 to -53 dBm */
    SIGNAL_QUALITY_STRONG = 31,   /* ≥ -51 dBm */
    SIGNAL_QUALITY_UNKNOWN = 99   /* Not detectable */
} signal_quality_t;

/* ============================================================
 * MQTT RELATED DATA TYPES
 * ============================================================ */

/**
 * @brief MQTT client instance number
 *
 * Valid range: 0 to 5
 */
typedef enum
{
    MQTT_INSTANCE_0 = 0, /**< MQTT client instance 0 */
    MQTT_INSTANCE_1 = 1, /**< MQTT client instance 1 */
    MQTT_INSTANCE_2 = 2, /**< MQTT client instance 2 */
    MQTT_INSTANCE_3 = 3, /**< MQTT client instance 3 */
    MQTT_INSTANCE_4 = 4, /**< MQTT client instance 4 */
    MQTT_INSTANCE_5 = 5  /**< MQTT client instance 5 */
} mqtt_instance_no_t;

/**
 * @brief MQTT network open/close result codes
 */
typedef enum
{
    MQTT_NET_RESULT_FAIL = -1,   /**< Generic failure */
    MQTT_NET_RESULT_SUCCESS = 0, /**< Operation successful */

    /* -------- OPEN command specific errors -------- */
    MQTT_NET_RESULT_WRONG_PARAM = 1,       /**< Invalid parameter */
    MQTT_NET_RESULT_ID_OCCUPIED = 2,       /**< MQTT identifier already in use */
    MQTT_NET_RESULT_PDP_ACT_FAIL = 3,      /**< PDP context activation failed */
    MQTT_NET_RESULT_DOMAIN_PARSE_FAIL = 4, /**< Domain name parsing failed */
    MQTT_NET_RESULT_NET_CONN_ERROR = 5     /**< Network connection error */

} mqtt_net_result_t;

/**
 * @brief MQTT command execution result
 */
typedef enum
{
    MQTT_CMD_RESULT_SUCCESS = 0,        /**< Packet sent successfully and ACK received */
    MQTT_CMD_RESULT_RETRANSMISSION = 1, /**< Packet retransmission occurred */
    MQTT_CMD_RESULT_SEND_FAILED = 2     /**< Failed to send packet */
} mqtt_cmd_result_t;

/**
 * @brief MQTT connection state
 */
typedef enum
{
    MQTT_CONN_STATE_INITIALIZING = 1, /**< MQTT client is initializing */
    MQTT_CONN_STATE_CONNECTING = 2    /**< MQTT client is connecting to server */
} mqtt_conn_state_t;

/**
 * @brief MQTT URC error codes (+QMTSTAT)
 */
typedef enum
{
    MQTT_URC_ERR_CONN_CLOSED_BY_PEER = 1, /* Connection closed or reset by peer */
    MQTT_URC_ERR_PINGREQ_TIMEOUT = 2,     /* PINGREQ send timeout or failure */
    MQTT_URC_ERR_CONNECT_TIMEOUT = 3,     /* CONNECT send timeout or failure */
    MQTT_URC_ERR_CONNACK_TIMEOUT = 4,     /* CONNACK receive timeout or failure */
    MQTT_URC_ERR_SERVER_DISCONNECT = 5,   /* Server closed connection after DISCONNECT */
    MQTT_URC_ERR_SEND_FAILURE_CLOSE = 6,  /* Client closed connection due to repeated send failure */
    MQTT_URC_ERR_LINK_OR_SERVER_DOWN = 7, /* Link not alive or server unavailable */
    MQTT_URC_ERR_CLIENT_CLOSED = 8,       /* Client closed MQTT connection */

    MQTT_URC_ERR_RESERVED_START = 9 /* 9–255 reserved for future use */

} mqtt_urc_error_code_t;

/**
 * @brief MQTT packet classification type
 */
typedef enum
{
    MQTT_PKT_UNKNOWN = 0, /* Not an MQTT related packet */

    MQTT_PKT_DATA_IN_PROGRESS, /* MQTT payload transfer ongoing */
    MQTT_PKT_SEND_PROMPT,      /* '>' prompt for sending data */

    MQTT_PKT_OPEN_RES,        /* +QMTOPEN response */
    MQTT_PKT_CONNECT_RES,     /* +QMTCONN response */
    MQTT_PKT_SUBSCRIBE_RES,   /* +QMTSUB response */
    MQTT_PKT_UNSUBSCRIBE_RES, /* +QMTUNS response */
    MQTT_PKT_RECV_URC,        /* +QMTRECV incoming publish */
    MQTT_PKT_STATUS_URC       /* +QMTSTAT status URC */

} mqtt_packet_type_t;

typedef enum
{
    HTTP_PKT_NONE = 0,  /**< No data */
    HTTP_PKT_HEADER,    /**< HTTP response header */
    HTTP_PKT_GET,       /* GET request */
    HTTP_PKT_BODY,      /**< HTTP response body chunk */
    HTTP_PKT_BODY_DONE, /**< Complete HTTP response body */
    HTTP_PKT_ERROR      /**< Error packet */
} http_packet_type_t;

#endif /* AT_DATA_TYPES_H */
#ifndef EC200U_AT_COMMANDS_H
#define EC200U_AT_COMMANDS_H

#ifdef __cplusplus
extern "C"
{
#endif

/* =========================================================
 * BASIC COMMANDS
 * ========================================================= */
#define AT_CMD_BASIC "AT\r"
#define AT_CMD_RESET "ATZ\r"
#define AT_CMD_ECHO_OFF "ATE0\r"
#define AT_CMD_FACTORY_RESET "AT&F\r"
#define AT_CMD_SAVE_CONFIG "AT&W\r"
#define AT_CMD_VIEW_CONFIG "AT&V\r"

/* =========================================================
 * DEVICE INFORMATION
 * ========================================================= */
#define AT_CMD_PRODUCT_INFO "ATI\r"
#define AT_CMD_MANUFACTURER "AT+GMI\r"
#define AT_CMD_MODEL "AT+GMM\r"
#define AT_CMD_FIRMWARE "AT+GMR\r"
#define AT_CMD_IMEI "AT+GSN\r"
#define AT_CMD_IMEI_EXT "AT+CGSN\r"

/* =========================================================
 * FUNCTIONALITY
 * ========================================================= */
#define AT_CMD_SET_CFUN "AT+CFUN=%d\r"
#define AT_CMD_GET_CFUN "AT+CFUN?\r"

/* =========================================================
 * SIM / USIM
 * ========================================================= */
#define AT_CMD_SIM_STATUS "AT+CPIN?\r"
#define AT_RES_SIM_STATUS "+CPIN:"
#define AT_CMD_SIM_ENTER_PIN "AT+CPIN=\"%s\"\r"
#define AT_CMD_SIM_ICCID "AT+QCCID\r"
#define AT_CMD_SIM_IMSI "AT+CIMI\r"
#define AT_CMD_SIM_REG_STATUS "AT+CREG?\r"
#define AT_RES_SIM_REG_STATUS "+CREG:"
#define AT_CMD_SIM_REG_STATUS_PDP "AT+CEREG?\r"

/* =========================================================
 * NETWORK / OPERATOR
 * ========================================================= */
#define AT_CMD_OPERATOR_QUERY "AT+COPS?\r"
#define AT_RES_OPERATOR_QUERY "+COPS:"
#define AT_CMD_OPERATOR_AUTO "AT+COPS=0\r"
#define AT_CMD_OPERATOR_MANUAL "AT+COPS=%d,%d,\"%s\",%d\r"
#define AT_CMD_SIGNAL_QUALITY "AT+CSQ\r"
#define AT_RES_SIGNAL_QUALITY "+CSQ:"
#define AT_CMD_EXT_SIGNAL_QUALITY "AT+QCSQ\r"
#define AT_CMD_NETWORK_INFO "AT+QNWINFO\r"

/* =========================================================
 * TIME / CLOCK
 * ========================================================= */
#define AT_CMD_TIME_AUTO_UPDATE "AT+CTZU=%d\r"
#define AT_CMD_TIMEZONE_REPORT "AT+CTZR=%d\r"
#define AT_CMD_GET_NETWORK_TIME "AT+QLTS\r"
#define AT_CMD_GET_RTC_TIME "AT+CCLK?\r"

/* =========================================================
 * PDP / DATA CONTEXT
 * ========================================================= */
#define AT_CMD_PDP_DEFINE "AT+CGDCONT=%d,\"%s\",\"%s\"\r"
#define AT_CMD_PDP_ATTACH "AT+CGATT=1\r"
#define AT_CMD_PDP_DETACH "AT+CGATT=0\r"
#define AT_CMD_PDP_ACTIVATE "AT+CGACT=1,%d\r"
#define AT_CMD_PDP_DEACTIVATE "AT+CGACT=0,%d\r"
#define AT_CMD_PDP_ADDRESS "AT+CGPADDR=%d\r"

/* =========================================================
 * HTTP / HTTPS
 * ========================================================= */
#define AT_CMD_HTTP_CFG_URL "AT+QHTTPCFG=\"url\",\"%s\"\r"
#define AT_CMD_HTTP_CFG_SSL "AT+QHTTPCFG=\"ssl\",%d\r"
#define AT_CMD_HTTP_GET "AT+QHTTPGET=%d\r"
#define AT_CMD_HTTP_POST "AT+QHTTPPOST=%d,%d,%d\r"
#define AT_CMD_HTTP_READ "AT+QHTTPREAD=%d\r"

/* =========================================================
 * MQTT
 * ========================================================= */
#define AT_CMD_MQTT_ENABLE_PAYLOAD_LEN "AT+QMTCFG=\"recv/mode\",%d,0,1\r"
#define AT_CMD_MQTT_AUTO_CLEANUP "AT+QMTCFG=\"session\",%d,1\r"
#define AT_CMD_MQTT_OPEN "AT+QMTOPEN=%d,\"%s\",%d\r"
#define AT_RES_MQTT_OPEN "+QMTOPEN:"
#define AT_CMD_MQTT_CONNECT "AT+QMTCONN=%d,\"%s\",\"%s\",\"%s\"\r"
#define AT_RES_MQTT_CONNECT "+QMTCONN:"
#define AT_CMD_MQTT_PUBLISH "AT+QMTPUBEX=%d,0,0,0,\"%s\",%d\r"
#define AT_CMD_MQTT_SUBSCRIBE "AT+QMTSUB=%d,%d,\"%s\",%d\r"
#define AT_RES_MQTT_SUBSCRIBE "+QMTSUB:"
#define AT_RES_MQTT_RECV "+QMTRECV:"
#define AT_CMD_MQTT_UNSUBSCRIBE "AT+QMTUNS=%d,1,\"%s\"\r"
#define AT_RES_MQTT_UNSUBSCRIBE "+QMTUNS:"
#define AT_CMD_MQTT_DISCONNECT "AT+QMTDISC=%d\r"
#define AT_CMD_MQTT_CLOSE "AT+QMTCLOSE=%d\r"
#define AT_RES_MQTT_URC "+QMTSTAT:"

/* =========================================================
 * SMS
 * ========================================================= */
#define AT_CMD_SMS_TEXT_MODE "AT+CMGF=%d\r"
#define AT_CMD_SMS_SEND "AT+CMGS=\"%s\"\r"
#define AT_CMD_SMS_LIST "AT+CMGL=\"ALL\"\r"
#define AT_CMD_SMS_READ "AT+CMGR=%d\r"
#define AT_CMD_SMS_DELETE "AT+CMGD=%d\r"

/* =========================================================
 * POWER MANAGEMENT
 * ========================================================= */
#define AT_CMD_POWER_OFF "AT+QPOWD\r"
#define AT_CMD_SLEEP_ENABLE "AT+QSCLK=1\r"
#define AT_CMD_SLEEP_DISABLE "AT+QSCLK=0\r"

#ifdef __cplusplus
}
#endif

#endif /* EC200U_AT_COMMANDS_H */

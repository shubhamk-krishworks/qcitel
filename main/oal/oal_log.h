#ifndef OAL_LOG_H
#define OAL_LOG_H

/* ===================== LOG LEVEL APIS ===================== */

/**
 * @brief Log informational messages
 *
 * @param tag    Module or component tag
 * @param format printf-style format string
 * @param ...    Variable arguments
 */
void OAL_LOGI(const char *tag, const char *format, ...);

/**
 * @brief Log warning messages
 *
 * @param tag    Module or component tag
 * @param format printf-style format string
 * @param ...    Variable arguments
 */
void OAL_LOGW(const char *tag, const char *format, ...);

/**
 * @brief Log error messages
 *
 * @param tag    Module or component tag
 * @param format printf-style format string
 * @param ...    Variable arguments
 */
void OAL_LOGE(const char *tag, const char *format, ...);

/**
 * @brief Generic printf wrapper
 *
 * @param format printf-style format string
 * @param ...    Variable arguments
 */
void OAL_Printf(const char *format, ...);

#endif /* OAL_LOG_H */

#include "oal_log.h"

#include <stdarg.h>
#include <stdio.h>

/* ===================== INTERNAL HELPER ===================== */

static void oal_log_print(const char *level,
                          const char *tag,
                          const char *format,
                          va_list args)
{
    /* Print prefix: LEVEL:TAG: */
    printf("%s:%s: ", level, tag);

    /* Print user message */
    vprintf(format, args);

    /* New line */
    printf("\n");
}

/* ===================== LOG IMPLEMENTATION ===================== */

void OAL_LOGI(const char *tag, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    oal_log_print("INFO", tag, format, args);
    va_end(args);
}

void OAL_LOGW(const char *tag, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    oal_log_print("WARN", tag, format, args);
    va_end(args);
}

void OAL_LOGE(const char *tag, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    oal_log_print("ERROR", tag, format, args);
    va_end(args);
}

void OAL_Printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
#include "ezbsp_log.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#ifndef EZBSP_LOG_BUF_SIZE
#define EZBSP_LOG_BUF_SIZE 256
#endif

#ifndef EZBSP_LOG_ENABLE_COLOR
#define EZBSP_LOG_ENABLE_COLOR 1
#endif

#ifndef EZBSP_LOG_AUTO_NEWLINE
#define EZBSP_LOG_AUTO_NEWLINE 0
#endif

static ezbsp_log_level_t g_log_level = EZBSP_LOG_LEVEL_DEBUG;
static ezbsp_log_output_func_t g_log_output = NULL;
static bool g_log_auto_newline = EZBSP_LOG_AUTO_NEWLINE;

static void ezbsp_log_default_output(const char *str)
{
    fputs(str, stdout);
}

void ezbsp_log_set_level(ezbsp_log_level_t level)
{
    g_log_level = level;
}

void ezbsp_log_set_output(ezbsp_log_output_func_t output)
{
    g_log_output = output;
}

void ezbsp_log_set_auto_newline(bool enable)
{
    g_log_auto_newline = enable;
}

static const char *ezbsp_log_level_name(ezbsp_log_level_t level)
{
    switch (level) {
    case EZBSP_LOG_LEVEL_DEBUG:
        return "D";
    case EZBSP_LOG_LEVEL_INFO:
        return "I";
    case EZBSP_LOG_LEVEL_WARN:
        return "W";
    case EZBSP_LOG_LEVEL_ERROR:
        return "E";
    default:
        return "?";
    }
}

#if EZBSP_LOG_ENABLE_COLOR

static const char *ezbsp_log_level_color(ezbsp_log_level_t level)
{
    switch (level) {
    case EZBSP_LOG_LEVEL_DEBUG:
        return "\033[36m";
    case EZBSP_LOG_LEVEL_INFO:
        return "\033[32m";
    case EZBSP_LOG_LEVEL_WARN:
        return "\033[33m";
    case EZBSP_LOG_LEVEL_ERROR:
        return "\033[31m";
    default:
        return "";
    }
}

#define EZBSP_LOG_COLOR_RESET "\033[0m"

#endif

static void ezbsp_log_write(ezbsp_log_level_t level,
                            const char *format,
                            va_list args)
{
    char buf[EZBSP_LOG_BUF_SIZE];
    int offset = 0;
    int ret;

    if (level < g_log_level || level >= EZBSP_LOG_LEVEL_NONE) {
        return;
    }

#if EZBSP_LOG_ENABLE_COLOR
    ret = snprintf(buf,
                   sizeof(buf),
                   "%s[%s] ",
                   ezbsp_log_level_color(level),
                   ezbsp_log_level_name(level));
#else
    ret = snprintf(buf,
                   sizeof(buf),
                   "[%s] ",
                   ezbsp_log_level_name(level));
#endif

    if (ret < 0) {
        return;
    }

    if ((size_t)ret >= sizeof(buf)) {
        offset = (int)sizeof(buf) - 1;
    } else {
        offset = ret;
    }

    ret = vsnprintf(buf + offset,
                    sizeof(buf) - (size_t)offset,
                    format,
                    args);

    if (ret < 0) {
        return;
    }

    offset += ret;

    if (offset < 0) {
        offset = 0;
    }

    if ((size_t)offset >= sizeof(buf)) {
        offset = (int)sizeof(buf) - 1;
    }

#if EZBSP_LOG_ENABLE_COLOR
    {
        const char *reset = EZBSP_LOG_COLOR_RESET;
        size_t reset_len = strlen(reset);

        if ((size_t)offset + reset_len < sizeof(buf)) {
            memcpy(buf + offset, reset, reset_len);
            offset += (int)reset_len;
            buf[offset] = '\0';
        }
    }
#endif

    if (g_log_auto_newline) {
        if ((size_t)offset + 1 < sizeof(buf)) {
            buf[offset++] = '\n';
            buf[offset] = '\0';
        } else {
            buf[sizeof(buf) - 2] = '\n';
            buf[sizeof(buf) - 1] = '\0';
        }
    } else {
        buf[offset] = '\0';
    }

    if (g_log_output) {
        g_log_output(buf);
    } else {
        ezbsp_log_default_output(buf);
    }
}

void ezbsp_logd(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    ezbsp_log_write(EZBSP_LOG_LEVEL_DEBUG, format, args);
    va_end(args);
}

void ezbsp_logi(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    ezbsp_log_write(EZBSP_LOG_LEVEL_INFO, format, args);
    va_end(args);
}

void ezbsp_logw(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    ezbsp_log_write(EZBSP_LOG_LEVEL_WARN, format, args);
    va_end(args);
}

void ezbsp_loge(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    ezbsp_log_write(EZBSP_LOG_LEVEL_ERROR, format, args);
    va_end(args);
}
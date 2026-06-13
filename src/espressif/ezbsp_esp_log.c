#include "ezbsp_log.h"
#include "esp_log.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifndef EZBSP_LOG_BUF_SIZE
#define EZBSP_LOG_BUF_SIZE 256
#endif

static const char *TAG = "EZBSP";
static ezbsp_log_level_t g_log_level = EZBSP_LOG_LEVEL_DEBUG;
static ezbsp_log_output_func_t s_custom_output = NULL;
static bool s_auto_newline = true;

/* ANSI 终端颜色控制码定义 */
#define ANSI_COLOR_RESET   "\033[0m"
#define ANSI_COLOR_DEBUG   "\033[36m" // Cyan
#define ANSI_COLOR_INFO    "\033[32m" // Green
#define ANSI_COLOR_WARN    "\033[33m" // Yellow
#define ANSI_COLOR_ERROR   "\033[31m" // Red

static const char *ezbsp_log_get_color(ezbsp_log_level_t level)
{
    switch (level) {
        case EZBSP_LOG_LEVEL_DEBUG: return ANSI_COLOR_DEBUG;
        case EZBSP_LOG_LEVEL_INFO:  return ANSI_COLOR_INFO;
        case EZBSP_LOG_LEVEL_WARN:  return ANSI_COLOR_WARN;
        case EZBSP_LOG_LEVEL_ERROR: return ANSI_COLOR_ERROR;
        default:                    return "";
    }
}

static const char *ezbsp_log_get_name(ezbsp_log_level_t level)
{
    switch (level) {
        case EZBSP_LOG_LEVEL_DEBUG: return "D";
        case EZBSP_LOG_LEVEL_INFO:  return "I";
        case EZBSP_LOG_LEVEL_WARN:  return "W";
        case EZBSP_LOG_LEVEL_ERROR: return "E";
        default:                    return "?";
    }
}

void ezbsp_log_set_level(ezbsp_log_level_t level)
{
    g_log_level = level;
    
    // 同步刷新底层 ESP-IDF 的过滤级别（保持两边同步）
    esp_log_level_t esp_level;
    switch (level) {
        case EZBSP_LOG_LEVEL_DEBUG: esp_level = ESP_LOG_DEBUG;   break;
        case EZBSP_LOG_LEVEL_INFO:  esp_level = ESP_LOG_INFO;    break;
        case EZBSP_LOG_LEVEL_WARN:  esp_level = ESP_LOG_WARN;    break;
        case EZBSP_LOG_LEVEL_ERROR: esp_level = ESP_LOG_ERROR;   break;
        case EZBSP_LOG_LEVEL_NONE:  esp_level = ESP_LOG_NONE;    break;
        default:                    esp_level = ESP_LOG_INFO;    break;
    }
    esp_log_level_set(TAG, esp_level);
}

void ezbsp_log_set_output(ezbsp_log_output_func_t output)
{
    s_custom_output = output;
}

void ezbsp_log_set_auto_newline(bool enable)
{
    s_auto_newline = enable;
}

static void ezbsp_log_write_esp(ezbsp_log_level_t level, const char *format, va_list args)
{
    // 如果高于全局过滤等级或属于NONE，直接拦截
    if (level < g_log_level || level >= EZBSP_LOG_LEVEL_NONE) {
        return;
    }

    char buf[EZBSP_LOG_BUF_SIZE];
    int offset = 0;
    int ret;

    // 1. 注入颜色控制字符与级别标签
    ret = snprintf(buf, sizeof(buf), "%s[%s] ", ezbsp_log_get_color(level), ezbsp_log_get_name(level));
    if (ret < 0) return;
    offset = (ret >= (int)sizeof(buf)) ? ((int)sizeof(buf) - 1) : ret;

    // 2. 注入具体的用户格式化内容
    ret = vsnprintf(buf + offset, sizeof(buf) - (size_t)offset, format, args);
    if (ret < 0) return;
    offset += ret;
    if ((size_t)offset >= sizeof(buf)) offset = (int)sizeof(buf) - 1;

    // 3. 安全注入颜色恢复标志 (Reset)
    const char *reset = ANSI_COLOR_RESET;
    size_t reset_len = strlen(reset);
    if ((size_t)offset + reset_len < sizeof(buf)) {
        memcpy(buf + offset, reset, reset_len);
        offset += (int)reset_len;
    }

    // 4. 自动化换行逻辑
    if (s_auto_newline) {
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

    // 5. 最终输出路由分流
    if (s_custom_output) {
        s_custom_output(buf);
    } else {
        // 利用 ESP-IDF 的底层控制台直接无缓冲打印这串带 ANSI 的数据
        esp_log_writev(ESP_LOG_INFO, TAG, buf, NULL);
    }
}

void ezbsp_logd(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    ezbsp_log_write_esp(EZBSP_LOG_LEVEL_DEBUG, format, args);
    va_end(args);
}

void ezbsp_logi(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    ezbsp_log_write_esp(EZBSP_LOG_LEVEL_INFO, format, args);
    va_end(args);
}

void ezbsp_logw(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    ezbsp_log_write_esp(EZBSP_LOG_LEVEL_WARN, format, args);
    va_end(args);
}

void ezbsp_loge(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    ezbsp_log_write_esp(EZBSP_LOG_LEVEL_ERROR, format, args);
    va_end(args);
}
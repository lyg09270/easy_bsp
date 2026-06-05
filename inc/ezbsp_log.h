#ifndef EZBSP_LOG_H
#define EZBSP_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef enum {
    EZBSP_LOG_LEVEL_DEBUG = 0,
    EZBSP_LOG_LEVEL_INFO,
    EZBSP_LOG_LEVEL_WARN,
    EZBSP_LOG_LEVEL_ERROR,
    EZBSP_LOG_LEVEL_NONE,
} ezbsp_log_level_t;

typedef void (*ezbsp_log_output_func_t)(const char *str);

void ezbsp_log_set_level(ezbsp_log_level_t level);
void ezbsp_log_set_output(ezbsp_log_output_func_t output);
void ezbsp_log_set_auto_newline(bool enable);

void ezbsp_logd(const char *format, ...);
void ezbsp_logi(const char *format, ...);
void ezbsp_logw(const char *format, ...);
void ezbsp_loge(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
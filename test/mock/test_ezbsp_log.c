#include "ezbsp_log.h"

int main()
{
    ezbsp_log_set_level(EZBSP_LOG_LEVEL_DEBUG);
    ezbsp_logd("Debug log\r\n");
    ezbsp_logi("Info log\r\n");
    ezbsp_logw("Warning log\r\n");
    ezbsp_loge("Error log\r\n");

    ezbsp_logd("\r\n\r\n");

    ezbsp_log_set_auto_newline(true);
    ezbsp_logd("Auto new line enabled");
    ezbsp_logd("Debug log");
    ezbsp_logi("Info log");
    ezbsp_logw("Warning log");
    ezbsp_loge("Error log");

    return 0;
}
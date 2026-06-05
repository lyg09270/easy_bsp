#include "ezbsp_log.h"
#include "ezbsp_time.h"

int main(void)
{
    uint64_t start_us = ezbsp_time_us();
    ezbsp_delay_us(100);
    uint64_t elapsed_us = ezbsp_time_us() - start_us;

    ezbsp_logd("Elapsed time: %llu us", (unsigned long long)elapsed_us);

    if (elapsed_us < 100) {
        ezbsp_loge("delay_us failed");
        return 1;
    }

    uint32_t start_ms = ezbsp_time_ms();
    ezbsp_delay_ms(100);
    uint32_t elapsed_ms = ezbsp_time_ms() - start_ms;

    ezbsp_logd("Elapsed time: %lu ms", (unsigned long)elapsed_ms);

    if (elapsed_ms < 100) {
        ezbsp_loge("delay_ms failed");
        return 1;
    }

    return 0;
}
#ifndef EZBSP_TIME_H
#define EZBSP_TIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

uint32_t ezbsp_time_ms(void);
uint64_t ezbsp_time_us(void);

void ezbsp_delay_ms(uint32_t ms);
void ezbsp_delay_us(uint32_t us);

static inline uint32_t ezbsp_time_elapsed_ms(uint32_t start_ms)
{
    return (uint32_t)(ezbsp_time_ms() - start_ms);
}

static inline bool ezbsp_time_expired_ms(uint32_t start_ms, uint32_t timeout_ms)
{
    return ezbsp_time_elapsed_ms(start_ms) >= timeout_ms;
}

#ifdef __cplusplus
}
#endif

#endif //EZBSP_TIME_H
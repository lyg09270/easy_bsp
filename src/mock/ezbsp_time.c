#include "ezbsp_time.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <time.h>
#include <unistd.h>
#endif

uint64_t ezbsp_time_us(void)
{
#if defined(_WIN32)
    static LARGE_INTEGER freq;
    static int initialized = 0;
    LARGE_INTEGER now;

    if (!initialized) {
        QueryPerformanceFrequency(&freq);
        initialized = 1;
    }

    QueryPerformanceCounter(&now);

    return (uint64_t)((now.QuadPart * 1000000ULL) / freq.QuadPart);
#else
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ((uint64_t)ts.tv_sec * 1000000ULL) +
           ((uint64_t)ts.tv_nsec / 1000ULL);
#endif
}

uint32_t ezbsp_time_ms(void)
{
    return (uint32_t)(ezbsp_time_us() / 1000ULL);
}

void ezbsp_delay_ms(uint32_t ms)
{
#if defined(_WIN32)
    Sleep((DWORD)ms);
#else
    usleep((useconds_t)ms * 1000U);
#endif
}

void ezbsp_delay_us(uint32_t us)
{
#if defined(_WIN32)
//Windows does not have a built-in function for microsecond sleep, so we can use a busy-wait loop based on the high-resolution performance counter.
    uint64_t start = ezbsp_time_us();

    while ((ezbsp_time_us() - start) < us) {
    }
#else
    usleep((useconds_t)us);
#endif
}
#ifndef EZBSP_ERR_H
#define EZBSP_ERR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EZBSP_ERR_NONE = 0,
    EZBSP_ERR_INVALID_PARAM,
    EZBSP_ERR_NULLPTR,
    EZBSP_ERR_TIMEOUT,
    EZBSP_ERR_UNKNOWN
} ezbsp_err_t;

#ifdef __cplusplus
}
#endif

#endif //EZBSP_ERR_H
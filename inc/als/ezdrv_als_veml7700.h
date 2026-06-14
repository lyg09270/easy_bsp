#ifndef EZDEV_VEML7700_H
#define EZDEV_VEML7700_H

#include <stdint.h>
#include "ezdev_als_opt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief VEML7700 driver instance index enumeration
 * @note Used as 'subid' to distinguish multiple physical VEML7700 chips
 */
typedef enum {
    VEML7700_DEV_0 = 0,
    VEML7700_DEV_MAX
} veml7700_subid_t;

/**
 * @brief Exported operations table for the VEML7700 driver
 * @note This is linked at the board layer to the generic 'als_ctxs' pool
 */
extern const ezdev_als_opt_t ezdev_veml7700_opt;

#ifdef __cplusplus
}
#endif

#endif // EZDEV_VEML7700_H
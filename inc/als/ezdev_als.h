#ifndef EZDEV_ALS_H
#define EZDEV_ALS_H

#include <stdint.h>
#include <stdbool.h>
#include "ezdev_als_opt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Abstract ALS logical device ID
 */
typedef enum {
    EZDEV_ALS_DEVICE_0 = 0,    /**< ALS Device 0 */
    EZDEV_ALS_DEVICE_MAX       /**< Maximum supported ALS devices */
} ezdev_als_id_t;

/**
 * @brief Initialize specified ALS device, bind opt table and I2C handles
 * @return 0 on success, -1 on failure
 */
int ezdev_als_init(uint8_t dev_id);

/**
 * @brief Set sensitivity (gain) and refresh rate (integration time)
 * @return 0 on success, -1 on failure
 */
int ezdev_als_set_config(uint8_t dev_id, ezdev_als_gain_t gain, ezdev_als_integration_t it);

/**
 * @brief Change power management mode (Shutdown / Continuous / PowerSave)
 * @return 0 on success, -1 on failure
 */
int ezdev_als_set_power_mode(uint8_t dev_id, ezdev_als_mode_t mode);

/**
 * @brief Non-blocking call to trigger sampling and refresh internal data cache
 * @note  Should be invoked periodically in main loop or timers
 * @return 0 on success, -1 on failure
 */
int ezdev_als_update(uint8_t dev_id);

/**
 * @brief Fetch the latest verified data package with timestamp
 * @param[out] data Pointer to application-assigned buffer
 * @return 0 if fresh and valid, -1 if stale or uninitialized
 */
int ezdev_als_get_data(uint8_t dev_id, ezdev_als_data_t *data);

#ifdef __cplusplus
}
#endif

#endif // EZDEV_ALS_H
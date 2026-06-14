#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "als/ezdev_als.h"
#include "ezbsp_time.h"

#include "als/ezdrv_als_veml7700.h"

/**
 * @brief Internal logical context structure for each ALS device instance
 */
typedef struct ezdev_als_ctx {
    uint8_t subid;                 /**< Driver sub-device ID mapping */
    const ezdev_als_opt_t *opt;    /**< Pointer to the driver operation table */
    ezdev_als_data_t cache_data;   /**< Cached data with timestamp for this instance */
    bool is_valid;                 /**< Internal data freshness/validity status flag */
} ezdev_als_ctx_t;

/**
 * @brief Board-specific context map array.
 * @note  To be initialized or populated dynamically by board-level configurations.
 * By default, we set opt to NULL to prevent early calls.
 */
static ezdev_als_ctx_t als_ctxs[EZDEV_ALS_DEVICE_MAX] = {
    [EZDEV_ALS_DEVICE_0] = {
        .subid = VEML7700_DEV_0,
        .opt = &ezdev_veml7700_opt,   // Will be bound to &ezdev_veml7700_opt (or similar) at the board level
        .is_valid = false
    }
};

int ezdev_als_init(uint8_t dev_id)
{
    if (dev_id >= EZDEV_ALS_DEVICE_MAX) {
        return -1;
    }
    
    if (als_ctxs[dev_id].opt == NULL || als_ctxs[dev_id].opt->init == NULL) {
        return -1;
    }

    int ret = als_ctxs[dev_id].opt->init(als_ctxs[dev_id].subid);
    if (ret == 0) {
        memset(&als_ctxs[dev_id].cache_data, 0, sizeof(ezdev_als_data_t));
        als_ctxs[dev_id].is_valid = false;
    }
    
    return ret;
}

int ezdev_als_set_config(uint8_t dev_id, ezdev_als_gain_t gain, ezdev_als_integration_t it)
{
    if (dev_id >= EZDEV_ALS_DEVICE_MAX) {
        return -1;
    }
    
    if (als_ctxs[dev_id].opt == NULL || als_ctxs[dev_id].opt->set_config == NULL) {
        return -1;
    }

    int ret = als_ctxs[dev_id].opt->set_config(als_ctxs[dev_id].subid, gain, it);
    if (ret == 0) {
        /* Invalidate current data because gain or integration time modifications
           will change the physical calculation metrics immediately. */
        als_ctxs[dev_id].is_valid = false;
    }
    
    return ret;
}

int ezdev_als_set_power_mode(uint8_t dev_id, ezdev_als_mode_t mode)
{
    if (dev_id >= EZDEV_ALS_DEVICE_MAX) {
        return -1;
    }
    
    if (als_ctxs[dev_id].opt == NULL || als_ctxs[dev_id].opt->set_power_mode == NULL) {
        return -1;
    }

    return als_ctxs[dev_id].opt->set_power_mode(als_ctxs[dev_id].subid, mode);
}

int ezdev_als_update(uint8_t dev_id)
{
    if (dev_id >= EZDEV_ALS_DEVICE_MAX) {
        return -1;
    }
    
    if (als_ctxs[dev_id].opt == NULL || als_ctxs[dev_id].opt->read_data == NULL) {
        return -1;
    }

    /* Create a local data buffer to pass to the underlying driver layer */
    ezdev_als_data_t temp_data = {0};

    int ret = als_ctxs[dev_id].opt->read_data(als_ctxs[dev_id].subid, &temp_data);
    if (ret == 0) {
        /* Capture system timestamp on successful physical values conversion */
        als_ctxs[dev_id].cache_data.lux = temp_data.lux;
        als_ctxs[dev_id].cache_data.white_lux = temp_data.white_lux;
        als_ctxs[dev_id].cache_data.timestamp_us = ezbsp_time_us();
        als_ctxs[dev_id].is_valid = true;
    } else {
        als_ctxs[dev_id].is_valid = false;
    }

    return ret;
}

int ezdev_als_get_data(uint8_t dev_id, ezdev_als_data_t *data)
{
    if (dev_id >= EZDEV_ALS_DEVICE_MAX || data == NULL) {
        return -1;
    }

    /* Deny query if the cache hasn't been refreshed successfully yet */
    if (!als_ctxs[dev_id].is_valid) {
        return -1;
    }

    /* Perform a fast block structural copy to the user space application buffer */
    *data = als_ctxs[dev_id].cache_data;
    
    return 0;
}
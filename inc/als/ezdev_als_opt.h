#ifndef EZDEV_ALS_OPT_H
#define EZDEV_ALS_OPT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generic ALS gain configuration
 */
typedef enum {
    EZDEV_ALS_GAIN_LOW = 0,    /**< Low sensitivity, for bright environments */
    EZDEV_ALS_GAIN_NORMAL,     /**< Standard sensitivity */
    EZDEV_ALS_GAIN_HIGH,       /**< High sensitivity, for dark environments */
} ezdev_als_gain_t;

/**
 * @brief Generic ALS integration time configuration
 */
typedef enum {
    EZDEV_ALS_INTEGRATION_FAST = 0, /**< Fast sampling, lower resolution */
    EZDEV_ALS_INTEGRATION_NORMAL,   /**< Balanced mode */
    EZDEV_ALS_INTEGRATION_SLOW,     /**< Slow sampling, highest resolution */
} ezdev_als_integration_t;

/**
 * @brief Combined power and operation modes for ALS devices
 */
typedef enum {
    EZDEV_ALS_MODE_SHUTDOWN = 0,    /**< Deep sleep, ADC disabled */
    EZDEV_ALS_MODE_CONTINUOUS,      /**< Continuous conversion mode */
    EZDEV_ALS_MODE_POWER_SAVE,      /**< Hardware periodic power-saving mode */
} ezdev_als_mode_t;

/**
 * @brief High-level application data structure with timestamp
 */
typedef struct {
    float lux;                 /**< Ambient light intensity (Lux) */
    float white_lux;           /**< White light intensity (Lux), optimized for VEML7700 */
    uint32_t timestamp_us;     /**< System uptime tick when this data was refreshed */
} ezdev_als_data_t;

/**
 * @brief ALS driver operations function pointer table
 */
typedef struct {
    /**
     * @brief Initialize hardware registers
     * @return 0 on success, -1 on failure
     */
    int (*init)(uint8_t subid);

    /**
     * @brief Configure optical sensitivity and sampling window
     */
    int (*set_config)(uint8_t subid, ezdev_als_gain_t gain, ezdev_als_integration_t it);

    /**
     * @brief Switch device power and operational state
     */
    int (*set_power_mode)(uint8_t subid, ezdev_als_mode_t mode);

    /**
     * @brief Read data from hardware and convert to physical units (Lux)
     */
    int (*read_data)(uint8_t subid, ezdev_als_data_t *data);
} ezdev_als_opt_t;

#ifdef __cplusplus
}
#endif

#endif // EZDEV_ALS_OPT_H
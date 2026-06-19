#ifndef EZDEV_DISPLAY_H
#define EZDEV_DISPLAY_H

#include "display/ezdev_display_opt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EZDEV_DISPLAY_DEVICE_0 = 0,
    EZDEV_DISPLAY_DEVICE_MAX,
} ezdev_display_device_t;

int ezdev_display_init(uint8_t dev_id);

int ezdev_display_get_info(uint8_t dev_id,
                           ezdev_display_info_t *info);

int ezdev_display_write_area(uint8_t dev_id,
                             const ezdev_display_area_t *area,
                             const void *pixels,
                             size_t stride_bytes);

int ezdev_display_present(uint8_t dev_id,
                          const ezdev_display_area_t *area,
                          ezdev_display_refresh_t mode);

int ezdev_display_set_power(uint8_t dev_id,
                            ezdev_display_power_t mode);

int ezdev_display_is_busy(uint8_t dev_id,
                          bool *busy);

#ifdef __cplusplus
}
#endif

#endif

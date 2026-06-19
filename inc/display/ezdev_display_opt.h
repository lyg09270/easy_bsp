#ifndef EZDEV_DISPLAY_OPT_H
#define EZDEV_DISPLAY_OPT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EZDEV_DISPLAY_FORMAT_MONO_1 = 0,
    EZDEV_DISPLAY_FORMAT_GRAY_2,
    EZDEV_DISPLAY_FORMAT_RGB565,
    EZDEV_DISPLAY_FORMAT_RGB888,
} ezdev_display_format_t;

typedef enum {
    EZDEV_DISPLAY_ROTATION_0 = 0,
    EZDEV_DISPLAY_ROTATION_90,
    EZDEV_DISPLAY_ROTATION_180,
    EZDEV_DISPLAY_ROTATION_270,
} ezdev_display_rotation_t;

typedef enum {
    EZDEV_DISPLAY_REFRESH_AUTO = 0,
    EZDEV_DISPLAY_REFRESH_FULL,
    EZDEV_DISPLAY_REFRESH_PARTIAL,
} ezdev_display_refresh_t;

typedef enum {
    EZDEV_DISPLAY_POWER_ON = 0,
    EZDEV_DISPLAY_POWER_SLEEP,
    EZDEV_DISPLAY_POWER_OFF,
} ezdev_display_power_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} ezdev_display_area_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    ezdev_display_format_t format;
    bool support_partial_refresh;
} ezdev_display_info_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    ezdev_display_format_t format;
    uint8_t x_alignment;
    uint8_t y_alignment;
    bool support_partial_refresh;
} ezdev_panel_info_t;

typedef struct {
    int (*init)(uint8_t subid);
    int (*deinit)(uint8_t subid);

    int (*write_physical_area)(uint8_t subid,
                               const ezdev_display_area_t *area,
                               const void *pixels,
                               size_t stride_bytes);

    int (*present_physical)(uint8_t subid,
                            const ezdev_display_area_t *area,
                            ezdev_display_refresh_t mode);

    int (*set_power)(uint8_t subid,
                     ezdev_display_power_t power);

    int (*is_busy)(uint8_t subid,
                   bool *busy);
} ezdev_panel_opt_t;

#ifdef __cplusplus
}
#endif

#endif

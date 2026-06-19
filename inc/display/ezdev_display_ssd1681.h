#ifndef EZDEV_DISPLAY_SSD1681_H
#define EZDEV_DISPLAY_SSD1681_H

#include "display/ezdev_display_opt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SSD1681_DEV_0 = 0,
    SSD1681_DEV_MAX,
} ssd1681_dev_t;

extern const ezdev_panel_info_t ezdev_ssd1681_panel_info;
extern const ezdev_panel_opt_t ezdev_ssd1681_panel_opt;

#ifdef __cplusplus
}
#endif

#endif

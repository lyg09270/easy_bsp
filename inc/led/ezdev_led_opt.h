#ifndef EZDEV_LED_OPT_H
#define EZDEV_LED_OPT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int (*on)(uint8_t subid);
    int (*off)(uint8_t subid);
    int (*toggle)(uint8_t subid);
    int (*set_brightness)(uint8_t subid, uint8_t brightness);
    int (*get_brightness)(uint8_t subid, uint8_t *brightness);
} ezdev_led_opt_t;

#ifdef __cplusplus
}
#endif

#endif //EZDEV_LED_OPT_H
#ifndef EZDEV_LED_TEMPLATE_H
#define EZDEV_LED_TEMPLATE_H

#include <stdint.h>
#include "ezdev_led_opt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TEMPLATE_LED_0 = 0,
    TEMPLATE_LED_1,
    TEMPLATE_LED_MAX
} ezdev_led_template_id_t;

extern ezdev_led_opt_t ezdev_led_template_opt;

int ezdev_led_template_on(uint8_t subid);
int ezdev_led_template_off(uint8_t subid);
int ezdev_led_template_toggle(uint8_t subid);

int ezdev_led_template_set_brightness(uint8_t subid, uint8_t brightness);
int ezdev_led_template_get_brightness(uint8_t subid, uint8_t *brightness);

#ifdef __cplusplus
}
#endif

#endif //EZDEV_LED_TEMPLATE_H
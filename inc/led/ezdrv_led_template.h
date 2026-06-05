#ifndef EZDRV_LED_TEMPLATE_H
#define EZDRV_LED_TEMPLATE_H

#include <stdint.h>
#include "ezdrv_led_opt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TEMPLATE_LED_0 = 0,
    TEMPLATE_LED_1,
    TEMPLATE_LED_MAX
} ezdrv_led_template_id_t;

extern ezdrv_led_opt_t ezdrv_led_template_opt;

int ezdrv_led_template_on(uint8_t subid);
int ezdrv_led_template_off(uint8_t subid);
int ezdrv_led_template_toggle(uint8_t subid);

int ezdrv_led_template_set_brightness(uint8_t subid, uint8_t brightness);
int ezdrv_led_template_get_brightness(uint8_t subid, uint8_t *brightness);

#ifdef __cplusplus
}
#endif

#endif //EZDRV_LED_TEMPLATE_H
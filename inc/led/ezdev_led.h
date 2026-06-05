#ifndef EZDEV_LED_H
#define EZDEV_LED_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LED_DEVICE_0 = 0,
    LED_DEVICE_1,
    MAX_LED_DEVICES
} led_device_id_t;

int ezdev_led_on(uint8_t dev_id);
int ezdev_led_off(uint8_t dev_id);
int ezdev_led_toggle(uint8_t dev_id);

int ezdev_led_set_brightness(uint8_t dev_id, uint8_t brightness);
int ezdev_led_get_brightness(uint8_t dev_id, uint8_t *brightness);

#ifdef __cplusplus
}
#endif

#endif //EZDEV_LED_H
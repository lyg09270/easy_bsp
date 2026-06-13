#include <stdint.h>
#include <stddef.h>
#include "led/ezdev_led.h"
#include "led/ezdrv_led_opt.h"

//include driver header
#include "led/ezdrv_led_template.h"

typedef struct ezdev_led_ctx {
    uint8_t subid;
    ezdrv_led_opt_t* opt;
} ezdev_led_ctx_t;

//To be defined by board-specific code to initialize the led_ctxs array with appropriate function pointers and initial brightness values.
ezdev_led_ctx_t led_ctxs[MAX_LED_DEVICES] = {
    [LED_DEVICE_0] = {.subid = TEMPLATE_LED_0, .opt = &ezdrv_led_template_opt },
    [LED_DEVICE_1] = {.subid = TEMPLATE_LED_1, .opt = &ezdrv_led_template_opt }
};

int ezdev_led_on(uint8_t dev_id)
{
    if (dev_id >= MAX_LED_DEVICES) {
        return -1;
    }
    return led_ctxs[dev_id].opt->on(led_ctxs[dev_id].subid);
}

int ezdev_led_off(uint8_t dev_id)
{
    if (dev_id >= MAX_LED_DEVICES) {
        return -1;
    }
    return led_ctxs[dev_id].opt->off(led_ctxs[dev_id].subid);
}

int ezdev_led_toggle(uint8_t dev_id)
{
    if (dev_id >= MAX_LED_DEVICES) {
        return -1;
    }
    return led_ctxs[dev_id].opt->toggle(led_ctxs[dev_id].subid);
}

int ezdev_led_set_brightness(uint8_t dev_id, uint8_t brightness)
{
    if (dev_id >= MAX_LED_DEVICES) {
        return -1;
    }
    return led_ctxs[dev_id].opt->set_brightness(led_ctxs[dev_id].subid, brightness);
}

int ezdev_led_get_brightness(uint8_t dev_id, uint8_t *brightness)
{
    if (dev_id >= MAX_LED_DEVICES || brightness == NULL) {
        return -1;
    }
    return led_ctxs[dev_id].opt->get_brightness(led_ctxs[dev_id].subid, brightness);
}
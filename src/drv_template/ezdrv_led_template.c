#include "led/ezdrv_led_template.h"
#include "ezbsp_log.h"

//Write context in this struct
typedef struct ezdrv_led_template_ctx {
    uint8_t brightness;
    uint8_t state;
} ezdrv_led_template_ctx_t;

//This array define the context for each LED
ezdrv_led_template_ctx_t led_template_ctx[TEMPLATE_LED_MAX] = {
    [TEMPLATE_LED_0] = { .brightness = 0, .state = 0 },
    [TEMPLATE_LED_1] = { .brightness = 0, .state = 0 }
};

int ezdrv_led_template_on(uint8_t subid) {
    // Implementation for turning LED on
    ezbsp_logi("Turning LED %d ON", subid);
    return 0;
}

int ezdrv_led_template_off(uint8_t subid) {
    // Implementation for turning LED off
    ezbsp_logi("Turning LED %d OFF", subid);
    return 0;
}

int ezdrv_led_template_toggle(uint8_t subid) {
    // Implementation for toggling LED
    ezbsp_logi("Turning LED %d TOGGLE", subid);
    led_template_ctx[subid].state = !led_template_ctx[subid].state; // Toggle state
    return 0;
}

int ezdrv_led_template_set_brightness(uint8_t subid, uint8_t brightness) {
    // Implementation for setting LED brightness
    ezbsp_logi("Setting LED %d brightness to %d", subid, brightness);
    led_template_ctx[subid].brightness = brightness;
    return 0;
}

int ezdrv_led_template_get_brightness(uint8_t subid, uint8_t *brightness) {
    // Implementation for getting LED brightness
    if (brightness == NULL) {
        return -1; // Invalid parameter
    }
    *brightness = led_template_ctx[subid].brightness;
    ezbsp_logi("Getting LED %d brightness: %d", subid, *brightness);
    return 0;
}

ezdrv_led_opt_t ezdrv_led_template_opt = {
    .on = ezdrv_led_template_on,
    .off = ezdrv_led_template_off,
    .toggle = ezdrv_led_template_toggle,
    .set_brightness = ezdrv_led_template_set_brightness,
    .get_brightness = ezdrv_led_template_get_brightness
};
#include "ezbsp.h"
#include "led/ezdev_led.h"

int main()
{
    ezbsp_log_set_auto_newline(true);
    uint8_t brightness;
    // Test turning LED on
    ezdev_led_on(LED_DEVICE_0);
    
    // Test setting brightness
    ezdev_led_set_brightness(LED_DEVICE_0, 128);
    
    // Test getting brightness
    ezdev_led_get_brightness(LED_DEVICE_0, &brightness);
    
    // Test toggling LED
    ezdev_led_toggle(LED_DEVICE_0);
    
    // Test turning LED off
    ezdev_led_off(LED_DEVICE_0);

    // Test turning LED on
    ezdev_led_on(LED_DEVICE_1);
    
    // Test setting brightness
    ezdev_led_set_brightness(LED_DEVICE_1, 128);
    
    // Test getting brightness
    ezdev_led_get_brightness(LED_DEVICE_1, &brightness);
    
    // Test toggling LED
    ezdev_led_toggle(LED_DEVICE_1);
    
    // Test turning LED off
    ezdev_led_off(LED_DEVICE_1);

    ezbsp_logi("LED tests completed successfully");
    
    return 0;
}
#include "ezhal/ezhal_exti.h"
#include "driver/gpio.h"
#include "esp_attr.h"

#define EZHAL_EXTI_NUM_MAX 40 

static ezhal_exti_callback_t s_exti_callbacks[EZHAL_EXTI_NUM_MAX] = {NULL};
static bool s_isr_service_installed = false;

static void IRAM_ATTR ezhal_gpio_isr_handler(void *arg)
{
    uint32_t pin = (uint32_t)arg;
    
    if (pin < EZHAL_EXTI_NUM_MAX && s_exti_callbacks[pin] != NULL) {
        s_exti_callbacks[pin](pin);
    }
}

int ezhal_exti_init(uint32_t pin, ezhal_exti_trigger_t trigger, ezhal_exti_pull_t pull, ezhal_exti_callback_t callback)
{
    if (pin >= EZHAL_EXTI_NUM_MAX || callback == NULL) {
        return -1;
    }

    // 1. Install global GPIO ISR service
    if (!s_isr_service_installed) {
        esp_err_t err = gpio_install_isr_service(0);
        if (err == ESP_OK || err == ESP_ERR_INVALID_STATE) {
            s_isr_service_installed = true;
        } else {
            return -1;
        }
    }

    // 2. Map trigger mode to ESP-IDF enum
    gpio_int_type_t esp_intr_type;
    switch (trigger) {
        case EZHAL_EXTI_TRIGGER_RISING:     esp_intr_type = GPIO_INTR_POSEDGE;  break;
        case EZHAL_EXTI_TRIGGER_FALLING:    esp_intr_type = GPIO_INTR_NEGEDGE;  break;
        case EZHAL_EXTI_TRIGGER_ANY_EDGE:   esp_intr_type = GPIO_INTR_ANYEDGE;  break;
        case EZHAL_EXTI_TRIGGER_LOW_LEVEL:  esp_intr_type = GPIO_INTR_LOW_LEVEL;break;
        case EZHAL_EXTI_TRIGGER_HIGH_LEVEL: esp_intr_type = GPIO_INTR_HIGH_LEVEL;break;
        default:                            return -1;
    }

    // 3. Map pull mode to ESP-IDF configs
    gpio_pullup_t pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_pulldown_t pull_down_en = GPIO_PULLDOWN_DISABLE;

    switch (pull) {
        case EZHAL_EXTI_PULL_NONE:
            pull_up_en = GPIO_PULLUP_DISABLE;
            pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;
        case EZHAL_EXTI_PULL_UP:
            pull_up_en = GPIO_PULLUP_ENABLE;
            pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;
        case EZHAL_EXTI_PULL_DOWN:
            pull_up_en = GPIO_PULLUP_DISABLE;
            pull_down_en = GPIO_PULLDOWN_ENABLE;
            break;
        default:
            return -1;
    }

    // 4. Configure GPIO pin settings
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = pull_up_en,
        .pull_down_en = pull_down_en,
        .intr_type = esp_intr_type
    };

    if (gpio_config(&io_conf) != ESP_OK) {
        return -1;
    }

    // 5. Save callback and register ISR handler
    s_exti_callbacks[pin] = callback;

    if (gpio_isr_handler_add((gpio_num_t)pin, ezhal_gpio_isr_handler, (void *)pin) != ESP_OK) {
        s_exti_callbacks[pin] = NULL;
        return -1;
    }

    return 0;
}

int ezhal_exti_enable(uint32_t pin)
{
    if (pin >= EZHAL_EXTI_NUM_MAX) {
        return -1;
    }
    return (gpio_intr_enable((gpio_num_t)pin) == ESP_OK) ? 0 : -1;
}

int ezhal_exti_disable(uint32_t pin)
{
    if (pin >= EZHAL_EXTI_NUM_MAX) {
        return -1;
    }
    return (gpio_intr_disable((gpio_num_t)pin) == ESP_OK) ? 0 : -1;
}

int ezhal_exti_clear(uint32_t pin)
{
    if (pin >= EZHAL_EXTI_NUM_MAX) {
        return -1;
    }
    return 0;
}
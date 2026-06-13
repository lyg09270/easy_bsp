#include "ezhal/ezhal_gpio.h"
#include "driver/gpio.h"

/**
 * @brief Initialize a GPIO pin.
 * @return int 0 on success, -1 on failure/invalid mode.
 */
int ezhal_gpio_init(uint32_t pin, ezhal_gpio_mode_t mode)
{
    // Basic pin verification for safe bit-shifting
    if (pin >= GPIO_NUM_MAX) {
        return -1;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    switch (mode) {
        case EZHAL_GPIO_MODE_INPUT:
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // Default pull-up for inputs
            break;
            
        case EZHAL_GPIO_MODE_OUTPUT_PP:
            io_conf.mode = GPIO_MODE_OUTPUT;
            break;
            
        case EZHAL_GPIO_MODE_OUTPUT_OD:
            io_conf.mode = GPIO_MODE_OUTPUT_OD;
            break;
            
        default:
            return -1;
    }

    // gpio_config returns ESP_OK (0) on success
    if (gpio_config(&io_conf) != ESP_OK) {
        return -1;
    }

    return 0;
}

/**
 * @brief Set GPIO output level.
 */
int ezhal_gpio_write(uint32_t pin, ezhal_gpio_level_t level)
{
    // 安全校验
    if (pin >= GPIO_NUM_MAX) {
        return -1;
    }

    uint32_t esp_level = (level == EZHAL_GPIO_LEVEL_HIGH) ? 1 : 0;
    
    if (gpio_set_level((gpio_num_t)pin, esp_level) != ESP_OK) {
        return -1;
    }

    return 0; // 成功返回 0
}

/**
 * @brief Read GPIO input level.
 */
ezhal_gpio_level_t ezhal_gpio_read(uint32_t pin)
{
    int level = gpio_get_level((gpio_num_t)pin);
    return (level == 1) ? EZHAL_GPIO_LEVEL_HIGH : EZHAL_GPIO_LEVEL_LOW;
}
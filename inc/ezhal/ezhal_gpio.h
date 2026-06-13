#ifndef EZHAL_GPIO_H
#define EZHAL_GPIO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EZHAL_GPIO_MODE_INPUT = 0,
    EZHAL_GPIO_MODE_OUTPUT_PP,
    EZHAL_GPIO_MODE_OUTPUT_OD
} ezhal_gpio_mode_t;

typedef enum {
    EZHAL_GPIO_LEVEL_LOW = 0,
    EZHAL_GPIO_LEVEL_HIGH
} ezhal_gpio_level_t;


int ezhal_gpio_init(uint32_t pin, ezhal_gpio_mode_t mode);
int ezhal_gpio_write(uint32_t pin, ezhal_gpio_level_t level);
ezhal_gpio_level_t ezhal_gpio_read(uint32_t pin);

#ifdef __cplusplus
}
#endif

#endif //EZHAL_GPIO_H
#ifndef EZHAL_EXTI_H
#define EZHAL_EXTI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief EXTI Trigger edge/level configuration.
 */
typedef enum {
    EZHAL_EXTI_TRIGGER_RISING = 0,  /**< Trigger on rising edge */
    EZHAL_EXTI_TRIGGER_FALLING,     /**< Trigger on falling edge */
    EZHAL_EXTI_TRIGGER_ANY_EDGE,    /**< Trigger on any change of edge */
    EZHAL_EXTI_TRIGGER_LOW_LEVEL,   /**< Trigger on low voltage level */
    EZHAL_EXTI_TRIGGER_HIGH_LEVEL,  /**< Trigger on high voltage level */
} ezhal_exti_trigger_t;

/**
 * @brief EXTI Pull resistor configuration.
 */
typedef enum {
    EZHAL_EXTI_PULL_NONE = 0,       /**< Floating, no internal pull resistor */
    EZHAL_EXTI_PULL_UP,             /**< Enable internal pull-up resistor */
    EZHAL_EXTI_PULL_DOWN            /**< Enable internal pull-down resistor */
} ezhal_exti_pull_t;

/**
 * @brief EXTI Callback function prototype.
 */
typedef void (*ezhal_exti_callback_t)(uint32_t pin);

/**
 * @brief Initialize and configure an external interrupt on a specific pin with pull settings.
 * @return int 0 on success, -1 on failure.
 */
int ezhal_exti_init(uint32_t pin, ezhal_exti_trigger_t trigger, ezhal_exti_pull_t pull, ezhal_exti_callback_t callback);

int ezhal_exti_enable(uint32_t pin);
int ezhal_exti_disable(uint32_t pin);
int ezhal_exti_clear(uint32_t pin);

#ifdef __cplusplus
}
#endif

#endif //EZHAL_EXTI_H
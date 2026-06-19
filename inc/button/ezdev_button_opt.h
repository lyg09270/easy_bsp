#ifndef EZDEV_BUTTON_OPT_H
#define EZDEV_BUTTON_OPT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EZDEV_BUTTON_STATE_RELEASED = 0,
    EZDEV_BUTTON_STATE_PRESSED,
} ezdev_button_state_t;

typedef enum {
    EZDEV_BUTTON_EVENT_PRESS = 0,
    EZDEV_BUTTON_EVENT_RELEASE,
    EZDEV_BUTTON_EVENT_CLICK,
    EZDEV_BUTTON_EVENT_LONG_PRESS,
    EZDEV_BUTTON_EVENT_REPEAT,
} ezdev_button_event_t;

typedef struct {
    uint16_t debounce_ms;
    uint16_t long_press_ms;
    uint16_t repeat_ms;
} ezdev_button_info_t;

/*
 * 底层只负责：
 * 1. 初始化硬件
 * 2. 读取当前是否按下
 *
 * 消抖、长按、连发、事件都放在 dev 层。
 */
typedef struct {
    int (*init)(uint8_t subid);
    int (*deinit)(uint8_t subid);
    int (*read_pressed)(uint8_t subid, bool *pressed);
} ezdev_button_opt_t;

#ifdef __cplusplus
}
#endif

#endif
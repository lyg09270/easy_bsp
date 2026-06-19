#ifndef EZDEV_BUTTON_H
#define EZDEV_BUTTON_H

#include <stdbool.h>
#include <stdint.h>

#include "button/ezdev_button_opt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EZDEV_BUTTON_UP = 0,
    EZDEV_BUTTON_DOWN,
    EZDEV_BUTTON_LEFT,
    EZDEV_BUTTON_RIGHT,
    EZDEV_BUTTON_OK,
    EZDEV_BUTTON_BACK,
    EZDEV_BUTTON_MAX,
} ezdev_button_id_t;

typedef void (*ezdev_button_cb_t)(
    uint8_t dev_id,
    ezdev_button_event_t event,
    void *user_data);

int ezdev_button_init(uint8_t dev_id);

int ezdev_button_init_all(void);

int ezdev_button_poll(uint32_t now_ms);

int ezdev_button_get_state(uint8_t dev_id,
                           ezdev_button_state_t *state);

int ezdev_button_set_callback(uint8_t dev_id,
                              ezdev_button_cb_t callback,
                              void *user_data);

#ifdef __cplusplus
}
#endif

#endif
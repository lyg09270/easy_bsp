#include "button/ezdev_button.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "button/ezdev_button_pcf8574.h"

#define BUTTON_DEBOUNCE_MS      30U
#define BUTTON_LONG_PRESS_MS    1500U
#define BUTTON_REPEAT_MS        500U

typedef struct {
    uint8_t subid;
    const ezdev_button_opt_t *opt;

    ezdev_button_cb_t callback;
    void *user_data;

    bool raw_pressed;
    bool stable_pressed;
    bool long_sent;
    bool initialized;

    uint32_t last_change_ms;
    uint32_t press_start_ms;
    uint32_t last_repeat_ms;
} ezdev_button_ctx_t;

static ezdev_button_ctx_t s_buttons[EZDEV_BUTTON_MAX] = {
    [EZDEV_BUTTON_UP] = {
        .subid = 0,
        .opt = &ezdev_button_pcf8574_opt,
    },
    [EZDEV_BUTTON_DOWN] = {
        .subid = 1,
        .opt = &ezdev_button_pcf8574_opt,
    },
    [EZDEV_BUTTON_LEFT] = {
        .subid = 2,
        .opt = &ezdev_button_pcf8574_opt,
    },
    [EZDEV_BUTTON_RIGHT] = {
        .subid = 3,
        .opt = &ezdev_button_pcf8574_opt,
    },
    [EZDEV_BUTTON_OK] = {
        .subid = 4,
        .opt = &ezdev_button_pcf8574_opt,
    },
    [EZDEV_BUTTON_BACK] = {
        .subid = 5,
        .opt = &ezdev_button_pcf8574_opt,
    },
};

static void button_emit(ezdev_button_ctx_t *ctx,
                        uint8_t dev_id,
                        ezdev_button_event_t event)
{
    if (ctx->callback != NULL) {
        ctx->callback(dev_id, event, ctx->user_data);
    }
}

int ezdev_button_init(uint8_t dev_id)
{
    ezdev_button_ctx_t *ctx;
    bool pressed = false;

    if (dev_id >= EZDEV_BUTTON_MAX) {
        return -1;
    }

    ctx = &s_buttons[dev_id];

    if (ctx->opt == NULL ||
        ctx->opt->init == NULL ||
        ctx->opt->read_pressed == NULL) {
        return -1;
    }

    if (ctx->initialized) {
        return 0;
    }

    if (ctx->opt->init(ctx->subid) != 0) {
        return -1;
    }

    if (ctx->opt->read_pressed(ctx->subid, &pressed) != 0) {
        return -1;
    }

    ctx->raw_pressed = pressed;
    ctx->stable_pressed = pressed;
    ctx->long_sent = false;
    ctx->last_change_ms = 0;
    ctx->press_start_ms = 0;
    ctx->last_repeat_ms = 0;
    ctx->initialized = true;

    return 0;
}

int ezdev_button_init_all(void)
{
    for (uint8_t i = 0; i < EZDEV_BUTTON_MAX; i++) {
        if (ezdev_button_init(i) != 0) {
            return -1;
        }
    }

    return 0;
}

int ezdev_button_set_callback(uint8_t dev_id,
                              ezdev_button_cb_t callback,
                              void *user_data)
{
    if (dev_id >= EZDEV_BUTTON_MAX) {
        return -1;
    }

    s_buttons[dev_id].callback = callback;
    s_buttons[dev_id].user_data = user_data;

    return 0;
}

int ezdev_button_get_state(uint8_t dev_id,
                           ezdev_button_state_t *state)
{
    if (dev_id >= EZDEV_BUTTON_MAX || state == NULL) {
        return -1;
    }

    *state = s_buttons[dev_id].stable_pressed
        ? EZDEV_BUTTON_STATE_PRESSED
        : EZDEV_BUTTON_STATE_RELEASED;

    return 0;
}

int ezdev_button_poll(uint32_t now_ms)
{
    for (uint8_t i = 0; i < EZDEV_BUTTON_MAX; i++) {
        ezdev_button_ctx_t *ctx = &s_buttons[i];
        bool raw;

        if (!ctx->initialized ||
            ctx->opt == NULL ||
            ctx->opt->read_pressed == NULL) {
            continue;
        }

        if (ctx->opt->read_pressed(ctx->subid, &raw) != 0) {
            continue;
        }

        if (raw != ctx->raw_pressed) {
            ctx->raw_pressed = raw;
            ctx->last_change_ms = now_ms;
        }

        if ((now_ms - ctx->last_change_ms) < BUTTON_DEBOUNCE_MS) {
            continue;
        }

        if (ctx->stable_pressed != ctx->raw_pressed) {
            ctx->stable_pressed = ctx->raw_pressed;

            if (ctx->stable_pressed) {
                ctx->press_start_ms = now_ms;
                ctx->last_repeat_ms = now_ms;
                ctx->long_sent = false;
                button_emit(ctx, i, EZDEV_BUTTON_EVENT_PRESS);
            } else {
                button_emit(ctx, i, EZDEV_BUTTON_EVENT_RELEASE);

                if (!ctx->long_sent) {
                    button_emit(ctx, i, EZDEV_BUTTON_EVENT_CLICK);
                }
            }
        }

        if (ctx->stable_pressed) {
            if (!ctx->long_sent &&
                (now_ms - ctx->press_start_ms) >= BUTTON_LONG_PRESS_MS) {
                ctx->long_sent = true;
                ctx->last_repeat_ms = now_ms;
                button_emit(ctx, i, EZDEV_BUTTON_EVENT_LONG_PRESS);
            }

            if (ctx->long_sent &&
                (now_ms - ctx->last_repeat_ms) >= BUTTON_REPEAT_MS) {
                ctx->last_repeat_ms = now_ms;
                button_emit(ctx, i, EZDEV_BUTTON_EVENT_REPEAT);
            }
        }
    }

    return 0;
}
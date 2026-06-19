#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "display/ezdev_display.h"
#include "display/ezdev_display_ssd1681.h"

/*
 * Clean display core.
 *
 * Only one framebuffer is kept here: physical MONO1 framebuffer.
 * Upper layers write logical MONO1 pixels; this file maps them to physical
 * coordinates according to rotation.
 *
 * SSD1681 driver never sees logical size or rotation.
 * LVGL never sees physical RAM size.
 */

#define DISPLAY0_MAX_WIDTH       250U
#define DISPLAY0_MAX_HEIGHT      250U
#define MONO_STRIDE(w)           (((w) + 7U) / 8U)
#define DISPLAY0_FB_BYTES        (MONO_STRIDE(DISPLAY0_MAX_WIDTH) * DISPLAY0_MAX_HEIGHT)

#ifndef DISPLAY0_DEFAULT_ROTATION
#define DISPLAY0_DEFAULT_ROTATION EZDEV_DISPLAY_ROTATION_270
#endif

typedef struct {
    uint8_t subid;
    const ezdev_panel_opt_t *panel;
    const ezdev_panel_info_t *panel_info;

    ezdev_display_rotation_t rotation;
    ezdev_display_info_t logical_info;

    uint8_t *physical_fb;
    size_t physical_stride;

    ezdev_display_area_t dirty_physical;
    bool dirty_valid;
    bool initialized;
} ezdev_display_ctx_t;

static uint8_t s_display0_fb[DISPLAY0_FB_BYTES]
    __attribute__((aligned(4)));

static ezdev_display_ctx_t s_display_ctx[EZDEV_DISPLAY_DEVICE_MAX] = {
    [EZDEV_DISPLAY_DEVICE_0] = {
        .subid = SSD1681_DEV_0,
        .panel = &ezdev_ssd1681_panel_opt,
        .panel_info = &ezdev_ssd1681_panel_info,
        .rotation = DISPLAY0_DEFAULT_ROTATION,
        .physical_fb = s_display0_fb,
    },
};

static ezdev_display_ctx_t *display_get_ctx(uint8_t dev_id)
{
    if (dev_id >= EZDEV_DISPLAY_DEVICE_MAX) {
        return NULL;
    }

    return &s_display_ctx[dev_id];
}

static size_t mono_stride(uint16_t width)
{
    return ((size_t)width + 7U) / 8U;
}

static size_t min_stride(ezdev_display_format_t format,
                         uint16_t width)
{
    switch (format) {
    case EZDEV_DISPLAY_FORMAT_MONO_1:
        return mono_stride(width);
    case EZDEV_DISPLAY_FORMAT_GRAY_2:
        return ((size_t)width + 3U) / 4U;
    case EZDEV_DISPLAY_FORMAT_RGB565:
        return (size_t)width * 2U;
    case EZDEV_DISPLAY_FORMAT_RGB888:
        return (size_t)width * 3U;
    default:
        return 0;
    }
}

static void update_logical_info(ezdev_display_ctx_t *ctx)
{
    uint16_t pw = ctx->panel_info->width;
    uint16_t ph = ctx->panel_info->height;

    ctx->logical_info.format = ctx->panel_info->format;
    ctx->logical_info.support_partial_refresh =
        ctx->panel_info->support_partial_refresh;

    if (ctx->rotation == EZDEV_DISPLAY_ROTATION_90 ||
        ctx->rotation == EZDEV_DISPLAY_ROTATION_270) {
        ctx->logical_info.width = ph;
        ctx->logical_info.height = pw;
    } else {
        ctx->logical_info.width = pw;
        ctx->logical_info.height = ph;
    }
}

static bool logical_area_valid(const ezdev_display_ctx_t *ctx,
                               const ezdev_display_area_t *area)
{
    if (ctx == NULL || area == NULL ||
        area->width == 0U || area->height == 0U) {
        return false;
    }

    return (uint32_t)area->x + area->width <= ctx->logical_info.width &&
           (uint32_t)area->y + area->height <= ctx->logical_info.height;
}

static bool mono_get_pixel(const uint8_t *fb,
                           size_t stride,
                           uint16_t x,
                           uint16_t y)
{
    const uint8_t *p = fb + (size_t)y * stride + x / 8U;
    return (*p & (uint8_t)(0x80U >> (x & 7U))) != 0U;
}

static void mono_set_pixel(uint8_t *fb,
                           size_t stride,
                           uint16_t x,
                           uint16_t y,
                           bool white)
{
    uint8_t *p = fb + (size_t)y * stride + x / 8U;

    if (white) {
        *p |= (uint8_t)(0x80U >> (x & 7U));
    } else {
        *p &= (uint8_t)~(0x80U >> (x & 7U));
    }
}

static void map_xy(const ezdev_display_ctx_t *ctx,
                   uint16_t lx,
                   uint16_t ly,
                   uint16_t *px,
                   uint16_t *py)
{
    uint16_t pw = ctx->panel_info->width;
    uint16_t ph = ctx->panel_info->height;

    switch (ctx->rotation) {
    case EZDEV_DISPLAY_ROTATION_0:
        *px = lx;
        *py = ly;
        break;

    case EZDEV_DISPLAY_ROTATION_90:
        *px = (uint16_t)(pw - 1U - ly);
        *py = lx;
        break;

    case EZDEV_DISPLAY_ROTATION_180:
        *px = (uint16_t)(pw - 1U - lx);
        *py = (uint16_t)(ph - 1U - ly);
        break;

    case EZDEV_DISPLAY_ROTATION_270:
        *px = ly;
        *py = (uint16_t)(ph - 1U - lx);
        break;

    default:
        *px = lx;
        *py = ly;
        break;
    }
}

static uint16_t align_down(uint16_t value,
                           uint8_t alignment)
{
    if (alignment <= 1U) {
        return value;
    }

    return (uint16_t)(value - value % alignment);
}

static uint16_t align_up_end(uint16_t value,
                             uint8_t alignment,
                             uint16_t maximum)
{
    uint32_t aligned;

    if (alignment <= 1U) {
        return value > maximum ? maximum : value;
    }

    aligned = ((uint32_t)value + alignment) /
              alignment * alignment - 1U;

    if (aligned > maximum) {
        aligned = maximum;
    }

    return (uint16_t)aligned;
}

static void merge_dirty(ezdev_display_ctx_t *ctx,
                        uint16_t min_x,
                        uint16_t min_y,
                        uint16_t max_x,
                        uint16_t max_y)
{
    ezdev_display_area_t a;
    uint16_t old_x2;
    uint16_t old_y2;
    uint16_t new_x2;
    uint16_t new_y2;

    min_x = align_down(min_x, ctx->panel_info->x_alignment);
    max_x = align_up_end(max_x,
                         ctx->panel_info->x_alignment,
                         (uint16_t)(ctx->panel_info->width - 1U));

    if (max_y >= ctx->panel_info->height) {
        max_y = (uint16_t)(ctx->panel_info->height - 1U);
    }

    a.x = min_x;
    a.y = min_y;
    a.width = (uint16_t)(max_x - min_x + 1U);
    a.height = (uint16_t)(max_y - min_y + 1U);

    if (!ctx->dirty_valid) {
        ctx->dirty_physical = a;
        ctx->dirty_valid = true;
        return;
    }

    old_x2 = (uint16_t)(ctx->dirty_physical.x +
                        ctx->dirty_physical.width);
    old_y2 = (uint16_t)(ctx->dirty_physical.y +
                        ctx->dirty_physical.height);
    new_x2 = (uint16_t)(a.x + a.width);
    new_y2 = (uint16_t)(a.y + a.height);

    if (a.x < ctx->dirty_physical.x) {
        ctx->dirty_physical.x = a.x;
    }

    if (a.y < ctx->dirty_physical.y) {
        ctx->dirty_physical.y = a.y;
    }

    if (new_x2 > old_x2) {
        old_x2 = new_x2;
    }

    if (new_y2 > old_y2) {
        old_y2 = new_y2;
    }

    ctx->dirty_physical.width =
        (uint16_t)(old_x2 - ctx->dirty_physical.x);
    ctx->dirty_physical.height =
        (uint16_t)(old_y2 - ctx->dirty_physical.y);
}

int ezdev_display_init(uint8_t dev_id)
{
    ezdev_display_ctx_t *ctx = display_get_ctx(dev_id);
    int ret;

    if (ctx == NULL || ctx->panel == NULL || ctx->panel_info == NULL ||
        ctx->panel->init == NULL ||
        ctx->panel->write_physical_area == NULL ||
        ctx->panel->present_physical == NULL ||
        ctx->physical_fb == NULL) {
        return -1;
    }

    if (ctx->initialized) {
        return 0;
    }

    if (ctx->panel_info->format != EZDEV_DISPLAY_FORMAT_MONO_1 ||
        ctx->panel_info->width > DISPLAY0_MAX_WIDTH ||
        ctx->panel_info->height > DISPLAY0_MAX_HEIGHT) {
        return -1;
    }

    ctx->physical_stride = mono_stride(ctx->panel_info->width);
    update_logical_info(ctx);

    memset(ctx->physical_fb,
           0xFF,
           ctx->physical_stride * ctx->panel_info->height);

    ctx->dirty_valid = false;

    ret = ctx->panel->init(ctx->subid);

    if (ret == 0) {
        ctx->initialized = true;
    }

    return ret;
}

int ezdev_display_get_info(uint8_t dev_id,
                           ezdev_display_info_t *info)
{
    ezdev_display_ctx_t *ctx = display_get_ctx(dev_id);

    if (ctx == NULL || info == NULL) {
        return -1;
    }

    update_logical_info(ctx);
    *info = ctx->logical_info;
    return 0;
}

int ezdev_display_set_rotation(uint8_t dev_id,
                               ezdev_display_rotation_t rotation)
{
    ezdev_display_ctx_t *ctx = display_get_ctx(dev_id);

    if (ctx == NULL) {
        return -1;
    }

    if (rotation != EZDEV_DISPLAY_ROTATION_0 &&
        rotation != EZDEV_DISPLAY_ROTATION_90 &&
        rotation != EZDEV_DISPLAY_ROTATION_180 &&
        rotation != EZDEV_DISPLAY_ROTATION_270) {
        return -1;
    }

    ctx->rotation = rotation;
    update_logical_info(ctx);

    if (ctx->physical_fb != NULL && ctx->panel_info != NULL) {
        ctx->physical_stride = mono_stride(ctx->panel_info->width);
        memset(ctx->physical_fb,
               0xFF,
               ctx->physical_stride * ctx->panel_info->height);
    }

    ctx->dirty_valid = false;
    return 0;
}

int ezdev_display_get_rotation(uint8_t dev_id,
                               ezdev_display_rotation_t *rotation)
{
    ezdev_display_ctx_t *ctx = display_get_ctx(dev_id);

    if (ctx == NULL || rotation == NULL) {
        return -1;
    }

    *rotation = ctx->rotation;
    return 0;
}

int ezdev_display_write_area(uint8_t dev_id,
                             const ezdev_display_area_t *area,
                             const void *pixels,
                             size_t stride_bytes)
{
    ezdev_display_ctx_t *ctx = display_get_ctx(dev_id);
    const uint8_t *src = pixels;
    size_t need_stride;
    uint16_t min_x = UINT16_MAX;
    uint16_t min_y = UINT16_MAX;
    uint16_t max_x = 0U;
    uint16_t max_y = 0U;
    bool have_pixel = false;

    if (ctx == NULL || !ctx->initialized || pixels == NULL ||
        !logical_area_valid(ctx, area)) {
        return -1;
    }

    need_stride = min_stride(ctx->logical_info.format, area->width);

    if (need_stride == 0U || stride_bytes < need_stride) {
        return -1;
    }

    for (uint16_t y = 0; y < area->height; y++) {
        for (uint16_t x = 0; x < area->width; x++) {
            uint16_t lx = (uint16_t)(area->x + x);
            uint16_t ly = (uint16_t)(area->y + y);
            uint16_t px;
            uint16_t py;
            bool white;

            white = mono_get_pixel(src, stride_bytes, x, y);
            map_xy(ctx, lx, ly, &px, &py);

            if (px >= ctx->panel_info->width ||
                py >= ctx->panel_info->height) {
                return -1;
            }

            mono_set_pixel(ctx->physical_fb,
                           ctx->physical_stride,
                           px,
                           py,
                           white);

            if (!have_pixel) {
                min_x = max_x = px;
                min_y = max_y = py;
                have_pixel = true;
            } else {
                if (px < min_x) { min_x = px; }
                if (py < min_y) { min_y = py; }
                if (px > max_x) { max_x = px; }
                if (py > max_y) { max_y = py; }
            }
        }
    }

    if (have_pixel) {
        merge_dirty(ctx, min_x, min_y, max_x, max_y);
    }

    return 0;
}

int ezdev_display_present(uint8_t dev_id,
                          const ezdev_display_area_t *area,
                          ezdev_display_refresh_t mode)
{
    ezdev_display_ctx_t *ctx = display_get_ctx(dev_id);
    const uint8_t *source;
    ezdev_display_refresh_t panel_mode;

    (void)area;

    if (ctx == NULL || !ctx->initialized) {
        return -1;
    }

    if (!ctx->dirty_valid) {
        return 0;
    }

    panel_mode = mode;

    if (panel_mode == EZDEV_DISPLAY_REFRESH_AUTO) {
        panel_mode = EZDEV_DISPLAY_REFRESH_PARTIAL;
    }

    if (panel_mode == EZDEV_DISPLAY_REFRESH_PARTIAL &&
        !ctx->panel_info->support_partial_refresh) {
        panel_mode = EZDEV_DISPLAY_REFRESH_FULL;
    }

    source = ctx->physical_fb +
             (size_t)ctx->dirty_physical.y * ctx->physical_stride +
             ctx->dirty_physical.x / 8U;

    if (ctx->panel->write_physical_area(ctx->subid,
                                        &ctx->dirty_physical,
                                        source,
                                        ctx->physical_stride) != 0) {
        return -1;
    }

    if (ctx->panel->present_physical(ctx->subid,
                                     &ctx->dirty_physical,
                                     panel_mode) != 0) {
        return -1;
    }

    ctx->dirty_valid = false;
    return 0;
}

int ezdev_display_set_power(uint8_t dev_id,
                            ezdev_display_power_t mode)
{
    ezdev_display_ctx_t *ctx = display_get_ctx(dev_id);

    if (ctx == NULL || !ctx->initialized ||
        ctx->panel->set_power == NULL) {
        return -1;
    }

    return ctx->panel->set_power(ctx->subid, mode);
}

int ezdev_display_is_busy(uint8_t dev_id,
                          bool *busy)
{
    ezdev_display_ctx_t *ctx = display_get_ctx(dev_id);

    if (ctx == NULL || !ctx->initialized || busy == NULL) {
        return -1;
    }

    if (ctx->panel->is_busy == NULL) {
        *busy = false;
        return 0;
    }

    return ctx->panel->is_busy(ctx->subid, busy);
}

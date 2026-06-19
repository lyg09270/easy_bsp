#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "display/ezdev_display_ssd1681.h"

#include "ezhal/ezhal_gpio.h"
#include "ezhal/ezhal_spi.h"
#include "ezbsp_time.h"

/* ======================== 面板配置 ======================== */

#define SSD1681_WIDTH                    122U
#define SSD1681_HEIGHT                   250U
#define SSD1681_ROW_BYTES                ((SSD1681_WIDTH + 7U) / 8U)
#define SSD1681_FRAME_BYTES              (SSD1681_ROW_BYTES * SSD1681_HEIGHT)

#define SSD1681_SPI_BUS                  EZHAL_SPI_BUS_0
#define SSD1681_SPI_SPEED_HZ             20000000U

#define SSD1681_PIN_RST                  10U
#define SSD1681_PIN_DC                   11U
#define SSD1681_PIN_CS                   18U
#define SSD1681_PIN_BUSY                 2U

#define SSD1681_BUSY_ACTIVE_LEVEL        EZHAL_GPIO_LEVEL_HIGH
#define SSD1681_BUSY_TIMEOUT_MS          10000U

/*
 * 连续局刷达到该次数后，自动执行一次全刷，减轻残影。
 */
#define SSD1681_PARTIAL_REFRESH_LIMIT    50U
#define SSD1681_PARTIAL_MAX_PIXELS       \
    (SSD1681_WIDTH * SSD1681_HEIGHT)

/* ======================== 控制命令 ======================== */

#define SSD1681_CMD_DRIVER_OUTPUT_CTRL   0x01U
#define SSD1681_CMD_GATE_DRIVING_VOLTAGE 0x03U
#define SSD1681_CMD_SOURCE_DRIVING_VOLT  0x04U
#define SSD1681_CMD_DEEP_SLEEP           0x10U
#define SSD1681_CMD_DATA_ENTRY_MODE      0x11U
#define SSD1681_CMD_SOFT_RESET           0x12U
#define SSD1681_CMD_TEMP_SENSOR_CTRL     0x18U
#define SSD1681_CMD_MASTER_ACTIVATE      0x20U
#define SSD1681_CMD_DISPLAY_UPDATE_CTRL  0x22U
#define SSD1681_CMD_WRITE_RAM_NEW        0x24U
#define SSD1681_CMD_WRITE_RAM_OLD        0x26U
#define SSD1681_CMD_WRITE_VCOM           0x2CU
#define SSD1681_CMD_WRITE_LUT            0x32U
#define SSD1681_CMD_WRITE_OTP_SELECTION  0x37U
#define SSD1681_CMD_BORDER_WAVEFORM      0x3CU
#define SSD1681_CMD_GATE_SETTING         0x3FU
#define SSD1681_CMD_SET_RAM_X_RANGE      0x44U
#define SSD1681_CMD_SET_RAM_Y_RANGE      0x45U
#define SSD1681_CMD_SET_RAM_X_COUNTER    0x4EU
#define SSD1681_CMD_SET_RAM_Y_COUNTER    0x4FU

/* ========================== LUT =========================== */

/*
 * 该 LUT 来自 Waveshare 1.54inch e-Paper V2（200×200，SSD1681）
 * 官方参考驱动。
 *
 * 如果你的面板不是该型号，局刷出现严重残影、闪烁或不刷新时，
 * 应替换为面板厂商提供的 LUT。
 */
static const uint8_t ssd1681_lut_full[159] = {
    0x80, 0x48, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x40, 0x48, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x80, 0x48, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x40, 0x48, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

    0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x01, 0x00, 0x08, 0x01, 0x00, 0x02,
    0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
    0x00, 0x00, 0x00,

    0x22, 0x17, 0x41, 0x00, 0x32, 0x20,
};

static const uint8_t ssd1681_lut_partial[159] = {
    0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

    0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
    0x00, 0x00, 0x00,

    0x02, 0x17, 0x41, 0xB0, 0x32, 0x28,
};

/* ======================== 驱动上下文 ====================== */

typedef enum {
    SSD1681_REFRESH_CONFIG_NONE = 0,
    SSD1681_REFRESH_CONFIG_FULL,
    SSD1681_REFRESH_CONFIG_PARTIAL,
} ssd1681_refresh_config_t;

typedef struct {
    ezhal_spi_dev_t spi_dev;

    uint8_t *frame;

    ezdev_display_area_t dirty_area;

    ssd1681_refresh_config_t refresh_config;

    uint8_t partial_refresh_count;

    bool initialized;
    bool sleeping;
    bool dirty_valid;
    bool base_frame_valid;
} ssd1681_ctx_t;

const ezdev_panel_info_t ezdev_ssd1681_panel_info = {
    .width = SSD1681_WIDTH,
    .height = SSD1681_HEIGHT,
    .format = EZDEV_DISPLAY_FORMAT_MONO_1,
    .x_alignment = 8,
    .y_alignment = 1,
    .support_partial_refresh = true,
};

static uint8_t ssd1681_frames[SSD1681_DEV_MAX][SSD1681_FRAME_BYTES]
    __attribute__((aligned(4)));

static ssd1681_ctx_t ssd1681_ctxs[SSD1681_DEV_MAX] = {
    [SSD1681_DEV_0] = {
        .spi_dev = EZHAL_SPI_DEV_INVALID,
    },
};

static ssd1681_ctx_t *ssd1681_get_ctx(uint8_t subid)
{
    if (subid >= SSD1681_DEV_MAX) {
        return NULL;
    }

    return &ssd1681_ctxs[subid];
}

/* ========================== GPIO ========================== */

static int ssd1681_gpio_init(void)
{
    if (ezhal_gpio_init(
            SSD1681_PIN_RST,
            EZHAL_GPIO_MODE_OUTPUT_PP) != 0 ||
        ezhal_gpio_init(
            SSD1681_PIN_DC,
            EZHAL_GPIO_MODE_OUTPUT_PP) != 0 ||
        ezhal_gpio_init(
            SSD1681_PIN_BUSY,
            EZHAL_GPIO_MODE_INPUT) != 0) {
        return -1;
    }

    if (ezhal_gpio_write(
            SSD1681_PIN_RST,
            EZHAL_GPIO_LEVEL_HIGH) != 0 ||
        ezhal_gpio_write(
            SSD1681_PIN_DC,
            EZHAL_GPIO_LEVEL_HIGH) != 0) {
        return -1;
    }

    return 0;
}

static int ssd1681_hardware_reset(void)
{
    if (ezhal_gpio_write(
            SSD1681_PIN_RST,
            EZHAL_GPIO_LEVEL_HIGH) != 0) {
        return -1;
    }

    ezbsp_delay_ms(20);

    if (ezhal_gpio_write(
            SSD1681_PIN_RST,
            EZHAL_GPIO_LEVEL_LOW) != 0) {
        return -1;
    }

    ezbsp_delay_ms(5);

    if (ezhal_gpio_write(
            SSD1681_PIN_RST,
            EZHAL_GPIO_LEVEL_HIGH) != 0) {
        return -1;
    }

    ezbsp_delay_ms(20);

    return 0;
}

/* ========================== BUSY ========================== */

static bool ssd1681_busy(void)
{
    return ezhal_gpio_read(SSD1681_PIN_BUSY) ==
           SSD1681_BUSY_ACTIVE_LEVEL;
}

static int ssd1681_wait_idle(uint32_t timeout_ms)
{
    uint32_t start = ezbsp_time_ms();

    while (ssd1681_busy()) {
        if ((ezbsp_time_ms() - start) >= timeout_ms) {
            return -1;
        }

        ezbsp_delay_ms(5);
    }

    return 0;
}

/* =========================== SPI ========================== */

static int ssd1681_write_command(ssd1681_ctx_t *ctx,
                                 uint8_t command)
{
    if (ezhal_gpio_write(
            SSD1681_PIN_DC,
            EZHAL_GPIO_LEVEL_LOW) != 0) {
        return -1;
    }

    return ezhal_spi_transfer(
        ctx->spi_dev,
        &command,
        NULL,
        1);
}

static int ssd1681_write_data(ssd1681_ctx_t *ctx,
                              const void *data,
                              size_t length)
{
    if (data == NULL || length == 0) {
        return -1;
    }

    if (ezhal_gpio_write(
            SSD1681_PIN_DC,
            EZHAL_GPIO_LEVEL_HIGH) != 0) {
        return -1;
    }

    return ezhal_spi_transfer(
        ctx->spi_dev,
        data,
        NULL,
        length);
}

static int ssd1681_write_command_data(ssd1681_ctx_t *ctx,
                                      uint8_t command,
                                      const void *data,
                                      size_t length)
{
    if (ssd1681_write_command(ctx, command) != 0) {
        return -1;
    }

    if (length == 0) {
        return 0;
    }

    return ssd1681_write_data(ctx, data, length);
}

/* ========================= 区域工具 ======================= */

static bool ssd1681_area_valid(const ezdev_display_area_t *area)
{
    uint32_t x_end;
    uint32_t y_end;

    if (area == NULL ||
        area->width == 0 ||
        area->height == 0) {
        return false;
    }

    if ((area->x & 0x07U) != 0) {
        return false;
    }

    x_end = (uint32_t)area->x + area->width;
    y_end = (uint32_t)area->y + area->height;

    return x_end <= SSD1681_WIDTH &&
           y_end <= SSD1681_HEIGHT;
}

static void ssd1681_merge_dirty_area(
    ssd1681_ctx_t *ctx,
    const ezdev_display_area_t *area)
{
    uint16_t x1;
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;

    if (!ctx->dirty_valid) {
        ctx->dirty_area = *area;
        ctx->dirty_valid = true;
        return;
    }

    x1 = ctx->dirty_area.x < area->x
        ? ctx->dirty_area.x
        : area->x;

    y1 = ctx->dirty_area.y < area->y
        ? ctx->dirty_area.y
        : area->y;

    x2 =
        (uint16_t)(ctx->dirty_area.x +
                   ctx->dirty_area.width);

    if ((uint16_t)(area->x + area->width) > x2) {
        x2 = (uint16_t)(area->x + area->width);
    }

    y2 =
        (uint16_t)(ctx->dirty_area.y +
                   ctx->dirty_area.height);

    if ((uint16_t)(area->y + area->height) > y2) {
        y2 = (uint16_t)(area->y + area->height);
    }

    ctx->dirty_area.x = x1;
    ctx->dirty_area.y = y1;
    ctx->dirty_area.width = (uint16_t)(x2 - x1);
    ctx->dirty_area.height = (uint16_t)(y2 - y1);
}

/* ========================= RAM 地址 ======================= */

static int ssd1681_set_window(
    ssd1681_ctx_t *ctx,
    const ezdev_display_area_t *area)
{
    uint8_t x_range[2];
    uint8_t y_range[4];
    uint8_t x_counter;
    uint8_t y_counter[2];

    uint16_t y_start = area->y;
    uint16_t y_end =
        (uint16_t)(area->y + area->height - 1U);

    x_range[0] = (uint8_t)(area->x / 8U);
    x_range[1] =
        (uint8_t)((area->x +
                   area->width - 1U) / 8U);

    y_range[0] = (uint8_t)(y_start & 0xFFU);
    y_range[1] =
        (uint8_t)((y_start >> 8U) & 0xFFU);
    y_range[2] = (uint8_t)(y_end & 0xFFU);
    y_range[3] =
        (uint8_t)((y_end >> 8U) & 0xFFU);

    x_counter = x_range[0];

    y_counter[0] = y_range[0];
    y_counter[1] = y_range[1];

    if (ssd1681_write_command_data(
            ctx,
            SSD1681_CMD_SET_RAM_X_RANGE,
            x_range,
            sizeof(x_range)) != 0) {
        return -1;
    }

    if (ssd1681_write_command_data(
            ctx,
            SSD1681_CMD_SET_RAM_Y_RANGE,
            y_range,
            sizeof(y_range)) != 0) {
        return -1;
    }

    if (ssd1681_write_command_data(
            ctx,
            SSD1681_CMD_SET_RAM_X_COUNTER,
            &x_counter,
            sizeof(x_counter)) != 0) {
        return -1;
    }

    return ssd1681_write_command_data(
        ctx,
        SSD1681_CMD_SET_RAM_Y_COUNTER,
        y_counter,
        sizeof(y_counter));
}

static int ssd1681_write_ram_area(
    ssd1681_ctx_t *ctx,
    uint8_t ram_command,
    const ezdev_display_area_t *area,
    const uint8_t *frame)
{
    size_t row_bytes;
    size_t x_byte;
    uint16_t row;

    if (ssd1681_set_window(ctx, area) != 0) {
        return -1;
    }

    if (ssd1681_write_command(ctx, ram_command) != 0) {
        return -1;
    }

    row_bytes = (area->width + 7U) / 8U;
    x_byte = area->x / 8U;

    if (area->x == 0 &&
        area->width == SSD1681_WIDTH) {
        const uint8_t *source =
            frame +
            (size_t)area->y * SSD1681_ROW_BYTES;

        return ssd1681_write_data(
            ctx,
            source,
            row_bytes * area->height);
    }

    for (row = 0; row < area->height; row++) {
        const uint8_t *source =
            frame +
            ((size_t)area->y + row) *
                SSD1681_ROW_BYTES +
            x_byte;

        if (ssd1681_write_data(
                ctx,
                source,
                row_bytes) != 0) {
            return -1;
        }
    }

    return 0;
}

/* ========================== LUT =========================== */

static int ssd1681_load_lut(
    ssd1681_ctx_t *ctx,
    const uint8_t lut[159])
{
    if (ssd1681_write_command_data(
            ctx,
            SSD1681_CMD_WRITE_LUT,
            lut,
            153) != 0) {
        return -1;
    }

    if (ssd1681_wait_idle(
            SSD1681_BUSY_TIMEOUT_MS) != 0) {
        return -1;
    }

    if (ssd1681_write_command_data(
            ctx,
            SSD1681_CMD_GATE_SETTING,
            &lut[153],
            1) != 0) {
        return -1;
    }

    if (ssd1681_write_command_data(
            ctx,
            SSD1681_CMD_GATE_DRIVING_VOLTAGE,
            &lut[154],
            1) != 0) {
        return -1;
    }

    if (ssd1681_write_command_data(
            ctx,
            SSD1681_CMD_SOURCE_DRIVING_VOLT,
            &lut[155],
            3) != 0) {
        return -1;
    }

    return ssd1681_write_command_data(
        ctx,
        SSD1681_CMD_WRITE_VCOM,
        &lut[158],
        1);
}

static int ssd1681_configure_full_refresh(
    ssd1681_ctx_t *ctx)
{
    const uint8_t border = 0x01U;
    const uint8_t temp_sensor = 0x80U;
    const uint8_t load_waveform = 0xB1U;

    if (ctx->refresh_config ==
        SSD1681_REFRESH_CONFIG_FULL) {
        return 0;
    }

    if (ssd1681_wait_idle(
            SSD1681_BUSY_TIMEOUT_MS) != 0) {
        return -1;
    }

    if (ssd1681_write_command_data(
            ctx,
            SSD1681_CMD_BORDER_WAVEFORM,
            &border,
            1) != 0) {
        return -1;
    }

    if (ssd1681_write_command_data(
            ctx,
            SSD1681_CMD_TEMP_SENSOR_CTRL,
            &temp_sensor,
            1) != 0) {
        return -1;
    }

    if (ssd1681_write_command_data(
            ctx,
            SSD1681_CMD_DISPLAY_UPDATE_CTRL,
            &load_waveform,
            1) != 0) {
        return -1;
    }

    if (ssd1681_write_command(
            ctx,
            SSD1681_CMD_MASTER_ACTIVATE) != 0) {
        return -1;
    }

    if (ssd1681_wait_idle(
            SSD1681_BUSY_TIMEOUT_MS) != 0) {
        return -1;
    }

    if (ssd1681_load_lut(
            ctx,
            ssd1681_lut_full) != 0) {
        return -1;
    }

    ctx->refresh_config =
        SSD1681_REFRESH_CONFIG_FULL;

    return 0;
}

static int ssd1681_configure_partial_refresh(
    ssd1681_ctx_t *ctx)
{
    static const uint8_t otp_selection[10] = {
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x40, 0x00, 0x00, 0x00, 0x00,
    };

    const uint8_t border = 0x80U;
    const uint8_t load_partial = 0xC0U;

    if (ctx->refresh_config ==
        SSD1681_REFRESH_CONFIG_PARTIAL) {
        return 0;
    }

    if (ssd1681_wait_idle(
            SSD1681_BUSY_TIMEOUT_MS) != 0) {
        return -1;
    }

    if (ssd1681_load_lut(
            ctx,
            ssd1681_lut_partial) != 0) {
        return -1;
    }

    if (ssd1681_write_command_data(
            ctx,
            SSD1681_CMD_WRITE_OTP_SELECTION,
            otp_selection,
            sizeof(otp_selection)) != 0) {
        return -1;
    }

    if (ssd1681_write_command_data(
            ctx,
            SSD1681_CMD_BORDER_WAVEFORM,
            &border,
            1) != 0) {
        return -1;
    }

    if (ssd1681_write_command_data(
            ctx,
            SSD1681_CMD_DISPLAY_UPDATE_CTRL,
            &load_partial,
            1) != 0) {
        return -1;
    }

    if (ssd1681_write_command(
            ctx,
            SSD1681_CMD_MASTER_ACTIVATE) != 0) {
        return -1;
    }

    if (ssd1681_wait_idle(
            SSD1681_BUSY_TIMEOUT_MS) != 0) {
        return -1;
    }

    ctx->refresh_config =
        SSD1681_REFRESH_CONFIG_PARTIAL;

    return 0;
}

/* ======================== 面板初始化 ====================== */

static int ssd1681_panel_init(ssd1681_ctx_t *ctx)
{
    uint16_t gate_count =
        SSD1681_HEIGHT - 1U;

    uint8_t driver_output[3] = {
        (uint8_t)(gate_count & 0xFFU),
        (uint8_t)((gate_count >> 8U) & 0xFFU),
        0x00U,
    };

    const uint8_t data_entry_mode = 0x03U;

    if (ssd1681_hardware_reset() != 0) {
        return -1;
    }

    if (ssd1681_wait_idle(
            SSD1681_BUSY_TIMEOUT_MS) != 0) {
        return -1;
    }

    if (ssd1681_write_command(
            ctx,
            SSD1681_CMD_SOFT_RESET) != 0) {
        return -1;
    }

    if (ssd1681_wait_idle(
            SSD1681_BUSY_TIMEOUT_MS) != 0) {
        return -1;
    }

    if (ssd1681_write_command_data(
            ctx,
            SSD1681_CMD_DRIVER_OUTPUT_CTRL,
            driver_output,
            sizeof(driver_output)) != 0) {
        return -1;
    }

    if (ssd1681_write_command_data(
            ctx,
            SSD1681_CMD_DATA_ENTRY_MODE,
            &data_entry_mode,
            1) != 0) {
        return -1;
    }

    ctx->refresh_config =
        SSD1681_REFRESH_CONFIG_NONE;

    if (ssd1681_configure_full_refresh(ctx) != 0) {
        return -1;
    }

    ctx->sleeping = false;
    ctx->base_frame_valid = false;
    ctx->dirty_valid = false;
    ctx->partial_refresh_count = 0;

    return 0;
}

/* ========================= 刷新控制 ======================= */

static int ssd1681_start_refresh(
    ssd1681_ctx_t *ctx,
    bool partial)
{
    const uint8_t update_control =
        partial ? 0xCFU : 0xC7U;

    if (ssd1681_write_command_data(
            ctx,
            SSD1681_CMD_DISPLAY_UPDATE_CTRL,
            &update_control,
            1) != 0) {
        return -1;
    }

    return ssd1681_write_command(
        ctx,
        SSD1681_CMD_MASTER_ACTIVATE);
}

/* ========================= OPT 实现 ======================= */

static int ssd1681_init(uint8_t subid)
{
    ssd1681_ctx_t *ctx =
        ssd1681_get_ctx(subid);

    if (ctx == NULL) {
        return -1;
    }

    if (ctx->initialized) {
        return 0;
    }

    ctx->frame = ssd1681_frames[subid];

    memset(
        ctx->frame,
        0xFF,
        SSD1681_FRAME_BYTES);

    if (ssd1681_gpio_init() != 0) {
        return -1;
    }

    if (ezhal_spi_bus_init(
            SSD1681_SPI_BUS,
            EZHAL_SPI_MODE_0,
            SSD1681_SPI_SPEED_HZ) != 0) {
        return -1;
    }

    ctx->spi_dev = ezhal_spi_add_device(
        SSD1681_SPI_BUS,
        SSD1681_PIN_CS);

    if (ctx->spi_dev ==
        EZHAL_SPI_DEV_INVALID) {
        return -1;
    }

    if (ssd1681_panel_init(ctx) != 0) {
        return -1;
    }

    ctx->initialized = true;

    return 0;
}

static int ssd1681_write_physical_area(
    uint8_t subid,
    const ezdev_display_area_t *area,
    const void *pixels,
    size_t stride_bytes)
{
    ssd1681_ctx_t *ctx =
        ssd1681_get_ctx(subid);

    const uint8_t *source = pixels;
    size_t row_bytes;
    size_t destination_x;
    uint16_t row;

    if (ctx == NULL ||
        !ctx->initialized ||
        ctx->sleeping ||
        pixels == NULL ||
        !ssd1681_area_valid(area)) {
        return -1;
    }

    row_bytes = (area->width + 7U) / 8U;

    if (stride_bytes < row_bytes) {
        return -1;
    }

    destination_x = area->x / 8U;

    for (row = 0; row < area->height; row++) {
        uint8_t *destination =
            ctx->frame +
            ((size_t)area->y + row) *
                SSD1681_ROW_BYTES +
            destination_x;

        memcpy(
            destination,
            source + (size_t)row * stride_bytes,
            row_bytes);
    }

    ssd1681_merge_dirty_area(ctx, area);

    return 0;
}

static int ssd1681_present_physical(
    uint8_t subid,
    const ezdev_display_area_t *area,
    ezdev_display_refresh_t mode)
{
    ssd1681_ctx_t *ctx =
        ssd1681_get_ctx(subid);

    ezdev_display_area_t refresh_area;
    uint32_t dirty_pixels;
    bool use_partial;

    (void)area;

    if (ctx == NULL ||
        !ctx->initialized ||
        ctx->sleeping) {
        return -1;
    }

    if (ssd1681_wait_idle(
            SSD1681_BUSY_TIMEOUT_MS) != 0) {
        return -1;
    }

    if (!ctx->dirty_valid) {
        return 0;
    }

    if (mode != EZDEV_DISPLAY_REFRESH_FULL &&
        mode != EZDEV_DISPLAY_REFRESH_PARTIAL) {
        return -1;
    }

    dirty_pixels =
        (uint32_t)ctx->dirty_area.width *
        ctx->dirty_area.height;

    use_partial =
        mode == EZDEV_DISPLAY_REFRESH_PARTIAL &&
        ctx->base_frame_valid &&
        dirty_pixels <= SSD1681_PARTIAL_MAX_PIXELS &&
        ctx->partial_refresh_count <
            SSD1681_PARTIAL_REFRESH_LIMIT;

if (use_partial) {
    refresh_area = ctx->dirty_area;

    if (ssd1681_configure_partial_refresh(ctx) != 0) {
        return -1;
    }

    /*
     * 写入本次需要显示的新画面。
     */
    if (ssd1681_write_ram_area(
            ctx,
            SSD1681_CMD_WRITE_RAM_NEW,
            &refresh_area,
            ctx->frame) != 0) {
        return -1;
    }

    /*
     * 执行局部刷新。
     */
    if (ssd1681_start_refresh(ctx, true) != 0) {
        return -1;
    }

    /*
     * 必须等待物理刷新完成，之后才能更新下一次
     * 差分刷新所使用的 previous/current RAM。
     */
    if (ssd1681_wait_idle(
            SSD1681_BUSY_TIMEOUT_MS) != 0) {
        return -1;
    }

    /*
     * 将本次新画面同步为下一次局刷的基准。
     *
     * 0x26 = previous frame
     * 0x24 = current frame
     */
    if (ssd1681_write_ram_area(
            ctx,
            SSD1681_CMD_WRITE_RAM_OLD,
            &refresh_area,
            ctx->frame) != 0) {
        return -1;
    }

    if (ssd1681_write_ram_area(
            ctx,
            SSD1681_CMD_WRITE_RAM_NEW,
            &refresh_area,
            ctx->frame) != 0) {
        return -1;
    }

    ctx->partial_refresh_count++;
    } else {
        refresh_area.x = 0;
        refresh_area.y = 0;
        refresh_area.width = SSD1681_WIDTH;
        refresh_area.height = SSD1681_HEIGHT;

        if (ssd1681_configure_full_refresh(
                ctx) != 0) {
            return -1;
        }

        /*
         * 全刷时同步新旧 RAM，作为后续局刷基准帧。
         */
        if (ssd1681_write_ram_area(
                ctx,
                SSD1681_CMD_WRITE_RAM_NEW,
                &refresh_area,
                ctx->frame) != 0) {
            return -1;
        }

        if (ssd1681_write_ram_area(
                ctx,
                SSD1681_CMD_WRITE_RAM_OLD,
                &refresh_area,
                ctx->frame) != 0) {
            return -1;
        }

        if (ssd1681_start_refresh(
                ctx,
                false) != 0) {
            return -1;
        }

        ctx->base_frame_valid = true;
        ctx->partial_refresh_count = 0;
    }

    ctx->dirty_valid = false;

    return 0;
}

static int ssd1681_set_power(
    uint8_t subid,
    ezdev_display_power_t mode)
{
    ssd1681_ctx_t *ctx =
        ssd1681_get_ctx(subid);

    if (ctx == NULL ||
        !ctx->initialized) {
        return -1;
    }

    if (mode == EZDEV_DISPLAY_POWER_SLEEP) {
        const uint8_t sleep_check = 0x01U;

        if (ssd1681_wait_idle(
                SSD1681_BUSY_TIMEOUT_MS) != 0) {
            return -1;
        }

        if (ssd1681_write_command_data(
                ctx,
                SSD1681_CMD_DEEP_SLEEP,
                &sleep_check,
                1) != 0) {
            return -1;
        }

        ctx->sleeping = true;
        ctx->base_frame_valid = false;
        ctx->refresh_config =
            SSD1681_REFRESH_CONFIG_NONE;

        return 0;
    }

    if (mode == EZDEV_DISPLAY_POWER_ON) {
        if (!ctx->sleeping) {
            return 0;
        }

        return ssd1681_panel_init(ctx);
    }

    return -1;
}

static int ssd1681_is_busy(
    uint8_t subid,
    bool *busy)
{
    ssd1681_ctx_t *ctx =
        ssd1681_get_ctx(subid);

    if (ctx == NULL ||
        !ctx->initialized ||
        busy == NULL) {
        return -1;
    }

    *busy = ssd1681_busy();

    return 0;
}

/* ========================= 操作表 ========================= */

const ezdev_panel_opt_t ezdev_ssd1681_panel_opt = {
    .init = ssd1681_init,
    .deinit = NULL,
    .write_physical_area = ssd1681_write_physical_area,
    .present_physical = ssd1681_present_physical,
    .set_power = ssd1681_set_power,
    .is_busy = ssd1681_is_busy,
};

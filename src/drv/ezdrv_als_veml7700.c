#include "als/ezdrv_als_veml7700.h"
#include "ezhal/ezhal_i2c.h"
#include "ezbsp_log.h"
#include <stddef.h>

#define VEML7700_I2C_ADDR      0x10
#define VEML7700_I2C_SPEED     100000

#define REG_ALS_CONF           0x00
#define REG_ALS_PSM            0x03
#define REG_ALS_DATA           0x04
#define REG_WHITE_DATA         0x05

#define CONF_GAIN_MASK         (0x03 << 11)
#define CONF_GAIN_X1           (0x00 << 11)
#define CONF_GAIN_X2           (0x01 << 11)
#define CONF_GAIN_X1_4         (0x03 << 11)

#define CONF_IT_MASK           (0x0F << 6)
#define CONF_IT_25MS           (0x0C << 6)
#define CONF_IT_100MS          (0x00 << 6)
#define CONF_IT_400MS          (0x02 << 6)

#define CONF_ALS_SD_MASK       (0x01 << 0)
#define CONF_ALS_SD_ENABLE     (0x01 << 0)   /* shutdown */
#define CONF_ALS_SD_DISABLE    (0x00 << 0)   /* active */

#define PSM_EN_DISABLE         0x0000
#define PSM_EN_ENABLE          (0x01 << 0)
#define PSM_MODE_1             (0x00 << 1)

typedef struct {
    ezhal_i2c_dev_t i2c_dev;
    float resolution;
    uint16_t conf;
    bool ready;
} veml7700_ctx_t;

static veml7700_ctx_t s_ctx[VEML7700_DEV_MAX] = {
    [VEML7700_DEV_0] = {
        .i2c_dev = EZHAL_I2C_DEV_INVALID,
        .resolution = 0.0576f,
        .conf = CONF_GAIN_X1 | CONF_IT_100MS | CONF_ALS_SD_DISABLE,
        .ready = false,
    },
};

static const ezhal_i2c_bus_t s_bus[VEML7700_DEV_MAX] = {
    [VEML7700_DEV_0] = EZHAL_I2C_BUS_0,
};

static int veml7700_write_reg(ezhal_i2c_dev_t dev, uint8_t reg, uint16_t val)
{
    uint8_t buf[3] = {
        reg,
        (uint8_t)(val & 0xFF),
        (uint8_t)(val >> 8),
    };

    return ezhal_i2c_write(dev, buf, sizeof(buf));
}

static int veml7700_read_reg(ezhal_i2c_dev_t dev, uint8_t reg, uint16_t *val)
{
    uint8_t buf[2];

    if (val == NULL) return -1;
    if (ezhal_i2c_write_read(dev, &reg, 1, buf, 2) != 0) return -1;

    *val = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    return 0;
}

static void veml7700_set_resolution(uint8_t id, ezdev_als_gain_t gain, ezdev_als_integration_t it)
{
    float r = 0.0576f;

    if (gain == EZDEV_ALS_GAIN_LOW) {
        r *= 4.0f;       /* x1/4 */
    } else if (gain == EZDEV_ALS_GAIN_HIGH) {
        r *= 0.5f;       /* x2 */
    }

    if (it == EZDEV_ALS_INTEGRATION_FAST) {
        r *= 4.0f;       /* 25ms */
    } else if (it == EZDEV_ALS_INTEGRATION_SLOW) {
        r *= 0.25f;      /* 400ms */
    }

    s_ctx[id].resolution = r;
}

int ezdev_veml7700_init(uint8_t subid)
{
    veml7700_ctx_t *ctx;

    if (subid >= VEML7700_DEV_MAX) return -1;

    ctx = &s_ctx[subid];
    if (ctx->ready) return 0;

    if (ezhal_i2c_bus_init(s_bus[subid]) != 0) return -1;

    ctx->i2c_dev = ezhal_i2c_add_device(s_bus[subid], VEML7700_I2C_ADDR, VEML7700_I2C_SPEED);
    if (ctx->i2c_dev == EZHAL_I2C_DEV_INVALID) return -1;

    ctx->conf = CONF_GAIN_X1 | CONF_IT_100MS | CONF_ALS_SD_DISABLE;
    ctx->resolution = 0.0576f;

    if (veml7700_write_reg(ctx->i2c_dev, REG_ALS_PSM, PSM_EN_DISABLE) != 0) return -1;
    if (veml7700_write_reg(ctx->i2c_dev, REG_ALS_CONF, ctx->conf) != 0) return -1;

    ctx->ready = true;
    ezbsp_logi("VEML7700 %d init ok", subid);
    return 0;
}

int ezdev_veml7700_set_config(uint8_t subid, ezdev_als_gain_t gain, ezdev_als_integration_t it)
{
    veml7700_ctx_t *ctx;
    uint16_t conf;

    if (subid >= VEML7700_DEV_MAX || !s_ctx[subid].ready) return -1;

    ctx = &s_ctx[subid];
    conf = ctx->conf & ~(CONF_GAIN_MASK | CONF_IT_MASK);

    switch (gain) {
        case EZDEV_ALS_GAIN_LOW:    conf |= CONF_GAIN_X1_4; break;
        case EZDEV_ALS_GAIN_NORMAL: conf |= CONF_GAIN_X1;   break;
        case EZDEV_ALS_GAIN_HIGH:   conf |= CONF_GAIN_X2;   break;
        default: return -1;
    }

    switch (it) {
        case EZDEV_ALS_INTEGRATION_FAST:   conf |= CONF_IT_25MS;  break;
        case EZDEV_ALS_INTEGRATION_NORMAL: conf |= CONF_IT_100MS; break;
        case EZDEV_ALS_INTEGRATION_SLOW:   conf |= CONF_IT_400MS; break;
        default: return -1;
    }

    if (veml7700_write_reg(ctx->i2c_dev, REG_ALS_CONF, conf) != 0) return -1;

    ctx->conf = conf;
    veml7700_set_resolution(subid, gain, it);
    return 0;
}

int ezdev_veml7700_set_power_mode(uint8_t subid, ezdev_als_mode_t mode)
{
    veml7700_ctx_t *ctx;
    uint16_t conf;
    uint16_t psm = PSM_EN_DISABLE;

    if (subid >= VEML7700_DEV_MAX || !s_ctx[subid].ready) return -1;

    ctx = &s_ctx[subid];
    conf = ctx->conf;

    switch (mode) {
        case EZDEV_ALS_MODE_SHUTDOWN:
            conf = (conf & ~CONF_ALS_SD_MASK) | CONF_ALS_SD_ENABLE;
            psm = PSM_EN_DISABLE;
            break;

        case EZDEV_ALS_MODE_CONTINUOUS:
            conf = (conf & ~CONF_ALS_SD_MASK) | CONF_ALS_SD_DISABLE;
            psm = PSM_EN_DISABLE;
            break;

        case EZDEV_ALS_MODE_POWER_SAVE:
            conf = (conf & ~CONF_ALS_SD_MASK) | CONF_ALS_SD_DISABLE;
            psm = PSM_EN_ENABLE | PSM_MODE_1;
            break;

        default:
            return -1;
    }

    if (veml7700_write_reg(ctx->i2c_dev, REG_ALS_CONF, conf) != 0) return -1;
    if (veml7700_write_reg(ctx->i2c_dev, REG_ALS_PSM, psm) != 0) return -1;

    ctx->conf = conf;
    return 0;
}

int ezdev_veml7700_read_data(uint8_t subid, ezdev_als_data_t *data)
{
    veml7700_ctx_t *ctx;
    uint16_t als;
    uint16_t white;

    if (subid >= VEML7700_DEV_MAX || data == NULL || !s_ctx[subid].ready) return -1;

    ctx = &s_ctx[subid];
    if (ctx->conf & CONF_ALS_SD_ENABLE) return -1;

    if (veml7700_read_reg(ctx->i2c_dev, REG_ALS_DATA, &als) != 0) return -1;
    if (veml7700_read_reg(ctx->i2c_dev, REG_WHITE_DATA, &white) != 0) return -1;

    data->lux = (float)als * ctx->resolution;
    data->white_lux = (float)white * ctx->resolution;
    return 0;
}

const ezdev_als_opt_t ezdev_veml7700_opt = {
    .init = ezdev_veml7700_init,
    .set_config = ezdev_veml7700_set_config,
    .set_power_mode = ezdev_veml7700_set_power_mode,
    .read_data = ezdev_veml7700_read_data,
};
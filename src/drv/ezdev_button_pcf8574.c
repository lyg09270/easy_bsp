#include "button/ezdev_button_pcf8574.h"

#include <stdbool.h>
#include <stdint.h>

#include "ezhal/ezhal_i2c.h"
#include "ezhal/ezhal_exti.h"

#define PCF8574_I2C_BUS        EZHAL_I2C_BUS_0
#define PCF8574_I2C_ADDR       0x20U
#define PCF8574_I2C_SPEED      100000U

#define PCF8574_INT_PIN        0U
#define PCF8574_PIN_NUM        8U

/*
 * PCF8574 是 quasi-bidirectional IO。
 * 当作输入使用时，需要先写 1。
 */
#define PCF8574_INPUT_MASK     0xFFU

static ezhal_i2c_dev_t s_pcf8574_dev = EZHAL_I2C_DEV_INVALID;
static volatile bool s_pcf8574_irq_pending;
static bool s_pcf8574_initialized;
static uint8_t s_pcf8574_cache = 0xFFU;

static void pcf8574_exti_cb(uint32_t pin)
{
    (void)pin;
    s_pcf8574_irq_pending = true;
}

static int pcf8574_write_u8(uint8_t value)
{
    return ezhal_i2c_write(
        s_pcf8574_dev,
        &value,
        1);
}

static int pcf8574_read_u8(uint8_t *value)
{
    if (value == NULL) {
        return -1;
    }

    return ezhal_i2c_read(
        s_pcf8574_dev,
        value,
        1);
}

static int pcf8574_bus_init(void)
{
    if (s_pcf8574_initialized) {
        return 0;
    }

    if (ezhal_i2c_bus_init(PCF8574_I2C_BUS) != 0) {
        return -1;
    }

    s_pcf8574_dev = ezhal_i2c_add_device(
        PCF8574_I2C_BUS,
        PCF8574_I2C_ADDR,
        PCF8574_I2C_SPEED);

    if (s_pcf8574_dev == EZHAL_I2C_DEV_INVALID) {
        return -1;
    }

    /*
     * 全部写 1，作为输入。
     */
    if (pcf8574_write_u8(PCF8574_INPUT_MASK) != 0) {
        return -1;
    }

    if (pcf8574_read_u8(&s_pcf8574_cache) != 0) {
        return -1;
    }

    /*
     * PCF8574 INT 一般为开漏低有效。
     * 接 ESP GPIO0，内部上拉，下降沿触发。
     */
    if (ezhal_exti_init(
            PCF8574_INT_PIN,
            EZHAL_EXTI_TRIGGER_FALLING,
            EZHAL_EXTI_PULL_UP,
            pcf8574_exti_cb) != 0) {
        return -1;
    }

    if (ezhal_exti_enable(PCF8574_INT_PIN) != 0) {
        return -1;
    }

    s_pcf8574_irq_pending = false;
    s_pcf8574_initialized = true;

    return 0;
}

static int pcf8574_button_init(uint8_t subid)
{
    if (subid >= PCF8574_PIN_NUM) {
        return -1;
    }

    return pcf8574_bus_init();
}

static int pcf8574_button_deinit(uint8_t subid)
{
    (void)subid;
    return 0;
}

static int pcf8574_button_read_pressed(uint8_t subid,
                                       bool *pressed)
{
    uint8_t value;

    if (pressed == NULL ||
        subid >= PCF8574_PIN_NUM) {
        return -1;
    }

    if (!s_pcf8574_initialized) {
        return -1;
    }

    /*
     * 关键：
     * 每轮 ezdev_button_poll() 会从 subid 0 开始。
     * subid==0 时强制读一次 PCF8574。
     * 这样即使 INT 没工作，按键也能读到。
     */
    if (subid == 0U || s_pcf8574_irq_pending) {
        s_pcf8574_irq_pending = false;

        if (pcf8574_read_u8(&value) != 0) {
            return -1;
        }

        s_pcf8574_cache = value;
    }

    /*
     * 按键接 GND：
     * 松开 = 1
     * 按下 = 0
     */
    *pressed =
        ((s_pcf8574_cache & (uint8_t)(1U << subid)) == 0U);

    return 0;
}

const ezdev_button_opt_t ezdev_button_pcf8574_opt = {
    .init = pcf8574_button_init,
    .deinit = pcf8574_button_deinit,
    .read_pressed = pcf8574_button_read_pressed,
};
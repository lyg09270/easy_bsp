#include "ezhal/ezhal_i2c.h"

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "EZHAL_I2C";

#define EZHAL_I2C_DEV_MAX       16
#define EZHAL_I2C_TIMEOUT_MS    50

/**
 * @brief I2C 物理总线结构体
 */
typedef struct {
    int port_num;
    int scl_io_num;
    int sda_io_num;
    i2c_master_bus_handle_t bus_handle;
    bool is_initialized;
} ezhal_esp_i2c_bus_t;

/**
 * @brief I2C 逻辑从设备结构体
 */
typedef struct {
    i2c_master_dev_handle_t esp_dev_handle;
    bool is_allocated;
} ezhal_esp_i2c_dev_t;

/*
 * 总线静态对象池
 *
 * 注意：
 * 这里保留你原来的引脚。
 * BUS_0: SCL=4, SDA=5
 * BUS_1: SCL=19, SDA=18
 */
static ezhal_esp_i2c_bus_t bus_pool[EZHAL_I2C_BUS_MAX] = {
    [EZHAL_I2C_BUS_0] = {
        .port_num = 0,
        .scl_io_num = 4,
        .sda_io_num = 5,
        .bus_handle = NULL,
        .is_initialized = false,
    },
    [EZHAL_I2C_BUS_1] = {
        .port_num = 1,
        .scl_io_num = 19,
        .sda_io_num = 18,
        .bus_handle = NULL,
        .is_initialized = false,
    },
};

/*
 * 从设备动态对象池
 */
static ezhal_esp_i2c_dev_t dev_pool[EZHAL_I2C_DEV_MAX];

static bool ezhal_i2c_dev_is_valid(ezhal_i2c_dev_t dev)
{
    if (dev >= EZHAL_I2C_DEV_MAX) {
        return false;
    }

    if (!dev_pool[dev].is_allocated) {
        return false;
    }

    if (dev_pool[dev].esp_dev_handle == NULL) {
        return false;
    }

    return true;
}

int ezhal_i2c_bus_init(ezhal_i2c_bus_t bus)
{
    esp_err_t err;

    if (bus >= EZHAL_I2C_BUS_MAX) {
        return -1;
    }

    ezhal_esp_i2c_bus_t *obj = &bus_pool[bus];

    if (obj->is_initialized) {
        return 0;
    }

    i2c_master_bus_config_t bus_config = {
        .i2c_port = obj->port_num,
        .sda_io_num = obj->sda_io_num,
        .scl_io_num = obj->scl_io_num,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true,
        },
    };

    err = i2c_new_master_bus(&bus_config, &obj->bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG,
                 "Failed to create I2C bus %d: %s",
                 bus,
                 esp_err_to_name(err));
        return -1;
    }

    obj->is_initialized = true;

    ESP_LOGI(TAG,
             "I2C bus %d init OK, port=%d, SDA=%d, SCL=%d",
             bus,
             obj->port_num,
             obj->sda_io_num,
             obj->scl_io_num);

    return 0;
}

int ezhal_i2c_bus_deinit(ezhal_i2c_bus_t bus)
{
    esp_err_t err;

    if (bus >= EZHAL_I2C_BUS_MAX) {
        return -1;
    }

    ezhal_esp_i2c_bus_t *obj = &bus_pool[bus];

    if (!obj->is_initialized) {
        return 0;
    }

    err = i2c_del_master_bus(obj->bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG,
                 "Failed to delete I2C bus %d: %s",
                 bus,
                 esp_err_to_name(err));
        return -1;
    }

    obj->bus_handle = NULL;
    obj->is_initialized = false;

    return 0;
}

ezhal_i2c_dev_t ezhal_i2c_add_device(ezhal_i2c_bus_t bus,
                                     uint16_t dev_addr,
                                     uint32_t speed)
{
    uint8_t dev_id = EZHAL_I2C_DEV_INVALID;
    esp_err_t err;

    if (bus >= EZHAL_I2C_BUS_MAX) {
        return EZHAL_I2C_DEV_INVALID;
    }

    ezhal_esp_i2c_bus_t *bus_obj = &bus_pool[bus];

    if (!bus_obj->is_initialized || bus_obj->bus_handle == NULL) {
        ESP_LOGE(TAG, "Bus %d not initialized", bus);
        return EZHAL_I2C_DEV_INVALID;
    }

    for (int i = 0; i < EZHAL_I2C_DEV_MAX; i++) {
        if (!dev_pool[i].is_allocated) {
            dev_id = (uint8_t)i;
            break;
        }
    }

    if (dev_id == EZHAL_I2C_DEV_INVALID) {
        ESP_LOGE(TAG, "Device pool full, cannot add device 0x%02X", dev_addr);
        return EZHAL_I2C_DEV_INVALID;
    }

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = dev_addr,
        .scl_speed_hz = speed,
    };

    i2c_master_dev_handle_t esp_handle = NULL;

    err = i2c_master_bus_add_device(bus_obj->bus_handle,
                                    &dev_config,
                                    &esp_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG,
                 "Failed to add device 0x%02X to bus %d: %s",
                 dev_addr,
                 bus,
                 esp_err_to_name(err));
        return EZHAL_I2C_DEV_INVALID;
    }

    dev_pool[dev_id].esp_dev_handle = esp_handle;
    dev_pool[dev_id].is_allocated = true;

    ESP_LOGI(TAG,
             "Add I2C device OK, bus=%d, dev=0x%02X, handle=%u, speed=%lu",
             bus,
             dev_addr,
             dev_id,
             (unsigned long)speed);

    return (ezhal_i2c_dev_t)dev_id;
}

int ezhal_i2c_remove_device(ezhal_i2c_dev_t dev)
{
    esp_err_t err;

    if (!ezhal_i2c_dev_is_valid(dev)) {
        return -1;
    }

    err = i2c_master_bus_rm_device(dev_pool[dev].esp_dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG,
                 "Remove I2C device %u failed: %s",
                 dev,
                 esp_err_to_name(err));
        return -1;
    }

    dev_pool[dev].esp_dev_handle = NULL;
    dev_pool[dev].is_allocated = false;

    return 0;
}

int ezhal_i2c_write(ezhal_i2c_dev_t dev,
                    const uint8_t *data,
                    size_t length)
{
    esp_err_t err;

    if (!ezhal_i2c_dev_is_valid(dev)) {
        return -1;
    }

    if (data == NULL || length == 0) {
        return -1;
    }

    err = i2c_master_transmit(dev_pool[dev].esp_dev_handle,
                              data,
                              length,
                              EZHAL_I2C_TIMEOUT_MS);

    if (err != ESP_OK) {
        ESP_LOGE(TAG,
                 "I2C write failed, dev=%u, len=%u, err=%s",
                 dev,
                 (unsigned int)length,
                 esp_err_to_name(err));
        return -1;
    }

    return 0;
}

int ezhal_i2c_read(ezhal_i2c_dev_t dev,
                   uint8_t *data,
                   size_t length)
{
    esp_err_t err;

    if (!ezhal_i2c_dev_is_valid(dev)) {
        return -1;
    }

    if (data == NULL || length == 0) {
        return -1;
    }

    err = i2c_master_receive(dev_pool[dev].esp_dev_handle,
                             data,
                             length,
                             EZHAL_I2C_TIMEOUT_MS);

    if (err != ESP_OK) {
        ESP_LOGE(TAG,
                 "I2C read failed, dev=%u, len=%u, err=%s",
                 dev,
                 (unsigned int)length,
                 esp_err_to_name(err));
        return -1;
    }

    return 0;
}

/**
 * @brief I2C 复合事务：先写后读，中间 repeated-start
 */
int ezhal_i2c_write_read(ezhal_i2c_dev_t dev,
                         const uint8_t *write_data,
                         size_t write_length,
                         uint8_t *read_data,
                         size_t read_length)
{
    esp_err_t err;

    if (!ezhal_i2c_dev_is_valid(dev)) {
        return -1;
    }

    if (write_data == NULL || write_length == 0) {
        return -1;
    }

    if (read_data == NULL || read_length == 0) {
        return -1;
    }

    err = i2c_master_transmit_receive(dev_pool[dev].esp_dev_handle,
                                      write_data,
                                      write_length,
                                      read_data,
                                      read_length,
                                      EZHAL_I2C_TIMEOUT_MS);

    if (err != ESP_OK) {
        ESP_LOGE(TAG,
                 "I2C write_read failed, dev=%u, wlen=%u, rlen=%u, err=%s",
                 dev,
                 (unsigned int)write_length,
                 (unsigned int)read_length,
                 esp_err_to_name(err));
        return -1;
    }

    return 0;
}

int ezhal_i2c_probe(ezhal_i2c_bus_t bus,
                    uint16_t dev_addr)
{
    esp_err_t err;

    if (bus >= EZHAL_I2C_BUS_MAX) {
        return -1;
    }

    ezhal_esp_i2c_bus_t *bus_obj = &bus_pool[bus];

    if (!bus_obj->is_initialized || bus_obj->bus_handle == NULL) {
        ESP_LOGE(TAG, "Bus %d not initialized, cannot probe", bus);
        return -1;
    }

    err = i2c_master_probe(bus_obj->bus_handle,
                           dev_addr,
                           EZHAL_I2C_TIMEOUT_MS);

    if (err == ESP_OK) {
        return 0;
    }

    if (err == ESP_ERR_NOT_FOUND) {
        return -1;
    }

    ESP_LOGE(TAG,
             "I2C probe failed, bus=%d, addr=0x%02X, err=%s",
             bus,
             dev_addr,
             esp_err_to_name(err));

    return -1;
}
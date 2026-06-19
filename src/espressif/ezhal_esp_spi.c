#include "ezhal/ezhal_spi.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "EZHAL_SPI";

#define EZHAL_SPI_DEV_MAX 8

typedef struct {
    spi_host_device_t host_id;
    bool is_initialized;
    ezhal_spi_mode_t mode;
    uint32_t bitrate;
} ezhal_esp_spi_bus_t;

typedef struct {
    spi_device_handle_t esp_dev_handle;
    bool is_allocated;
} ezhal_esp_spi_dev_t;

static ezhal_esp_spi_bus_t bus_pool[EZHAL_SPI_BUS_MAX] = {
    [EZHAL_SPI_BUS_0] = { .host_id = SPI2_HOST, .is_initialized = false },
};

static ezhal_esp_spi_dev_t dev_pool[EZHAL_SPI_DEV_MAX];

static bool ezhal_spi_dev_is_valid(ezhal_spi_dev_t dev) {
    return (dev < EZHAL_SPI_DEV_MAX && dev_pool[dev].is_allocated && dev_pool[dev].esp_dev_handle != NULL);
}

int ezhal_spi_bus_init(ezhal_spi_bus_t bus, ezhal_spi_mode_t mode, uint32_t bitrate) {
    if (bus >= EZHAL_SPI_BUS_MAX || bus_pool[bus].is_initialized) return -1;

    spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = 7,
        .sclk_io_num = 6,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    if (spi_bus_initialize(bus_pool[bus].host_id, &buscfg, SPI_DMA_CH_AUTO) != ESP_OK) return -1;

    bus_pool[bus].is_initialized = true;
    bus_pool[bus].mode = mode;
    bus_pool[bus].bitrate = bitrate;
    return 0;
}

ezhal_spi_dev_t ezhal_spi_add_device(ezhal_spi_bus_t bus, uint32_t cs_pin) {
    if (bus >= EZHAL_SPI_BUS_MAX || !bus_pool[bus].is_initialized) return EZHAL_SPI_DEV_INVALID;

    int dev_id = -1;
    for (int i = 0; i < EZHAL_SPI_DEV_MAX; i++) {
        if (!dev_pool[i].is_allocated) { dev_id = i; break; }
    }
    if (dev_id == -1) return EZHAL_SPI_DEV_INVALID;

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = bus_pool[bus].bitrate,
        .mode = bus_pool[bus].mode,
        .spics_io_num = cs_pin,
        .queue_size = 7,
    };

    if (spi_bus_add_device(bus_pool[bus].host_id, &devcfg, &dev_pool[dev_id].esp_dev_handle) != ESP_OK) {
        return EZHAL_SPI_DEV_INVALID;
    }

    dev_pool[dev_id].is_allocated = true;
    return (ezhal_spi_dev_t)dev_id;
}

int ezhal_spi_transfer(ezhal_spi_dev_t dev, const uint8_t *tx_data, uint8_t *rx_data, size_t length) {
    if (!ezhal_spi_dev_is_valid(dev)) return -1;

    spi_transaction_t t = {
        .length = length * 8,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };

    return (spi_device_transmit(dev_pool[dev].esp_dev_handle, &t) == ESP_OK) ? 0 : -1;
}

int ezhal_spi_bus_deinit(ezhal_spi_bus_t bus) {
    if (bus >= EZHAL_SPI_BUS_MAX || !bus_pool[bus].is_initialized) return -1;
    spi_bus_free(bus_pool[bus].host_id);
    bus_pool[bus].is_initialized = false;
    return 0;
}
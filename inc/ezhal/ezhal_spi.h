#ifndef EZHAL_SPI_H
#define EZHAL_SPI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EZHAL_SPI_DEV_INVALID ((ezhal_spi_dev_t)0xFF)

typedef enum {
    EZHAL_SPI_BUS_0 = 0,
    EZHAL_SPI_BUS_MAX
} ezhal_spi_bus_t;

/**
 * @brief SPI Clock configuration modes
 */
typedef enum {
    EZHAL_SPI_MODE_0 = 0, // CPOL=0, CPHA=0
    EZHAL_SPI_MODE_1,     // CPOL=0, CPHA=1
    EZHAL_SPI_MODE_2,     // CPOL=1, CPHA=0
    EZHAL_SPI_MODE_3      // CPOL=1, CPHA=1
} ezhal_spi_mode_t;

typedef uint8_t ezhal_spi_dev_t;

/**
 * @brief Initialize the SPI bus.
 * @param[in] bus SPI bus instance.
 * @param[in] mode Clock polarity and phase.
 * @param[in] bitrate Clock frequency in Hz.
 */
int ezhal_spi_bus_init(ezhal_spi_bus_t bus, ezhal_spi_mode_t mode, uint32_t bitrate);

int ezhal_spi_bus_deinit(ezhal_spi_bus_t bus);

/**
 * @brief Register a device on the SPI bus.
 * @param[in] bus The bus to attach to.
 * @param[in] cs_pin GPIO pin identifier for Chip Select.
 * @return Handle for the SPI device.
 */
ezhal_spi_dev_t ezhal_spi_add_device(ezhal_spi_bus_t bus, uint32_t cs_pin);

int ezhal_spi_remove_device(ezhal_spi_dev_t dev);

/**
 * @brief Full-duplex transfer.
 * @param[in] dev Device handle.
 * @param[in] tx_data Buffer to send.
 * @param[out] rx_data Buffer to receive (can be NULL if not required).
 * @param[in] length Number of bytes to exchange.
 */
int ezhal_spi_transfer(ezhal_spi_dev_t dev,
                       const uint8_t *tx_data,
                       uint8_t *rx_data,
                       size_t length);

#ifdef __cplusplus
}
#endif

#endif // EZHAL_SPI_H
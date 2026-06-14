#ifndef EZHAL_I2C_H
#define EZHAL_I2C_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EZHAL_I2C_DEV_INVALID  ((ezhal_i2c_dev_t)0xFF)

/**
 * @brief I2C Hardware Bus Instance Enumeration.
 * @note Managed internally in .c via object pool.
 */
typedef enum {
    EZHAL_I2C_BUS_0 = 0, /**< Hardware I2C Bus 0 */
    EZHAL_I2C_BUS_1,     /**< Hardware I2C Bus 1 */
    EZHAL_I2C_BUS_MAX    /**< Total number of supported buses */
} ezhal_i2c_bus_t;

/**
 * @brief Opaque handle representing an I2C Slave Device.
 */
typedef uint8_t ezhal_i2c_dev_t;

/**
 * @brief Initialize the specified I2C master bus.
 * @note This only initializes the bus itself, not individual devices.
 * @return int 0 on success, -1 on failure.
 */
int ezhal_i2c_bus_init(ezhal_i2c_bus_t bus);

/**
 * @brief De-initialize the specified I2C master bus.
 * @return int 0 on success, -1 on failure.
 */
int ezhal_i2c_bus_deinit(ezhal_i2c_bus_t bus);

/**
 * @brief Register a specific slave device onto an initialized I2C bus.
 * @param[in] bus        The I2C bus where the device is attached.
 * @param[in] dev_addr   7-bit target slave device address.
 * @param[in] speed      I2C SCL line frequency.
 * @return ezhal_i2c_dev_t Valid device handle on success, EZHAL_I2C_DEV_INVALID on failure.
 */
ezhal_i2c_dev_t ezhal_i2c_add_device(ezhal_i2c_bus_t bus,
                                     uint16_t dev_addr,
                                     uint32_t speed);

/**
 * @brief Unregister and remove a slave device from its bus.
 * @return int 0 on success, -1 on failure.
 */
int ezhal_i2c_remove_device(ezhal_i2c_dev_t dev);

/**
 * @brief Write data to a specific registered device.
 * @param[in] dev       The device handle obtained from ezhal_i2c_add_device.
 * @param[in] data      Pointer to the byte stream to be transmitted.
 * @param[in] length    Number of bytes to write.
 * @return int 0 on success, -1 on failure.
 */
int ezhal_i2c_write(ezhal_i2c_dev_t dev,
                    const uint8_t *data,
                    size_t length);

/**
 * @brief Read data from a specific registered device.
 * @param[in] dev       The device handle obtained from ezhal_i2c_add_device.
 * @param[out] data     Pointer to the buffer where received data will be stored.
 * @param[in] length    Number of bytes to read.
 * @return int 0 on success, -1 on failure.
 */
int ezhal_i2c_read(ezhal_i2c_dev_t dev,
                   uint8_t *data,
                   size_t length);

/**
 * @brief Write then read in one I2C transaction.
 *
 * @note This is used for register-style I2C devices.
 *
 * Typical waveform:
 * START + ADDR(W) + write_data
 * REPEATED START + ADDR(R) + read_data
 * STOP
 *
 * @param[in] dev           The device handle obtained from ezhal_i2c_add_device.
 * @param[in] write_data    Data to transmit before reading, usually register address.
 * @param[in] write_length  Number of bytes to write.
 * @param[out] read_data    Buffer to receive data.
 * @param[in] read_length   Number of bytes to read.
 * @return int 0 on success, -1 on failure.
 */
int ezhal_i2c_write_read(ezhal_i2c_dev_t dev,
                         const uint8_t *write_data,
                         size_t write_length,
                         uint8_t *read_data,
                         size_t read_length);

/**
 * @brief Probe an I2C address directly on a specific bus without adding a device.
 * @param[in] bus       The I2C bus to scan.
 * @param[in] dev_addr  The 7-bit slave address to probe.
 * @return int 0 if the device exists, -1 on failure or NACK.
 */
int ezhal_i2c_probe(ezhal_i2c_bus_t bus,
                    uint16_t dev_addr);

#ifdef __cplusplus
}
#endif

#endif // EZHAL_I2C_H
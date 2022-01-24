#pragma once

// ====================================================================
// INCLUDES
// ====================================================================

#include "common.h"

#include "driver/i2c.h"

// ====================================================================
// DEFINES
// ====================================================================

#define ACK_CHECK_ENABLE          0x1     /**< \a Leader will check acknowledgement from \a follower */
#define ACK_CHECK_DISABLE         0x0     /**< \a Leader will not check acknowledgement from \a follower */

#define I2C_CLOCK_SPEED_STANDARD  100000     /**< 100kHz Clock speed */
#define I2C_CLOCK_SPEED_FAST      400000     /**< 400kHz Clock speed */

#define I2C_DEFAULT_TICKS         (1000 / portTICK_RATE_MS)     /**< Default ticks to wait for I2C operation (1 second) */


// ====================================================================
// TYPEDEFS
// ====================================================================

// I2C_NUM_0
// I2C_NUM_1


// ====================================================================
// GLOBAL FUNCTIONS
// ====================================================================

///
/// @brief Initialize the I2C driver for one of the ports as the \a leader
///
/// @param i2c_port The I2C port
/// @param clk_speed The clock speed (Hz)
/// @param sda_pin GPIO pin number for the data line
/// @param scl_pin GPIO pin number for the clock line
///
/// @return
///         - TRUE  The I2C driver successfully initialized for the given port
///         - FALSE The I2C driver failed to initialized, or it is already initialized for the port
bool i2c_init(int i2c_port, int clk_speed, int sda_pin, int scl_pin);

///
/// @brief Send bytes to the \a follower
///
/// @param i2c_port The I2C port
/// @param address The 7-bit follower address to send bytes to
/// @param data Pointer to the bytes to send
/// @param data_len The number of bytes to send
/// @param ticks_to_wait Ticks to wait before timing out. This refers to the I2C bus.
///
/// @return
///         - TRUE  The operation completed successfully
///         - FALSE The operation failed
bool i2c_write(int i2c_port, uint8_t address, uint8_t* data, int data_len, TickType_t ticks_to_wait);

///
/// @brief Read bytes from from the \a follower
///
/// @param i2c_port The I2C port
/// @param address The 7-bit follower address to read bytes from
/// @param data Pointer to where the bytes will be read into
/// @param data_len The number of bytes to read1
/// @param ticks_to_wait Ticks to wait before timing out. This refers to the I2C bus.
///
/// @return
///         - TRUE  The operation completed successfully
///         - FALSE The operation failed
bool i2c_read(int i2c_port, uint8_t address, uint8_t* data, int data_len, TickType_t ticks_to_wait);

///
/// @brief Perform an I2C operation with a command handle
///
/// @note The command handle will be deleted
///
/// @param i2c_port The I2C port
/// @param cmd The I2C command handle
/// @param ticks_to_wait Ticks to wait before timing out. This refers to the I2C bus.
///
/// @return
///         - TRUE  The operation completed successfully
///         - FALSE The operation failed
bool i2c_cmd(int i2c_port, i2c_cmd_handle_t cmd, TickType_t ticks_to_wait);

///
/// @brief Attempt to ping the \a follower
///
/// @param i2c_port The I2C port
/// @param address The 7-bit follower address to ping
/// @param ticks_to_wait Ticks to wait before timing out. This refers to the I2C bus.
///
/// @return
///         - TRUE  The operation completed successfully
///         - FALSE The operation failed
bool i2c_ping(int i2c_port, uint8_t address, TickType_t ticks_to_wait);

// ====================================================================
// INCLUDES
// ====================================================================

#include "common.h"
#include "i2c_support.h"

#include "driver/i2c.h"


// ====================================================================
// DEFINES
// ====================================================================

// ====================================================================
// MACROS
// ====================================================================

#define I2C_PORT_INDEX(p) (p == I2C_NUM_0) ? 0 : 1

// ====================================================================
// STATIC VARIABLES
// ====================================================================

static SemaphoreHandle_t i2c_mux[I2C_NUM_MAX] = {NULL};
static bool i2c_initialized[I2C_NUM_MAX] = {false};

// ====================================================================
// GLOBAL FUNCTIONS
// ====================================================================

bool i2c_init(int i2c_port, int clk_speed, int sda_pin, int scl_pin)
{
    if(i2c_initialized[I2C_PORT_INDEX(i2c_port)])
        return false;

    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = sda_pin;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = scl_pin;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
#if IDF_VERSION_MAJOR_MINOR >= 43
    conf.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;
#endif
    conf.master.clk_speed = clk_speed;

    esp_err_t err;
    err = i2c_param_config(i2c_port, (const i2c_config_t*)&conf);
    CHECK_ESP_ERR_NO_RET("Failed to config i2c");
    if(err != ESP_OK) return false;

    err = i2c_driver_install(i2c_port, I2C_MODE_MASTER, 0, 0, 0);
    CHECK_ESP_ERR_NO_RET("Failed to install driver");
    if(err != ESP_OK) return false;

    i2c_mux[I2C_PORT_INDEX(i2c_port)] = xSemaphoreCreateMutex();
    i2c_initialized[I2C_PORT_INDEX(i2c_port)] = true;
    return true;
}

bool i2c_write(int i2c_port, uint8_t address, uint8_t* data, int data_len, TickType_t ticks_to_wait)
{
    if(!i2c_initialized[I2C_PORT_INDEX(i2c_port)])
        return false;

    xSemaphoreTake(i2c_mux[I2C_PORT_INDEX(i2c_port)], portMAX_DELAY);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address<<1) | I2C_MASTER_WRITE, ACK_CHECK_ENABLE);

    if(data_len == 1) {
        i2c_master_write_byte(cmd, data[0], ACK_CHECK_ENABLE);
    } else if(data_len > 1) {
        i2c_master_write(cmd, data, data_len, ACK_CHECK_ENABLE);
    }

    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(i2c_port, cmd, ticks_to_wait);
    CHECK_ESP_ERR_NO_RET("Write error");

    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(i2c_mux[I2C_PORT_INDEX(i2c_port)]);
    return (err == ESP_OK);
}

bool i2c_read(int i2c_port, uint8_t address, uint8_t* data, int data_len, TickType_t ticks_to_wait)
{
    if(!i2c_initialized[I2C_PORT_INDEX(i2c_port)])
        return false;

    xSemaphoreTake(i2c_mux[I2C_PORT_INDEX(i2c_port)], portMAX_DELAY);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address<<1) | I2C_MASTER_READ, ACK_CHECK_ENABLE);

    if (data_len > 1)
        i2c_master_read(cmd, data, data_len - 1, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, data + data_len - 1, I2C_MASTER_NACK); //@TODO: look into ACK vs NACK here

    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(i2c_port, cmd, ticks_to_wait);
    CHECK_ESP_ERR_NO_RET("Read error");

    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(i2c_mux[I2C_PORT_INDEX(i2c_port)]);
    return (err == ESP_OK);
}

bool i2c_cmd(int i2c_port, i2c_cmd_handle_t cmd, TickType_t ticks_to_wait)
{
    if(!i2c_initialized[I2C_PORT_INDEX(i2c_port)])
        return false;

    xSemaphoreTake(i2c_mux[I2C_PORT_INDEX(i2c_port)], portMAX_DELAY);

    esp_err_t err = i2c_master_cmd_begin(i2c_port, cmd, ticks_to_wait);
    CHECK_ESP_ERR_NO_RET("Cmd error");

    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(i2c_mux[I2C_PORT_INDEX(i2c_port)]);
    return (err == ESP_OK);
}

bool i2c_ping(int i2c_port, uint8_t address, TickType_t ticks_to_wait)
{
    if(!i2c_initialized[I2C_PORT_INDEX(i2c_port)])
        return false;

    xSemaphoreTake(i2c_mux[I2C_PORT_INDEX(i2c_port)], portMAX_DELAY);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address<<1) | I2C_MASTER_READ, ACK_CHECK_ENABLE);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(i2c_port, cmd, ticks_to_wait);
    CHECK_ESP_ERR_NO_RET("Ping error");

    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(i2c_mux[I2C_PORT_INDEX(i2c_port)]);
    return (err == ESP_OK);
}

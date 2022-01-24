#pragma once 

// ====================================================================
// Includes
// ====================================================================

#include "common.h"


// ====================================================================
// Defines
// ====================================================================

#define ADS_ADR             0x48    //1001000 (ADDR connected to GND)


#define ADS_I2C_PORT        I2C_NUM_0
#define ADS_I2C_CLK         400000
#define ADS_I2C_WAIT_TICKS  100

// ====================================================================
// Macros
// ====================================================================


// ====================================================================
// Data Types
// ====================================================================

// register address
typedef enum
{
  ADS_CONVERSION_REG_ADDR = 0,
  ADS_CONFIG_REG_ADDR,
  ADS_LO_THRESH_REG_ADDR,
  ADS_HI_THRESH_REG_ADDR,
  ADS_MAX_REG_ADDR
} ads_reg_addr_t;

// multiplex options
typedef enum
{
  ADS_MUX_0_1 = 0,
  ADS_MUX_0_3,
  ADS_MUX_1_3,
  ADS_MUX_2_3,
  ADS_MUX_0_GND,
  ADS_MUX_1_GND,
  ADS_MUX_2_GND,
  ADS_MUX_3_GND,
} AdsMux;

typedef enum
{
    ADS_CH_A0 = 0,
    ADS_CH_A1,
    ADS_CH_A2,
    ADS_CH_A3,
    ADS_CH_MAX
} AdsCh;

// full-scale resolution options
typedef enum
{
  ADS_FSR_6_144 = 0,
  ADS_FSR_4_096,
  ADS_FSR_2_048,
  ADS_FSR_1_024,
  ADS_FSR_0_512,
  ADS_FSR_0_256,
} ads_fsr_t;

// samples per second
typedef enum
{
  ADS_SPS_8 = 0,
  ADS_SPS_16,
  ADS_SPS_32,
  ADS_SPS_64,
  ADS_SPS_128,
  ADS_SPS_250,
  ADS_SPS_475,
  ADS_SPS_860
} ads_sps_t;

// modes
typedef enum
{
  ADS_MODE_CONTINUOUS = 0,
  ADS_MODE_SINGLE
} ads_mode_t;


// config
typedef union
{
  struct {
    uint16_t COMP_QUE:2;  // bits 0..  1  Comparator queue and disable
    uint16_t COMP_LAT:1;  // bit  2       Latching Comparator
    uint16_t COMP_POL:1;  // bit  3       Comparator Polarity
    uint16_t COMP_MODE:1; // bit  4       Comparator Mode
    uint16_t DR:3;        // bits 5..  7  Data rate
    uint16_t MODE:1;      // bit  8       Device operating mode
    uint16_t PGA:3;       // bits 9..  11 Programmable gain amplifier configuration
    uint16_t MUX:3;       // bits 12.. 14 Input multiplexer configuration
    uint16_t OS:1;        // bit  15      Operational status or single-shot conversion start
  } bit;
  uint16_t reg;
} ads_config_t;


// ====================================================================
// Global Functions
// ====================================================================

bool ads_set_ch(AdsCh channel);
bool ads_get_raw(AdsCh channel, int16_t* raw);
bool ads_get_voltage(AdsCh channel, float* voltage);
bool ads_init();

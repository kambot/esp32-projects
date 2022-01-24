// ====================================================================
// INCLUDES
// ====================================================================

#include "common.h"
#include "adc.h"

#include "esp_efuse.h"
#include "esp_adc_cal.h"
#include "driver/adc.h"


// ====================================================================
// DEFINES
// ====================================================================


// ====================================================================
// MACROS
// ====================================================================


// ====================================================================
// STATIC VARIABLES
// ====================================================================


static adc_atten_t atten = ADC_ATTEN_DB_2_5;
// static adc_atten_t atten = ADC_ATTEN_DB_11; //  gives full-scale voltage 3.9V
static adc_unit_t unit = ADC_UNIT_1;
static esp_adc_cal_characteristics_t *adc_chars;


// ====================================================================
// GLOBAL VARIABLES
// ====================================================================

// ====================================================================
// STATIC PROTOTYPES
// ====================================================================



// ====================================================================
// GLOBAL FUNCTIONS
// ====================================================================

void adc_init()
{

    // // $IDF_PATH/components/esptool_py/esptool/espefuse.py --port /dev/ttyUSB0 summary
    // // $IDF_PATH/components/esptool_py/esptool/espefuse.py --port /dev/ttyUSB0 adc_info

    esp_efuse_desc_t desc = {0};
    desc.efuse_block = EFUSE_BLK0;
    desc.bit_start = 136;
    desc.bit_count = 6;
    const esp_efuse_desc_t* test[] = {
        &desc,
        NULL
    };
    uint8_t b = 0;
    esp_efuse_read_field_blob(test, &b, 6);

    int sign = 1;
    int s = (b >> 5) & 0x01;
    if(s) sign = -1;
    int vref_mv = ((b & 0x1F)*sign)*7+1100;
    LOGI("vref: %d", vref_mv);

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, atten); //gpio 36

    // Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, 1100, adc_chars);

    if(val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        LOGI("Characterized using Two Point Value");
    } else if(val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        LOGI("Characterized using eFuse Vref");
    } else {
        LOGI("Characterized using Default Vref");
    }

}

int adc_get_value(uint8_t num_samples)
{
    uint64_t sum = 0;
    for(int i = 0; i < num_samples; ++i)
    {
        sum += adc1_get_raw(ADC1_CHANNEL_0);
    }
    return (sum / num_samples);
}


uint32_t adc_raw_to_mv(int adc_reading)
{
    return esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
}

float adc_mv_to_temp(uint32_t mv)
{
    float temp_c = (((float)mv - 500.0f) / 10.0f);
    float temp_f = temp_c * 1.8f + 32;
    return temp_f;
}

float get_temp(uint8_t num_samples)
{
    int value = adc_get_value(num_samples);
    uint32_t mv =  adc_raw_to_mv(value);
    // printf("adc voltage: %.2f\n", mv/1000.0f);
    float temp = adc_mv_to_temp(mv);


    // printf("value: %d\n", value);
    // printf("mv: %u\n", mv);
    // printf("temp: %.2f\n", temp);

    return temp;
}

// ====================================================================
// STATIC FUNCTIONS
// ====================================================================



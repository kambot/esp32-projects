#include "common.h"
#include "ads1115.h"
#include "i2c_support.h"
#include "esp32/rom/ets_sys.h"

// ====================================================================
// Static Variables
// ====================================================================

// configuration bits
static ads_config_t ads_config = {0};

// current read/write register
static ads_reg_addr_t curr_reg = ADS_MAX_REG_ADDR;

// current configured channel to read analog input from
static AdsCh ads_channel = ADS_CH_A0;

// the last time a reading was requested from the ADS chip
static int64_t ads_last_read_time = 0;

static esp_timer_handle_t timer_handle;

// ====================================================================
// Static Functions
// ====================================================================

static bool ads_i2c_read(uint8_t* data, int len)
{
    return i2c_read(ADS_I2C_PORT, ADS_ADR, data, len, ADS_I2C_WAIT_TICKS);
}

static bool ads_i2c_write(uint8_t* data, int len)
{
    return i2c_write(ADS_I2C_PORT, ADS_ADR, data, len, ADS_I2C_WAIT_TICKS);
}



static bool ads_set_register(ads_reg_addr_t reg)
{
    if(curr_reg == reg) return true;

    uint8_t wdata[1] = {reg};
    bool ret = ads_i2c_write(wdata, 1);

    if(ret) curr_reg = reg;
    else LOGE("Failed to set register");

    return ret;
}


static bool ads_read_register(ads_reg_addr_t reg, uint8_t* data, uint8_t len)
{
    if(!ads_set_register(reg)) return false;

    bool ret = ads_i2c_read(data, len);

    if(!ret) LOGE("Failed to read register");

    return ret;
}


static bool ads_write_register(ads_reg_addr_t reg, uint16_t data)
{
    uint8_t wdata[3] = {0};
    wdata[0] = reg;
    wdata[1] = data >> 8;
    wdata[2] = data & 0xFF;

    bool ret = ads_i2c_write(wdata, 3);

    if(ret) curr_reg = reg;
    return ret;
}

static uint16_t sps_config_to_int()
{
    uint16_t sps[] = {8,16,32,64,128,250,475,860};
    return sps[ads_config.bit.DR];
}








// static int64_t timer0 = 0;
// static int64_t timer1 = 0;
// static int timer_index = 0;
// static int timer_us = 1;
// int16_t raws[10] = {0};
// static void IRAM_ATTR ads_timer_cb(void* arg)
// {

//     timer_index++;
//     if(timer_index >= 10)
//     {
//         esp_timer_stop(timer_handle);
//         timer1 = esp_timer_get_time();
//         // printf("%lld %d\n",timer1 - timer0, timer_index);

//     }
//     else
//     {
//         // ads_get_raw(ADS_CH_A0, &(raws[timer_index]));
//         // esp_timer_start_once(timer_handle, timer_us);
//     }
// }










// ====================================================================
// Global Functions
// ====================================================================


bool ads_set_ch(AdsCh channel)
{
    if(channel == ads_channel) return true;

    ads_config.bit.MUX = ADS_MUX_0_GND + channel;
    if(!ads_write_register(ADS_CONFIG_REG_ADDR, ads_config.reg)) return false;

    ads_last_read_time = esp_timer_get_time();

    ads_channel = channel;
    LOGI("Set channel to A%d", ads_channel);
    return true;
}

bool ads_get_raw(AdsCh channel, int16_t* raw)
{
    if(!ads_set_ch(channel)) return false;

    // int64_t wait_us = 1000000.0f/sps_config_to_int()+1;//+10000.0f;
    // int64_t elapsed_us = esp_timer_get_time() - ads_last_read_time;
    // int64_t diff_us = wait_us - elapsed_us;
    // if(diff_us > 0) {
    //     // must delay some time here to let it collect a new sample on the channel
    //     ets_delay_us(diff_us);
    //     // printf("need to wait: %lld\n", diff_us);
    //     // usleep(diff_us);
    // }

    uint8_t rdata[2] = {0};
    bool ret = ads_read_register(ADS_CONVERSION_REG_ADDR, rdata, 2);
    if(!ret)
    {
        LOGE("Failed to get raw data");
        return false;
    }
    ads_last_read_time = esp_timer_get_time();
    *raw = ((uint16_t)rdata[0] << 8) | (uint16_t)rdata[1];
    return true;
}

bool ads_get_voltage(AdsCh channel, float* voltage)
{
    int16_t raw = 0;
    if(!ads_get_raw(channel, &raw)) return false;
    float fsr[] = {6.144, 4.096, 2.048, 1.024, 0.512, 0.256};
    int16_t bits = (1L<<15)-1;
    *voltage = (float)raw * fsr[ads_config.bit.PGA] / (float)bits;
    return true;
}

// // for stats
// #define TRIALS 2000
// double avgs[TRIALS] = {0};

// int64_t ts[860] = {0};

bool ads_init()
{
    bool ret;

    ret = i2c_init(ADS_I2C_PORT, ADS_I2C_CLK, 18, 19);
    if(!ret) return false;

    ads_config.bit.OS = 0;
    ads_config.bit.MUX = ADS_MUX_0_GND;
    ads_config.bit.PGA = ADS_FSR_6_144;
    ads_config.bit.MODE = ADS_MODE_CONTINUOUS;
    ads_config.bit.DR = ADS_SPS_860;
    ads_config.bit.COMP_MODE = 0;
    ads_config.bit.COMP_POL = 0;
    ads_config.bit.COMP_LAT = 0;
    ads_config.bit.COMP_QUE = 0x03;

    ads_write_register(ADS_CONFIG_REG_ADDR, ads_config.reg);
    ads_last_read_time = esp_timer_get_time();

    ads_set_ch(ADS_CH_A0);
    uint8_t _rdata[2] = {0};
    ads_read_register(ADS_CONVERSION_REG_ADDR, _rdata, 2);



    // const esp_timer_create_args_t timer_args = {
    //     .callback = &ads_timer_cb,
    //     .name = "ads_timer_cb"
    // };

    // esp_err_t err = esp_timer_create(&timer_args, &timer_handle);
    // if(err)
    // {
    //     ESP_LOGE("TAG","Failed to create timer: %X (%s)", err, esp_err_to_name(err));
    // }


    // err = esp_timer_start_periodic(timer_handle, timer_us);
    // if(err)
    // {
    //     ESP_LOGE("TAG","Failed to start timer: %X (%s)", err, esp_err_to_name(err));
    // }

    // // change channels
    // {
    //     double voltage = 0;
    //     ads_get_voltage(ADS_CH_A0, &voltage);
    //     LOGI("Channel 0 voltage: %.5f", voltage);
    //     for(int i = 0; i < 10; ++i)
    //     {
    //         ads_get_voltage(ADS_CH_A1, &voltage);
    //         LOGI("%d) Channel 1 voltage: %.5f", i, voltage);
    //     }
    // }

    // // collect 860 samples in a second
    // double voltage = 0;
    // int16_t raw = 0;
    // ads_set_ch(ADS_CH_A0);
    // LOGI("start");
    // for(int i = 0; i < 860; ++i)
    // {
    //     // ads_get_voltage(ADS_CH_A0, &voltage);
    //     ads_get_raw(ADS_CH_A0, &raw);
    //     ts[i] = ads_last_read_time;
    // }
    // LOGI("end");

    // for(int i = 1; i < 860; ++i)
    //     printf("%lld\n",ts[i]-ts[i-1]);

    // // change channels
    // for(;;)
    // {
    //     int ms = 1000;
    //     double voltage = 0;
    //     ads_get_voltage(ADS_CH_A0, &voltage);
    //     printf("A0 voltage: %.5f\n", voltage);
    //     delay(ms);
    //     ads_get_voltage(ADS_CH_A1, &voltage);
    //     printf("A1 voltage: %.5f\n", voltage);
    //     delay(ms);
    //     ads_get_voltage(ADS_CH_A2, &voltage);
    //     printf("A2 voltage: %.5f\n", voltage);
    //     delay(ms);
    //     ads_get_voltage(ADS_CH_A3, &voltage);
    //     printf("A3 voltage: %.5f\n", voltage);
    //     delay(ms);
    // }


    // // stats
    // double target = 0.0047f;
    // int samples = 15;
    // ads_set_ch(ADS_CH_A0);
    // delay(1000);

    // LOGI("Samples: %d", samples);
    // LOGI("Trials:  %d", TRIALS);

    // printf("ET: %d\n",TRIALS*samples/860);
    // for(int i = 0; i < TRIALS; ++i)
    // {
    //     if(i % 100 == 0) printf("%d\n",i);
    //     double voltage = 0;
    //     double v_avg = 0.0f;
    //     for(int j = 0; j < samples; ++j)
    //     {
    //         ads_get_voltage(ADS_CH_A0, &voltage);
    //         v_avg += voltage;
    //     }
    //     v_avg /= (double)samples;
    //     double v_dif = (v_avg/target-1.0f)*100.0f;
    //     avgs[i] = v_dif;
    // }

    // double _min = avgs[0];
    // double _max = avgs[0];
    // double _avg = 0.0f;
    // for(int i = 0; i < TRIALS; ++i)
    // {
    //     double a = avgs[i];
    //     if(a < _min) _min = a;
    //     if(a > _max) _max = a;
    //     _avg += a;
    // }
    // _avg /= (double)TRIALS;

    // printf("Average: %.4f%%\n",_avg);
    // printf("Min:     %.4f%%\n",_min);
    // printf("Max:     %.4f%%\n",_max);




    // delay(100);

    // int c = 3;
    // int16_t _raws[10] = {0};

    // int64_t _timer1 = 0;

    // uint8_t rdata[10] = {0};

    // int64_t _timer0 = esp_timer_get_time();
    // ads_read_register(ADS_CONVERSION_REG_ADDR, rdata, 3);
    // _timer1 = esp_timer_get_time();
    // printf("%lld\n", _timer1 - _timer0);

    // // esp_log_buffer_hex("TAG",rdata, 10);
    // // ads_get_raw(ADS_CH_A0, &(_raws[0]));
    // // ads_write_register(ADS_CONFIG_REG_ADDR, ads_config.reg);

    // for(int i = 0; i < 10000; ++i)
    // {
    //     ads_read_register(ADS_CONVERSION_REG_ADDR, rdata, 3);
    //     // ads_write_register(ADS_CONFIG_REG_ADDR, ads_config.reg);
    //     // ads_get_raw(ADS_CH_A0, &(_raws[0]));
    //     // int64_t x = esp_timer_get_time();
    // }
    // // _timer1 = esp_timer_get_time();
    // printf("%lld\n", _timer1 - _timer0);
    // printf("%lld\n", (_timer1 - _timer0)/c);








    return true;

}
// ====================================================================
// INCLUDES
// ====================================================================

#include "common.h"
#include "storage.h"
#include "datetime.h"
#include "wifi.h"
#include "https.h"
#include "adc.h"


// ====================================================================
// VARIABLES
// ====================================================================

esp_timer_handle_t timer_handle;
esp_timer_handle_t sensor_timer_handle;
task_data_t publish_task_cfg = {0};
task_data_t weather_task_cfg = {0};
uint8_t mac_address[6] = {0};
uint8_t bt_mac_address[6] = {0};

#define DATA_QUEUE_MAX 20
data_t data_queue[DATA_QUEUE_MAX] = {0};
int data_queue_count = 0;
bool data_queue_locked = false;
uptime_t sensor_last_uptime = 0;

#define TEMP_READINGS_MAX 50
// float temp_readings[TEMP_READINGS_MAX] = {0};
// int temp_readings_index = 0;
// int temp_readings_count = 0;

float temp_avg = 0.0f;


#define TEMP_AVGS_MAX 10
// float temp_avgs[TEMP_AVGS_MAX] = {0};
// int temp_avgs_index = 0;
// int temp_avgs_count = 0;

circ_buf_t temp_readings = {};
circ_buf_t temp_avgs = {};

// ====================================================================
// MACROS
// ====================================================================

// #define TEMP_READINGS_INSERT(value) circ_buf_insert(value, &temp_readings, &temp_readings_index, &temp_readings_count, TEMP_READINGS_MAX)

// #define TEMP_AVGS_INSERT(value)     circ_buf_insert(value, &temp_avgs, &temp_avgs_index, &temp_avgs_count, TEMP_READINGS_MAX)

// ====================================================================
// STATIC FUNCTION DECLARATIONS
// ====================================================================

char* get_running_partition();
const char* last_reset_reason_str();
void print_task_list();

void init();
void publish_task(void* arg);
void weather_task(void* arg);
void handle_sensor_temp_collection();
void IRAM_ATTR timer_cb(void* arg);
void IRAM_ATTR sensor_timer_cb(void* arg);



// ====================================================================
// MAIN
// ====================================================================

void app_main()
{
    init();
    print_task_list();

    vTaskDelete(NULL);
}

void publish_task(void* arg)
{
    LOGI("Entered publish task");
    bool ret;

    for(;;)
    {

        if(wifi_get_internet_status() && data_queue_count > 0)
        {
            LOGI("before publish data_queue_count = %d", data_queue_count);
            data_t item = {0};
            ret = data_get_from_queue(&item);

            if(ret)
            {
                ret = publish_data(item);
                if(!ret)
                {
                    data_add_to_queue(item);
                }
                LOGI("after publish data_queue_count = %d", data_queue_count);
            }
            // LOG_TASK_HWM();
        }

        delay(1000);
    }

    LOGI("Leaving publish task");
    vTaskDelete(NULL);
}


void weather_task(void* arg)
{
    LOGI("Entered weather task");
    bool ret;

    uptime_t weather_last_uptime = 0;
    uptime_t uptime;
    bool first_check = false;

    for(;;)
    {

        if(!wifi_get_internet_status() || time_since_last_update() == -1)
        {
            delay(1000);
            continue;
        }

        uptime = get_uptime();
        bool check_weather = (uptime - weather_last_uptime) >= OUT_TEMP_FREQ;

        if(!first_check)
        {
            LOGI("First weather check");
            check_weather = true;
        }

        if(check_weather)
        {
            float outside_temp = 0.0f;
            ret = get_out_temp(&outside_temp);
            // LOG_TASK_HWM();

            if(ret)
            {
                first_check = true;
                LOGI("Adding data to queue (outside temp: %.2f)", outside_temp);
                data_t data = {0};
                data.timestamp = TIME_NOW();
                data.data_type = DATA_TYPE_OUT_TEMP;
                data.value_num = outside_temp;
                data.uptime = uptime;
                data_add_to_queue(data);
                weather_last_uptime = uptime;
            }
            delay(5000);
        }

        delay(1000);
    }

    LOGI("Leaving weather task");
    vTaskDelete(NULL);
}

// void handle_sensor_temp_collection()
// {
//     static bool first_collection = false;
//     static int counter = 0;
//     // static float temp_prior = -1.0f;

//     time_t now = TIME_NOW();
//     uptime_t uptime = get_uptime();
//     float temp = get_temp(NUM_SAMPLES);
//     // if(temp_prior <= 0) temp_prior = temp;

//     float percent_change = ((float)temp - (float)temp_avg) / (float)temp_avg * 100.0f;
//     // if(ABS(percent_change) >= 7)
//     // {
//     //     LOGW("Large change (%.2f, avg: %.2f)", temp, temp_avg);
//     //     // for(int i = 0; i < TEMP_READINGS_MAX; ++i)
//     //     // {
//     //     //     int idx = (temp_readings_index+i)%TEMP_READINGS_MAX;
//     //     //     printf("%d %d: %.2f\n", i, idx, temp_readings[idx]);
//     //     // }
//     //     // // discard the point
//     //     // return;
//     // }

//     circ_buf_insert(temp, &temp_readings);




//     // temp_readings[temp_readings_index++] = temp;
//     // if(temp_readings_index >= TEMP_READINGS_MAX)
//     //     temp_readings_index = 0;

//     circ_buf_calc_avg(&temp_avg, &temp_readings);
//     // temp_avg = calc_temp_avg();


//     // if(counter == 10)
//     {
//         printf("Avg: %2.2f, Cur: %2.2f, Change: %c%.2f%%  ", temp_avg, temp, percent_change < 0 ? '-' : '+', ABS(percent_change));
//         for(int i = 0; i < TEMP_READINGS_MAX; ++i)
//         {
//             float value = 0.0f;
//             circ_buf_get_value(i, &value, &temp_readings);
//             printf("%5.2f", value);
//             if(i != (TEMP_READINGS_MAX-1)) printf(", ");
//         }
//         printf("\n");
//         counter = 0;
//     }

//     // float temp_avg_prior = temp_avg;
//     // float prior_temp = temp_readings[(temp_readings_index+8)%TEMP_READINGS_MAX];
//     // float avg_percent_change = ((float)temp - (float)prior_temp) / (float)prior_temp * 100.0f;
//     // // if(ABS(avg_percent_change) >= 2)
//     // {
//     //     printf("Temp Change: %c%.2f%%\n", avg_percent_change < 0 ? '-' : '+', ABS(avg_percent_change));
//     // }


//     bool add_data = false;

//     if(now != 0)
//     {
//         if(!first_collection) add_data = true;
//         if((uptime - sensor_last_uptime) >= TEMP_SENSOR_FREQ) add_data = true;
//     }

//     if(add_data)
//     {
//         first_collection = true;
//         LOGI("Adding data to queue (sensor temp: %.2f)", temp_avg);
//         data_t data = {0};
//         data.timestamp = now;
//         data.data_type = DATA_TYPE_IN_TEMP;
//         data.value_num = temp_avg;
//         data.uptime = uptime;
//         data_add_to_queue(data);
//         sensor_last_uptime = uptime;
//     }

//     counter++;
// }



void handle_sensor_temp_collection()
{
    static bool first_collection = false;
    static int counter = 0;
    static int avg_counter = 0;
    // static float temp_prior = -1.0f;


    float temp = get_temp(NUM_SAMPLES);
    // if(temp_prior <= 0) temp_prior = temp;

    float percent_change = ((float)temp - (float)temp_avg) / (float)temp_avg * 100.0f;
    // if(ABS(percent_change) >= 7)
    // {
    //     LOGW("Large change (%.2f, avg: %.2f)", temp, temp_avg);
    //     // for(int i = 0; i < TEMP_READINGS_MAX; ++i)
    //     // {
    //     //     int idx = (temp_readings_index+i)%TEMP_READINGS_MAX;
    //     //     printf("%d %d: %.2f\n", i, idx, temp_readings[idx]);
    //     // }
    //     // // discard the point
    //     // return;
    // }

    circ_buf_insert(temp, &temp_readings);





    // temp_readings[temp_readings_index++] = temp;
    // if(temp_readings_index >= TEMP_READINGS_MAX)
    //     temp_readings_index = 0;

    circ_buf_calc_avg(&temp_avg, &temp_readings);
    // temp_avg = calc_temp_avg();

    if(avg_counter == TEMP_READINGS_MAX)
    {
        avg_counter = 0;
        circ_buf_insert(temp_avg, &temp_avgs);

        float prior_avg = 0.0;
        if(circ_buf_get_value(1, &prior_avg, &temp_avgs))
        {
            float avg_dif = temp_avg-prior_avg;
            float avg_pc = avg_dif/prior_avg*100.0f;
            printf("Avg: %5.2f, Prior: %5.2f, Diff: %.2f (%.2f%%)\n", temp_avg, prior_avg, avg_dif, avg_pc);
        }


    }

    if(counter == 20)
    {
        // printf("Avg: %5.2f, Cur: %5.2f, Change: %c%.2f%%  ", temp_avg, temp, percent_change < 0 ? '-' : '+', ABS(percent_change));
        // for(int i = 0; i < MIN(10,TEMP_READINGS_MAX); ++i)
        // {
        //     float value = 0.0f;
        //     circ_buf_get_value(i, &value, &temp_readings);
        //     printf("%5.2f", value);
        //     if(i != (TEMP_READINGS_MAX-1)) printf(", ");
        // }
        // printf("\n");
        counter = 0;
    }

    // float temp_avg_prior = temp_avg;
    // float prior_temp = temp_readings[(temp_readings_index+8)%TEMP_READINGS_MAX];
    // float avg_percent_change = ((float)temp - (float)prior_temp) / (float)prior_temp * 100.0f;
    // // if(ABS(avg_percent_change) >= 2)
    // {
    //     printf("Temp Change: %c%.2f%%\n", avg_percent_change < 0 ? '-' : '+', ABS(avg_percent_change));
    // }


    time_t now = TIME_NOW();
    uptime_t uptime = get_uptime();
    bool add_data = false;

    if(now != 0)
    {
        if(!first_collection) add_data = true;
        if((uptime - sensor_last_uptime) >= TEMP_SENSOR_FREQ) add_data = true;
    }

    if(add_data)
    {
        first_collection = true;
        LOGI("Adding data to queue (sensor temp: %.2f)", temp_avg);
        data_t data = {0};
        data.timestamp = now;
        data.data_type = DATA_TYPE_IN_TEMP;
        data.value_num = temp_avg;
        data.uptime = uptime;
        data_add_to_queue(data);
        sensor_last_uptime = uptime;
    }

    counter++;
    avg_counter++;
}


void IRAM_ATTR timer_cb(void* arg)
{
// #if GET_SENSOR_TEMP
//     handle_sensor_temp_collection();
// #endif
}

void IRAM_ATTR sensor_timer_cb(void* arg)
{
#if GET_SENSOR_TEMP
    handle_sensor_temp_collection();
#endif
}

void init()
{
    esp_err_t err;

    esp_log_level_set("system_api", ESP_LOG_ERROR);

#if RELEASE_MODE
    esp_log_level_set("*", ESP_LOG_NONE);
#else
    // printf("Partition:         %s\n", get_running_partition());
    printf("Last Reset Reason: (%d) %s\n", esp_reset_reason(), last_reset_reason_str());
    printf("IDF Version:       %d.%d.%d (%s)\n", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH, esp_get_idf_version());
    const esp_app_desc_t* app_desc = esp_ota_get_app_description();
    printf("APP Version:       %s\n", app_desc->version);
    printf("Project Name:      %s\n", app_desc->project_name);
    printf("Compile Time:      %s\n", app_desc->time);
    printf("Compile Date:      %s\n", app_desc->date);
#endif

    esp_read_mac(mac_address, ESP_MAC_WIFI_STA);
    LOGI("Device MAC Address: " MAC_FMT, MAC_LIST(mac_address));
    // esp_read_mac(bt_mac_address, ESP_MAC_BT);
    // LOGI("Device BT MAC Address: " MAC_FMT, MAC_LIST(bt_mac_address));

    store_nvs_init();

    time_init(TIME_DEFAULT_SYNC_PERIOD);
    time_set_timezone_id(TIME_ZONE);

    wifi_init();
    wifi_set_max_reconnect_attempts(10);
    wifi_set_reconnect_time(10);
    // wifi_debug_logs(true);

    wifi_load_credentials();
    wifi_set_all("STUDIO_1837_DEVICES", "youshallnotpass", 3, WIFI_CONTEXT_PRIMARY);    // temp

    wifi_enable();
    wifi_connect(); // must always call this after setting wifi credentials

    adc_init();


    publish_task_cfg.task_func = publish_task;
    publish_task_cfg.task_name = "publish_task";
    publish_task_cfg.config.core = 0;
    publish_task_cfg.config.priority = 20;
    publish_task_cfg.config.stack_size = 5000;
    create_task(publish_task_cfg);

#if GET_OUT_TEMP

    weather_task_cfg.task_func = weather_task;
    weather_task_cfg.task_name = "weather_task";
    weather_task_cfg.config.core = 1;
    weather_task_cfg.config.priority = 20;
    weather_task_cfg.config.stack_size = 5000;
    create_task(weather_task_cfg);

#endif


    temp_avgs.index = 0;
    temp_avgs.count = 0;
    temp_avgs.max = TEMP_AVGS_MAX;
    temp_avgs.array = calloc(temp_avgs.max, sizeof(float));

    temp_readings.index = 0;
    temp_readings.count = 0;
    temp_readings.max = TEMP_READINGS_MAX;
    temp_readings.array = calloc(temp_readings.max, sizeof(float));
    for(int i = 0; i < TEMP_READINGS_MAX; ++i)
    {
        float temp = get_temp(NUM_SAMPLES);
        circ_buf_insert(temp, &temp_readings);
        delay(100);
    }


    // for(int i = 0; i < TEMP_READINGS_MAX; ++i)
    // {
    //     temp_readings[i] = get_temp(NUM_SAMPLES);
    // }
    // temp_avg = calc_temp_avg();

    esp_timer_create_args_t timer_args = {};
    timer_args.callback = &timer_cb;
    timer_args.name = "timer_cb";
    err = esp_timer_create((const esp_timer_create_args_t*)&timer_args, &timer_handle);
    if(err != ESP_OK) {
        LOGE("Failed to create timer");
    }
    err = esp_timer_start_periodic(timer_handle, 1000 * 1000); // 1 second (unit is microseconds)
    if(err != ESP_OK) {
        LOGE("Failed to start timer");
    }

    esp_timer_create_args_t sensor_timer_args = {};
    sensor_timer_args.callback = &sensor_timer_cb;
    sensor_timer_args.name = "sensor_timer_cb";
    err = esp_timer_create((const esp_timer_create_args_t*)&sensor_timer_args, &sensor_timer_handle);
    if(err != ESP_OK) {
        LOGE("Failed to sensor create timer");
    }
    err = esp_timer_start_periodic(sensor_timer_handle, 200 * 1000); // 200ms (unit is microseconds)
    if(err != ESP_OK) {
        LOGE("Failed to start sensor timer");
    }


}


// ====================================================================
// CIRCULAR BUFFER
// ====================================================================

void circ_buf_insert(float value, circ_buf_t* circ_buf)
{
    circ_buf->index++;
    if(circ_buf->index >= circ_buf->max) circ_buf->index = 0;
    circ_buf->array[circ_buf->index] = value;
    circ_buf->count = MIN(circ_buf->count+1, circ_buf->max);
}


bool circ_buf_get_value(int get_index, float* value, circ_buf_t* circ_buf)
{
    if(get_index >= circ_buf->count)
    {
        return false;
    }

    int _index = circ_buf->index;
    bool valid = false;
    for(int i = 0; i < circ_buf->count; ++i)
    {
        if(i == get_index)
        {
            // this is the value we want
            valid = true;
            break;
        }
        if(_index == 0)
            _index = circ_buf->max-1; // wrap
        else
            _index--;
    }

    if(!valid) return false;

    *value = circ_buf->array[_index];
    return true;
}

bool circ_buf_calc_avg(float* avg, circ_buf_t* circ_buf)
{
    if(circ_buf->count == 0) return false;

    float sum = 0.0f;
    for(int i = 0; i < circ_buf->count; ++i)
    {
        sum += circ_buf->array[i];
    }
    *avg = (sum / (float)circ_buf->count);
    return true;
}


// void circ_buf_insert(float value, float** array, int* index, int* count, int max)
// {
//     *array[*index++] = value;
//     if(*index >= max) *index = 0;
//     *count = MIN(*count+1, max);
// }

// bool circ_buf_get_value(int get_index, float* value, float* array, int* index, int count, int max)
// {

//     if(get_index >= count)
//     {
//         return false;
//     }

//     int _index = *index;
//     bool valid = false;
//     for(int i = 0; i < count; ++i)
//     {
//         if(i == get_index)
//         {
//             // this is the value we want
//             valid = true;
//             break;
//         }
//         if(_index == 0)
//             _index = max-1; // wrap
//         else
//             _index--;
//     }

//     *value = *array[_index];
//     return true;



//     // // 0: newest
//     // // 9: oldest
//     // int idx = temp_readings_index-1-index;
//     // if(idx < 0)
//     // {
//     //     idx += TEMP_READINGS_MAX;
//     // }
//     // return temp_readings[idx];
// }


// float circ_buf_calc_avg(float* array, int count)
// {
//     float sum = 0.0f;
//     for(int i = 0; i < count; ++i)
//     {
//         sum += array[i];
//     }
//     return (sum / (float)count);
// }

// ====================================================================
// DATA FUNCTIONS
// ====================================================================




// float get_temp_reading(int index)
// {
//     // 0: newest
//     // 9: oldest
//     int idx = temp_readings_index-1-index;
//     if(idx < 0)
//     {
//         idx += TEMP_READINGS_MAX;
//     }
//     return temp_readings[idx];
// }

// float calc_temp_avg()
// {
//     float sum = 0.0f;
//     for(int i = 0; i < TEMP_READINGS_MAX; ++i)
//     {
//         sum += temp_readings[i];
//     }
//     return (sum / (float)TEMP_READINGS_MAX);
// }

bool data_add_to_queue(data_t item)
{
    if(data_queue_count == DATA_QUEUE_MAX)
        return false;

    while(data_queue_locked)
    {
        delay(1);
    }
    data_queue_locked = true;

    memcpy(&data_queue[data_queue_count], &item, sizeof(data_t));
    data_queue_count++;

    LOGI("added to queue data_queue_count = %d", data_queue_count);

    data_queue_locked = false;
    return true;
}

bool data_get_from_queue(data_t* item)
{
    if(data_queue_count == 0)
        return false;

    while(data_queue_locked)
    {
        delay(1);
    }
    data_queue_locked = true;

    memcpy(item, &data_queue[0], sizeof(data_t));
    for(int i = 1; i < DATA_QUEUE_MAX; ++i)
    {
        memcpy(&data_queue[i-1], &data_queue[i], sizeof(data_t));
    }
    memset(&data_queue[DATA_QUEUE_MAX-1], 0, sizeof(data_t));
    data_queue_count--;

    data_queue_locked = false;
    return true;
}

const char* data_type_to_str(data_type_t data_type)
{
    switch(data_type)
    {
        case DATA_TYPE_IN_TEMP:  return "IN_TEMP";
        case DATA_TYPE_OUT_TEMP: return "OUT_TEMP";
        default:                 return "UNKNOWN";
    }
}

// ====================================================================
// MISC FUNCTIONS
// ====================================================================

// -------------------------------
// STATIC
// -------------------------------

char* get_running_partition()
{
    const esp_partition_t* curr_partition = esp_ota_get_running_partition();
    return (char*)curr_partition->label;
}


const char* last_reset_reason_str()
{
    switch(esp_reset_reason())
    {
        case ESP_RST_UNKNOWN:   return "UNKNOWN";   // Reset reason can not be determined
        case ESP_RST_POWERON:   return "POWERON";   // Reset due to power-on event (occurs after flashing)
        case ESP_RST_EXT:       return "EXT";       // Reset by external pin (not applicable for ESP32)
        case ESP_RST_SW:        return "SW";        // Software reset via esp_restart
        case ESP_RST_PANIC:     return "PANIC";     // Software reset due to exception/panic
        case ESP_RST_INT_WDT:   return "INT_WDT";   // Reset (software or hardware) due to interrupt watchdog
        case ESP_RST_TASK_WDT:  return "TASK_WDT";  // Reset due to task watchdog
        case ESP_RST_WDT:       return "WDT";       // Reset due to other watchdogs (Ctrl+T+R while monitoring)
        case ESP_RST_DEEPSLEEP: return "DEEPSLEEP"; // Reset after exiting deep sleep mode
        case ESP_RST_BROWNOUT:  return "BROWNOUT";  // Brownout reset (software or hardware)
        case ESP_RST_SDIO:      return "SDIO";      // Reset over SDIO
    }
    return "UNKNOWN";
}

void print_task_list()
{

#if !RELEASE_MODE
    printf("********************************************************\n");

#if CONFIG_FREERTOS_USE_TRACE_FACILITY
    TaskStatus_t *task_status_array;
    volatile UBaseType_t num_tasks;
    char state_str[2] = {0};
    num_tasks = uxTaskGetNumberOfTasks();
    task_status_array = (TaskStatus_t*)pvPortMalloc(num_tasks * sizeof(TaskStatus_t));
    if(task_status_array == NULL) return;

#if configTASKLIST_INCLUDE_COREID
    printf("%-*s  %-4s  %-4s  %-4s  %-4s  %-4s\n"
        ,CONFIG_FREERTOS_MAX_TASK_NAME_LEN
        ,"TASK"
        ,"STATUS"
        ,"PRIO"
        ,"HWM"
        ,"TNUM"
        ,"CORE"
    );
#else
    printf("%-*s  %-4s  %-4s  %-4s  %-4s\n"
        ,CONFIG_FREERTOS_MAX_TASK_NAME_LEN
        ,"TASK"
        ,"STATUS"
        ,"PRIO"
        ,"HWM"
        ,"TNUM"
    );
#endif

    uxTaskGetSystemState(task_status_array, num_tasks, NULL);

    for(int i = 0; i < num_tasks; i++)
    {
        switch(task_status_array[i].eCurrentState)
        {
            case eReady:
                state_str[0] = 'R';
                break;
            case eBlocked:
                state_str[0] = 'B';
                break;
            case eSuspended:
                state_str[0] = 'S';
                break;
            case eDeleted:
                state_str[0] = 'D';
                break;
            default:
                state_str[0] = 'U';
                break;
        }

#if configTASKLIST_INCLUDE_COREID
        printf("%-*s  %-6s  %-4u  %-4u  %-4u  %-4hd\n"
            ,CONFIG_FREERTOS_MAX_TASK_NAME_LEN
            ,task_status_array[i].pcTaskName
            ,state_str
            ,( unsigned int ) task_status_array[i].uxCurrentPriority
            ,( unsigned int ) task_status_array[i].usStackHighWaterMark
            ,( unsigned int ) task_status_array[i].xTaskNumber
            ,( short ) task_status_array[i].xCoreID
        );
#else
        printf("%-*s  %-6s  %-4u  %-4u  %-4u\n"
            ,CONFIG_FREERTOS_MAX_TASK_NAME_LEN
            ,task_status_array[i].pcTaskName
            ,state_str
            ,( unsigned int ) task_status_array[i].uxCurrentPriority
            ,( unsigned int ) task_status_array[i].usStackHighWaterMark
            ,( unsigned int ) task_status_array[i].xTaskNumber
        );
#endif
        // task_status_array[i].xHandle
    }
    vPortFree(task_status_array);

#endif

    printf("********************************************************\n");
#endif
}



// -------------------------------
// GLOBAL
// -------------------------------

bool create_task(task_data_t task_data)
{
    LOGI("Creating task: '%s', Stack size: %d, Priority: %d, Core: %d", task_data.task_name, task_data.config.stack_size, task_data.config.priority, task_data.config.core);
    int ret = xTaskCreatePinnedToCore(task_data.task_func, task_data.task_name, task_data.config.stack_size, NULL, task_data.config.priority, &task_data.config.handle, task_data.config.core);
    if(ret != pdPASS)
    {
        LOGE("Failed to create '%s' task, ret: %d", task_data.task_name, ret);
        return false;
    }
    return true;

}

uptime_t get_uptime()
{
    int64_t _uptime = MIN(esp_timer_get_time()/1000000,0xFFFFFFFF); // convert to seconds
    if(_uptime < 0) _uptime = 0xFFFFFFFF; // max of uint32_t
    return _uptime;
}

void uptime_to_str(uptime_t uptime, char* ret_str)
{
    int days = uptime / 86400;
    uptime -= (days*86400);
    int hours = uptime / 3600;
    uptime -= (hours*3600);
    int minutes = uptime / 60;
    uptime -= (minutes*60);
    int seconds = uptime;
    sprintf(ret_str, "%02dd %02dh %02dm %02ds", days, hours, minutes, seconds);
}

size_t get_free_heap()
{
    return heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

void print_bytes(uint8_t* bytes, int len, int per_line, char* fmt, char* sep)
{
    if(bytes == NULL) return;
    if(len <= 0) return;
    if(STR_EMPTY(fmt)) return;

    per_line = MAX(per_line, 1);
    char* _sep = sep == NULL ? "" : sep;

    int c = 0;
    for(int i = 0; i < len; ++i)
    {
        c++;
        printf(fmt, bytes[i]);
        if(c == per_line || (i+1) == len)
        {
            c = 0;
            printf("\n");
        }
        else
        {
            printf("%s", _sep);
        }

    }
    fflush(stdout);
}

bool mac_str_to_bytes(char* mac_str, uint8_t ret_bytes[6], bool has_colons)
{
    int idx = 0;
    for(int i = 0; i < 6; ++i)
    {
        char b0 = mac_str[idx++];
        char b1 = mac_str[idx++];
        char buf[5] = {'0', 'x', b0, b1, 0};
        long val = strtol(buf, NULL, 0);
        ret_bytes[i] = (uint8_t)val;
        if(has_colons) idx++;
    }
    return true;
}

bool str_append(char* str, int max_len, const char* format, ...)
{
    int len = strlen(str);
    int rem_len = max_len - len;

    va_list args;
    va_start(args, format);
    int check = vsnprintf(str+len, rem_len, format, args);
    va_end(args);

    // The number of characters that would have been written if n had been sufficiently large, not counting the terminating null character.
    // If an encoding error occurs, a negative number is returned.
    // Notice that only when this returned value is non-negative and less than n, the string has been completely written.
    if(check < 0 || check > rem_len) {
        LOGE("Failed to format string");
        return false;
    }

    return true;
}

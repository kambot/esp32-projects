#pragma once

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_ota_ops.h"
#include "esp_heap_caps.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <math.h>

// -------------------------------------------------------
// storage
// -------------------------------------------------------
#define NVS_PARTITION "nvs"
#define NVS_NAMESPACE "data"
#define NVS_KEY_SSID "wifi_ssid"
#define NVS_KEY_PASS "wifi_pass"
#define NVS_KEY_AUTH "wifi_auth"

// -------------------------------------------------------
// devices
// -------------------------------------------------------
#define DEVICE_LR 1


// -------------------------------------------------------
// configurations
// -------------------------------------------------------

#define DEVICE_ROLE     DEVICE_LR
#define RELEASE_MODE    0
#define NUM_SAMPLES     100 // for temp sensor reading
#define TIME_ZONE       "America/New_York"

#if DEVICE_ROLE == DEVICE_LR

#define DEVICE_NAME "living_room"

#define GET_OUT_TEMP 1
#define OUT_TEMP_FREQ (SECONDS_IN_HOUR)

#define GET_SENSOR_TEMP 1
#define TEMP_SENSOR_FREQ (60*5)

#endif


// -------------------------------------------------------
// misc
// -------------------------------------------------------

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
#define IDF_VERSION 4
#else
#define IDF_VERSION 3
#endif

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
#define IDF_VERSION_MAJOR_MINOR 44
#elif ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
#define IDF_VERSION_MAJOR_MINOR 43
#elif ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 2, 0)
#define IDF_VERSION_MAJOR_MINOR 42
#elif ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0)
#define IDF_VERSION_MAJOR_MINOR 41
#elif ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
#define IDF_VERSION_MAJOR_MINOR 40
#else
#define IDF_VERSION_MAJOR_MINOR 33
#endif

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// math
#define MAX(a, b)      ((a) > (b) ? (a) : (b))
#define MIN(a, b)      ((a) < (b) ? (a) : (b))
#define ABS(a)         ((a) < 0 ? -1*(a) : (a))
#define SQUARE(a)      ((a) * (a))
#define BOUND(value,lbound,ubound)    (MIN(MAX((value), (lbound)), (ubound)))

// strings
#define STR_EMPTY(x)      (x == 0 || strlen(x) == 0)
#define STR_EQUAL(x,y)    (strncmp((x),(y),strlen((x))) == 0 && strlen(x) == strlen(y))

#define BOOLSTR(b) (b) ? "true" : "false"

// delay milliseconds
#define delay(a)    vTaskDelay(pdMS_TO_TICKS(a))


// miscellaneous macros for debugging
#define DEBUG_PRINT() printf("[DEBUG] File: %s | Func: %s | Line: %d\n", __FILE__, __func__,__LINE__)
#define CHECK_HEAP() if(!heap_caps_check_integrity_all(true)) printf("[CORRUPT HEAP] File: %s | Func: %s | Line: %d\n", __FILE__, __func__, __LINE__)
#define LOG_TASK_HWM() LOGI("Task: '%s' (%d), High Water Mark: %u, Free Heap: %d", pcTaskGetTaskName(NULL), xPortGetCoreID(), uxTaskGetStackHighWaterMark(NULL), xPortGetFreeHeapSize())

// mac address macros
#define MAC_FMT "%02X:%02X:%02X:%02X:%02X:%02X"
#define MAC_LIST(mac) (mac)[0],(mac)[1],(mac)[2],(mac)[3],(mac)[4],(mac)[5]
#define MAC_IS_EMPTY(mac) ((mac)[0] == 0 && (mac)[1] == 0 && (mac)[2] == 0 && (mac)[3] == 0 && (mac)[4] == 0 && (mac)[5] == 0)

#define BINARY_LIST(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')
#define BINARY_FMT "%c%c%c%c%c%c%c%c"

// esp-idf return values
#define CHECK_ESP_ERR(msg) { \
    if(err != ESP_OK) { \
        LOGE("%s, error: 0x%x (%s)", msg, err, esp_err_to_name(err)); \
        return false; \
    } \
}

#define CHECK_ESP_ERR_NO_RET(msg) { \
    if(err != ESP_OK) { \
        LOGE("%s, error: 0x%x (%s)", msg, err, esp_err_to_name(err)); \
    } \
}

#define MAX_PRIORITY (configMAX_PRIORITIES-1)


// ====================================================================
// TYPEDEFS
// ====================================================================

typedef struct
{
    float* array;
    int index;
    int count;
    int max;
} circ_buf_t;

typedef struct
{
    int stack_size;
    int priority;
    int core;
    TaskHandle_t handle;
} task_config_t;

typedef struct
{
    task_config_t config;
    TaskFunction_t task_func;
    const char* task_name;
} task_data_t;


// time since boot (in seconds)
typedef uint32_t uptime_t;

#include "console_log.h"

typedef enum
{
    DATA_TYPE_IN_TEMP,
    DATA_TYPE_OUT_TEMP,

    DATA_TYPE_MAX
} data_type_t;

typedef struct
{
    time_t timestamp;
    data_type_t data_type;
    float value_num;
    bool value_bool;
    char value_str[10];
    uptime_t uptime;
} data_t;


// ====================================================================
// GLOBAL VARIABLES
// ====================================================================

extern uint8_t mac_address[6];
extern uint8_t bt_mac_address[6];

// ====================================================================
// GLOBAL FUNCTIONS
// ====================================================================

void circ_buf_insert(float value, circ_buf_t* circ_buf);
bool circ_buf_get_value(int get_index, float* value, circ_buf_t* circ_buf);
bool circ_buf_calc_avg(float* avg, circ_buf_t* circ_buf);


float get_temp_reading(int index);
float calc_temp_avg();
bool data_add_to_queue(data_t item);
bool data_get_from_queue(data_t* item);
const char* data_type_to_str(data_type_t data_type);

bool create_task(task_data_t task_data);
uptime_t get_uptime();
void uptime_to_str(uptime_t uptime, char* ret_str);
size_t get_free_heap();
void print_bytes(uint8_t* bytes, int len, int per_line, char* fmt, char* sep);
bool mac_str_to_bytes(char* mac_str, uint8_t ret_bytes[6], bool has_colons);
bool str_append(char* str, int max_len, const char* format, ...);


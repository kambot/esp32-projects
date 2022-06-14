/*
NOTES
https://script.google.com/home/?pli=1
https://github.com/walidamriou/ESP_Data_to_Google_Sheets


EXEC:
https://script.google.com/macros/s/AKfycbzec30qonlzTAWnisb_CLsjdsq7-jEdejkrTdgzmluW48tU3IENSdARUbbUdGcPXLKm/exec

DEPLOYMENT ID:
AKfycbzec30qonlzTAWnisb_CLsjdsq7-jEdejkrTdgzmluW48tU3IENSdARUbbUdGcPXLKm

EDIT CODE:
https://script.google.com/home/projects/1TVCEwkP_O5XrHciDdwkVNIRS8z6ZJPZ4kEATW75xHcil-nd-gj4ABv-d/edit

*/

// ====================================================================
// INCLUDES
// ====================================================================

#include "common.h"
#include "storage.h"
#include "datetime.h"
#include "wifi.h"
#include "https.h"


// ====================================================================
// VARIABLES
// ====================================================================

esp_timer_handle_t timer_handle;
esp_timer_handle_t sensor_timer_handle;
task_data_t bd_task_cg = {0};
uint8_t mac_address[6] = {0};
uint8_t bt_mac_address[6] = {0};


char* bd_data = NULL; // string (csv format)

#define BD_LIST_MAX 100
birthday_t bd_list[BD_LIST_MAX] = {0};
int bd_list_count = 0;

bool refresh_bd_rank = true;

int yday = 0;


// ====================================================================
// MACROS/DEFINES
// ====================================================================

#define BD_DOWNLOAD_SECONDS  (10*60)

// ====================================================================
// STATIC FUNCTION DECLARATIONS
// ====================================================================

char* get_running_partition();
const char* last_reset_reason_str();
void print_task_list();

void init();
void birthdays_task(void* arg);
void IRAM_ATTR timer_cb(void* arg);



// ====================================================================
// MAIN
// ====================================================================

void app_main()
{
    init();
    print_task_list();

    // //TEST
    // for(;;)
    // {
    //     if(TIME_NOW() != 0)
    //     {
    //         struct tm t = time_now_tm_local();
    //         printf("day:  %d\n", t.tm_mday);
    //         printf("mon:  %d\n", t.tm_mon); //0 based
    //         printf("year: %d\n", t.tm_year+1900);
    //         printf("yday: %d\n", t.tm_yday);    //0 based
    //         break;
    //     }
    //     delay(500);
    // }

    vTaskDelete(NULL);
}

void birthdays_task(void* arg)
{
    LOGI("Entered birthday task");

    // wait for sntp sync
    for(;;)
    {
        if(TIME_NOW() != 0) break;
        delay(100);
    }

    load_bd_data();
    if(bd_data != NULL)
    {
        parse_bd_data();
    }


    static int64_t refresh_timer = 0;

    for(;;)
    {

        if(wifi_get_internet_status() && esp_timer_get_time() >= refresh_timer)
        {
            // if this is successful, it will also update bd_data and store to NVS partition
            bool ret = get_birthdays();

            if(ret)
            {
                refresh_timer = esp_timer_get_time() + BD_DOWNLOAD_SECONDS*1e6;
            }
            else
            {
                refresh_timer = esp_timer_get_time() + 30*1e6;
            }

        }

        if(refresh_bd_rank)
        {
            rank_bd_list();
        }

        delay(1000);
    }

    LOGI("Leaving birthday task");
    vTaskDelete(NULL);
}




void IRAM_ATTR timer_cb(void* arg)
{
    if(TIME_NOW() != 0)
    {
        struct tm t = time_now_tm_local();
        int _yday = t.tm_yday+1;

        if(yday != _yday)
        {
            LOGI("yday: %d -> %d", yday, _yday);
            yday = _yday;
            refresh_bd_rank = true;
        }

    }


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
    wifi_set_all("STUDIO_1837_CORP", "soap4life", 3, WIFI_CONTEXT_PRIMARY);    // temp

    wifi_enable();
    wifi_connect(); // must always call this after setting wifi credentials



    bd_task_cg.task_func = birthdays_task;
    bd_task_cg.task_name = "birthdays_task";
    bd_task_cg.config.core = 0;
    bd_task_cg.config.priority = 10;
    bd_task_cg.config.stack_size = 5000;
    create_task(bd_task_cg);

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




bool hexstr_to_bytes(char* hexstr, uint8_t* ret_bytes)
{
    int len = strlen(hexstr);
    if(len % 2 != 0 || len == 0) return false;
    for(int i = 0; i < (len/2); ++i)
    {
        char b0 = hexstr[i*2];
        char b1 = hexstr[i*2+1];
        char buf[5] = {'0', 'x', b0, b1, 0};
        long val = strtol(buf, NULL, 0);
        ret_bytes[i] = (uint8_t)val;
    }
    return true;
}


char* decrypt_data(char* hexstr)
{
    int num_bytes = strlen(hexstr) / 2;
    uint8_t* bytes0 = calloc(num_bytes, sizeof(uint8_t));

    bool ret = hexstr_to_bytes(hexstr, bytes0);
    if(!ret)
    {
        LOGE("Failed to convert to bytes");
        free(bytes0);
        return NULL;
    }

    char* data = calloc(num_bytes+1, sizeof(char));
    for(int i = 0; i < num_bytes; ++i)
    {
        data[i] = (~bytes0[i]) ^ XOR;
    }

    free(bytes0);
    return data;
}

bool load_bd_data()
{
    size_t size = 0;
    bool ret = store_nvs_read_str(NVS_NAMESPACE, NVS_BD_LIST, NULL, &size);

    if(!ret || size == 0)
    {
        LOGE("Failed to load birthday data");
        return false;
    }

    if(bd_data) free(bd_data);
    bd_data = calloc(size+1, sizeof(char));

    ret = store_nvs_read_str(NVS_NAMESPACE, NVS_BD_LIST, bd_data, &size);
    if(!ret)
    {
        free(bd_data);
        bd_data = NULL;
        LOGE("Failed to load birthday data");
        return false;
    }

    LOGI("Loaded birthday data");
    return true;
}

bool save_bd_data()
{
    bool ret = store_nvs_write_str(NVS_NAMESPACE, NVS_BD_LIST, bd_data);
    if(ret) LOGI("Saved birthday data");
    else LOGE("Failed to save birthday data");
    return ret;
}

bool erase_bd_data()
{
    bool ret = store_nvs_erase_key(NVS_NAMESPACE, NVS_BD_LIST);
    if(ret) LOGI("Erased birthday data");
    else LOGE("Failed to erase birthday data");
    return ret;
}

bool line_to_bd_info(char* str, birthday_t* b)
{
    // char name[30] = {0};
    // int month = 0, day = 0, year = 0;
    birthday_t bd = {0};

    int obj = 0;
    char buf[30] = {0};
    int idx = 0;

    int len = strlen(str);
    for(int i = 0; i < len; ++i)
    {
        char c = str[i];
        if(c != ',') buf[idx++] = c;
    
        if(c == ',' || i == (len-1))
        {
            if(obj == 0) //name
            {
                if(strlen(buf) == 0)
                {
                    printf("parse name failed -> %s\n", str);
                    return false;
                }
                memcpy(bd.name, buf, MIN(30,strlen(buf)));
            }
            else if(obj == 1) //month
            {
                int month = atoi(buf);
                if(month <= 0 || month > 12)
                {
                    printf("parse month failed -> %s\n", str);
                    return false;
                }
                bd.month = (uint8_t)month;
            }
            else if(obj == 2) //day
            {
                int day = atoi(buf);
                if(day <= 0 || day > 31)
                {
                    printf("parse day failed -> %s\n", str);
                    return false;
                }
                bd.day = (uint8_t)day;
            }

            obj++;
            if(obj > 2)
            {
                bd.yday = get_day_of_year(bd.month, bd.day);
                if(bd.yday == 0) return false;

                if(b != NULL) memcpy(b, &bd, sizeof(birthday_t));
                return true;
            }

            memset(buf, 0, 30);
            idx = 0;
        }

    }

    return false;
}

void parse_bd_data()
{
    if(bd_data == NULL) return;

    char buf[100] = {0};
    int idx = 0;

    bd_list_count = 0;

    int len = strlen(bd_data);
    for(int i = 0; i < len; ++i)
    {
        char c = bd_data[i];

        if(c != '\n') buf[idx++] = c;

        if(c == '\n' || i == (len-1))
        {
            birthday_t bd = {0};
            bool ret = line_to_bd_info(buf, &bd);

            if(ret)
            {
                bool add = true;
                for(int k = 0; k < bd_list_count; ++k)
                {
                    if(memcmp(&bd_list[k], &bd, sizeof(birthday_t)) == 0)
                    {
                        printf("duplicate -> %s\n", buf);
                        add = false;
                        break;
                    }
                }

                if(add)
                {
                    memcpy(&bd_list[bd_list_count++], &bd, sizeof(birthday_t));
                }

            }

            memset(buf,0,100);
            idx = 0;
            continue;
        }
    }

    LOGI("Birthday list count: %d", bd_list_count);
    for(int i = 0; i < bd_list_count; ++i)
    {
        birthday_t* b = &bd_list[i];
        LOGI("%u -> %s %u/%u", b->yday, b->name, b->month, b->day);
    }

    rank_bd_list();

}


void swap_b(birthday_t* a, birthday_t* b)
{
    birthday_t temp = *a;
    *a = *b;
    *b = temp;
}

void bubble_sort_bd_list()
{
    int i, j;
    for(i = 0; i < bd_list_count-1; i++)
    {
        for(j = 0; j < bd_list_count-i-1; j++)
        {
            // want the negative numbers to wrap
            uint16_t n1 = bd_list[j].yday - yday;
            uint16_t n2 = bd_list[j+1].yday - yday;

            if(n1 > n2)
                swap_b(&bd_list[j], &bd_list[j+1]);
        }
    }
}


void rank_bd_list()
{
    if(bd_list_count < 0)
    {
        refresh_bd_rank = false;
        return;
    }

    if(yday == 0) return;

    bubble_sort_bd_list();
    LOGI("SORTED:");
    for(int i = 0; i < bd_list_count; ++i)
    {
        birthday_t* b = &bd_list[i];
        LOGI("%u %u -> %s %u/%u ", b->yday, (uint16_t)(b->yday - yday), b->name, b->month, b->day);
    }
    refresh_bd_rank = false;
}
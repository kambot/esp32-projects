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
#include "i2c.h"
#include "fonts.h"
#include "oled.h"
#include "button.h"
#include "gui.h"


// ====================================================================
// VARIABLES
// ====================================================================

esp_timer_handle_t timer_handle;
esp_timer_handle_t sensor_timer_handle;
task_data_t gui_task_cfg = {0};
task_data_t bd_task_cfg = {0};
uint8_t mac_address[6] = {0};
uint8_t bt_mac_address[6] = {0};

char* bd_data = NULL; // string (csv format)
birthday_t bd_list[BD_LIST_MAX] = {0};
int bd_list_count = 0;
bool refresh_bd_rank = true;
bool bd_list_updated = false;

int yday = 0;
int year = 0;

int64_t download_timer = 0;
bool downloading_data = false;
bool update_gui_date = false;
bool selection_changed = false;
int sel_idx = 0;
int sel_bd_idx = 0;

int64_t start_running_man_timer = -1e12;


struct
{
    bool enabled;
    uint8_t frames1[5];
    uint8_t frames2[5];
    float frame_counter;
    uint8_t frame_index;
    uint8_t speed;
    float x;
    uint8_t y;
    int8_t dir;
} running_man;

// ====================================================================
// MACROS/DEFINES
// ====================================================================

#define BD_DOWNLOAD_SECONDS  (10*60)

#define BTN_PIN 27



// gui
#define NOT_SELECTED  "     "
#define SELECTED      ICON_SELECTOR " "

#define WIFI_I  0
#define WIFI_X  110
#define WIFI_Y  0

#define DOWN_I  1
#define DOWN_X  95
#define DOWN_Y  0

#define DATE_I  2
#define DATE_X  0
#define DATE_Y  0

#define CNT_I   3
#define CNT_X   64
#define CNT_Y   0

//selected birthday date
#define BDD_I   4
#define BDD_X   0
#define BDD_Y   16

#define DAYS_I  5
#define DAYS_X  80
#define DAYS_Y  16

#define BD1_I   6
#define BD1_X   0
#define BD1_Y   32

#define BD2_I   7
#define BD2_X   0
#define BD2_Y   48

#define RUNNER1_I   8
#define RUNNER2_I   9

#define SCREEN_LOAD     GUI_SCREEN_1
#define SCREEN_BD       GUI_SCREEN_2
#define SCREEN_RUNNER   GUI_SCREEN_3

// ====================================================================
// STATIC FUNCTION DECLARATIONS
// ====================================================================

char* get_running_partition();
const char* last_reset_reason_str();
void print_task_list();

void init();
void birthdays_task(void* arg);
void IRAM_ATTR timer_cb(void* arg);

// button
void btn_1_press();
void btn_5_press();
void btn_3_press();
void btn_hold_running_man();

//gui
void gui_update();
void gui_draw();

// ====================================================================
// MAIN
// ====================================================================

void app_main()
{
    init();

    vTaskDelete(NULL);
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

    time_init(30*60);
    time_set_timezone_id(TIME_ZONE);

    wifi_init();
    // wifi_debug_logs(true);
    wifi_set_max_reconnect_attempts(10);
    wifi_set_reconnect_time(10);
    wifi_enable();

    // wifi_load_credentials();
    // wifi_set_all("STUDIO_1837_CORP", "soap4life", 3, WIFI_CONTEXT_PRIMARY);
    // wifi_connect(); // must always call this after setting wifi credentials


    bd_task_cfg.task_func = birthdays_task;
    bd_task_cfg.task_name = "birthdays_task";
    bd_task_cfg.config.core = 1;
    bd_task_cfg.config.priority = 10;
    bd_task_cfg.config.stack_size = 5000;
    create_task(bd_task_cfg);

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

    button_params_t btn_cfg = BUTTON_DEFAULT_PARAMS();
    button_init(btn_cfg, 1, 5);
    button_config_pin(PIN_MASK(BTN_PIN), 0);
    button_add_event_press(PIN_MASK(BTN_PIN), PRESSES, 1, btn_1_press);
    button_add_event_hold_mod(PIN_MASK(BTN_PIN), 1000, 1000000, 300, btn_1_press);

    button_add_event_press(PIN_MASK(BTN_PIN), PRESSES_EXACT, 5, btn_5_press);
    button_add_event_press(PIN_MASK(BTN_PIN), PRESSES_EXACT, 3, btn_3_press);

    button_add_event_hold(PIN_MASK(BTN_PIN), 1000, 1100, btn_hold_running_man, NULL, NULL);

    running_man.frames1[0] = '\x90';
    running_man.frames2[0] = '\x91';
    running_man.frames1[1] = '\x92';
    running_man.frames2[1] = '\x93';
    running_man.frames1[2] = '\x94';
    running_man.frames2[2] = '\x95';
    running_man.frames1[3] = '\x96';
    running_man.frames2[3] = '\x97';
    running_man.frames1[4] = '\x98';
    running_man.frames2[4] = '\x99';
    running_man.frame_counter = 0;
    running_man.frame_index = 0;
    running_man.speed = 1 + esp_random() % 5;
    running_man.x = 0;
    running_man.y = 16;
    running_man.dir = 1;
    running_man.enabled = true;

    gui_init(gui_update);
    gui_set_item(WIFI_I, SCREEN_BD|SCREEN_LOAD, WIFI_X, WIFI_Y, false, false, ICON_WIFI_DISCONNECTED);
    gui_set_item(DOWN_I, SCREEN_BD|SCREEN_LOAD, DOWN_X, DOWN_Y, false, false, "");
    gui_set_item(DATE_I, SCREEN_BD, DATE_X, DATE_Y, false, false, "");
    gui_set_item(CNT_I,  SCREEN_BD, CNT_X,  CNT_Y,  true,  false, "%2d of %-2d", 0, bd_list_count);
    gui_set_item(BDD_I,  SCREEN_BD, BDD_X,  BDD_Y,  false, false, "");
    gui_set_item(DAYS_I, SCREEN_BD, DAYS_X, DAYS_Y, false, false, "");
    gui_set_item(BD1_I,  SCREEN_BD, BD1_X,  BD1_Y,  false, false, "");
    gui_set_item(BD2_I,  SCREEN_BD, BD2_X,  BD2_Y,  false, false, "");
    gui_set_item(RUNNER1_I,  SCREEN_BD|SCREEN_RUNNER|SCREEN_LOAD, 0, 0,  false, false, "");
    gui_set_item(RUNNER2_I,  SCREEN_BD|SCREEN_RUNNER|SCREEN_LOAD, 0, 0,  false, false, "");
    gui_set_screen(SCREEN_LOAD);

    print_task_list();

    for(;;)
    {
        wifi_list_ap();
        uint8_t count =  wifi_get_num_accesspoints();
        bool found_network = false;

        for(int i = 0; i < count; ++i)
        {
            wifi_accesspoint_t ap = {0};
            wifi_get_ap_info(i, &ap);
            if(STR_EQUAL((char*)ap.ssid, "KebertXela"))
            {
                wifi_set_all((char*)ap.ssid, "Gumbercules2000", 3, WIFI_CONTEXT_PRIMARY);
                found_network = true;
                break;
            }
            else if(STR_EQUAL((char*)ap.ssid, "STUDIO_1837_CORP"))
            {
                wifi_set_all((char*)ap.ssid, "soap4life", 3, WIFI_CONTEXT_PRIMARY);
                found_network = true;
                break;
            }
        }

        if(found_network)
        {
            wifi_connect();
            break;
        }

        delay(2000);
    }


}


void birthdays_task(void* arg)
{
    LOGI("Entered birthday task");

    bool first_download = true;

    // wait for sntp sync
    for(;;)
    {
        if(yday != 0) break;
        delay(200);
    }

    // TEMP
    // erase_bd_data();

    load_bd_data();
    parse_bd_data();
    rank_bd_list();


    if(bd_list_count > 0)
    {
        first_download = false;
        gui_set_screen(SCREEN_BD);
    }

    for(;;)
    {

        if(wifi_get_internet_status() && esp_timer_get_time() >= download_timer)
        {
            // if this is successful, it will also update bd_data and store to NVS partition
            downloading_data = true;
            bool ret = get_birthdays();
            downloading_data = false;

            if(ret)
            {
                download_timer = esp_timer_get_time() + BD_DOWNLOAD_SECONDS*1e6;
            }
            else
            {
                download_timer = esp_timer_get_time() + 10*1e6;
            }

        }

        if(refresh_bd_rank)
        {
            if(first_download && bd_list_count > 0)
            {
                first_download = false;
                gui_set_screen(SCREEN_BD);
            }

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
        year = t.tm_year+1900;

        if(yday != _yday)
        {
            LOGI("yday: %d -> %d", yday, _yday);
            yday = _yday;
            refresh_bd_rank = true;
            update_gui_date = true;
        }
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
                    LOGW("parse name failed -> %s", str);
                    return false;
                }
                memcpy(bd.name, buf, MIN(30,strlen(buf)));
            }
            else if(obj == 1) //month
            {
                int month = atoi(buf);
                if(month <= 0 || month > 12)
                {
                    LOGW("parse month failed -> %s", str);
                    return false;
                }
                bd.month = (uint8_t)month;
            }
            else if(obj == 2) //day
            {
                int day = atoi(buf);
                if(day <= 0 || day > 31)
                {
                    LOGW("parse day failed -> %s", str);
                    return false;
                }
                bd.day = (uint8_t)day;
            }

            obj++;
            if(obj > 2)
            {
                bd.yday = get_day_of_year(bd.month, bd.day, year);
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
    LOGI("bd_data: %p", bd_data);
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
                        LOGW("duplicate -> %s", buf);
                        add = false;
                        break;
                    }
                }

                if(add)
                {
                    LOGI("adding to to bd list at index: %d", bd_list_count);
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
        print_birthday_t(b);
    }

    // rank_bd_list();
}

// year is current year
uint16_t days_until_next_bd(uint8_t _month, uint8_t _day, uint16_t _year)
{
    uint16_t bd_yday = get_day_of_year(_month, _day, year);

    int check = bd_yday - yday;
    if(check >= 0)
    {
        return (uint16_t)check;
    }

    int days_in_curr_year = 365;
    if(is_leap_year(_year))
    {
        days_in_curr_year = 366;
    }

    int rem_days = days_in_curr_year - yday;
    uint16_t days = rem_days + bd_yday;
    return days;
}

void print_birthday_t(birthday_t* b)
{
    if(b == NULL) return;
    LOGI("%-20s %2u/%-2u (%-3u, %-3u)", b->name, b->month, b->day, b->yday, b->countdown);
}


void swap_bd(birthday_t* a, birthday_t* b)
{
    birthday_t temp = *a;
    *a = *b;
    *b = temp;
}

void bubble_sort_bd_list()
{

    for(int i = 0; i < bd_list_count; ++i)
    {
        bd_list[i].countdown = days_until_next_bd(bd_list[i].month, bd_list[i].day, year);
    }

    for(int i = 0; i < bd_list_count-1; i++)
    {
        for(int j = 0; j < bd_list_count-i-1; j++)
        {
            // want the negative numbers to wrap
            // uint16_t n1 = bd_list[j].yday - yday;
            // uint16_t n2 = bd_list[j+1].yday - yday;
            uint16_t n1 = bd_list[j].countdown;
            uint16_t n2 = bd_list[j+1].countdown;

            if(n1 > n2)
                swap_bd(&bd_list[j], &bd_list[j+1]);
        }
    }
}

void rank_bd_list()
{
    if(bd_list_count <= 0)
    {
        refresh_bd_rank = false;
        return;
    }

    if(yday == 0) return;

    bubble_sort_bd_list();
    LOGI("SORTED (%d):", bd_list_count);
    for(int i = 0; i < bd_list_count; ++i)
    {
        birthday_t* b = &bd_list[i];
        print_birthday_t(b);
    }
    refresh_bd_rank = false;
    bd_list_updated = true;
}

// button

void btn_1_press()
{

    if((esp_timer_get_time() - start_running_man_timer) <= 2e6)
    {
        return;
    }

    gui_screen_t screen = gui_get_screen();
    if(screen == SCREEN_RUNNER)
    {
        gui_set_screen(SCREEN_BD);
        // sel_bd_idx = 0;
        // sel_idx = 0;
        // selection_changed = true;
    }
    else if(screen == SCREEN_LOAD)
    {
        running_man.speed += 1;
        printf("speed: %u\n", running_man.speed);
    }
    else
    {

        if(bd_list_count > 0)
        {
            sel_idx++;
            if(sel_idx >= 2) sel_idx = 0;

            sel_bd_idx++;
            if(sel_bd_idx >= bd_list_count)
            {
                sel_bd_idx = 0;
                sel_idx = 0;
            }
            selection_changed = true;
        }
    }
}

void btn_5_press()
{
    download_timer = 0;
}

void btn_3_press()
{
    start_running_man_timer = esp_timer_get_time();
}

void btn_hold_running_man()
{
    if((esp_timer_get_time() - start_running_man_timer) <= 2e6)
    {
        start_running_man_timer = esp_timer_get_time();
        gui_set_screen(SCREEN_RUNNER);
        running_man.enabled = true;
    }
}





void running_man_update()
{
    if(!running_man.enabled) return;

    // uint16_t target_frame_period = 10;
    uint16_t target_frame_period = 10; // update frame rate
    uint16_t gui_period = gui_get_frame_period();   //50ms

    float p = (float)gui_period/(float)target_frame_period;

    // update the frames
    running_man.frame_counter += p;
    if(running_man.frame_counter >= 1)
    {
        running_man.frame_counter = 0;
        running_man.frame_index++;
        if(running_man.frame_index >= 5)
            running_man.frame_index = 0;
    }

    float x = running_man.x;
    x += running_man.speed * running_man.dir * 1;


    bool update_y = false;
    if(running_man.dir == 1)
    {
        if(x > 129)
        {
            running_man.dir = -1;
            update_y = true;
            x = 129;
        }
    }
    else
    {
        if(x < -18)
        {
            running_man.dir = 1;
            update_y = true;
            x = -18;
        }
    }

    running_man.x = x;

    if(update_y)
    {
        if(running_man.y == 16) running_man.y = 42;
        else if(running_man.y == 42) running_man.y = 16;
        running_man.speed = 1 + esp_random() % 5;

        // let him run off of the screen
        gui_screen_t screen = gui_get_screen();
        if(screen != SCREEN_RUNNER && screen != SCREEN_LOAD)
        {
            running_man.enabled = false;
            return;
        }
    }

    bool rev = running_man.dir == -1;

    gui_set_coords(RUNNER1_I, (uint8_t)running_man.x, running_man.y);
    gui_set_text(RUNNER1_I, rev, "%c",running_man.frames1[running_man.frame_index]);

    gui_set_coords(RUNNER2_I, (uint8_t)running_man.x, running_man.y+11);
    gui_set_text(RUNNER2_I, rev, "%c",running_man.frames2[running_man.frame_index]);
}

void gui_update()
{

    static gui_screen_t prior_screen = GUI_SCREEN_NONE;

    gui_screen_t screen = gui_get_screen();

    if(screen == SCREEN_BD)
    {

        if(bd_list_updated)
        {
            gui_set_text(CNT_I, false, "%2d of %-2d", bd_list_count > 0 ? 1 : 0, bd_list_count);
            sel_idx = 0;
            sel_bd_idx = 0;
            selection_changed = true;
            bd_list_updated = false;
        }

        if(selection_changed)
        {
            LOGI("sel_idx: %d, sel_bd_idx, %d", sel_idx, sel_bd_idx);

            gui_set_text(CNT_I, false, "%2d of %-2d", bd_list_count > 0 ? sel_bd_idx+1 : 0, bd_list_count);

            if(bd_list_count > 0)
            {
                if(sel_idx == 0)
                {
                    gui_set_text(BD1_I, false, SELECTED " %s", bd_list[sel_bd_idx].name);

                    if((sel_bd_idx+1) < bd_list_count)
                    {
                        gui_set_text(BD2_I, false, NOT_SELECTED " %s", bd_list[sel_bd_idx+1].name);
                    }
                    else
                    {
                        // at the end of the list
                        gui_set_text(BD2_I, false, "");
                    }

                }
                else
                {
                    gui_set_text(BD1_I, false, NOT_SELECTED " %s", bd_list[sel_bd_idx-1].name);
                    gui_set_text(BD2_I, false, SELECTED " %s", bd_list[sel_bd_idx].name);
                }

            }
            else
            {
                gui_set_text(BD1_I, false, "");
                gui_set_text(BD2_I, false, "");
            }

            gui_set_text(BDD_I, false, "Birthday: %u/%u", bd_list[sel_bd_idx].month, bd_list[sel_bd_idx].day);
            gui_set_text(DAYS_I, false, "Days: %u", bd_list[sel_bd_idx].countdown);

            selection_changed = false;
        }

        if(update_gui_date)
        {
            struct tm t = time_now_tm_local();
            gui_set_text(DATE_I, false, "%d/%d", t.tm_mon+1, t.tm_mday);
            update_gui_date = false;
        }

    }

    running_man_update();

    gui_set_text(WIFI_I, false, wifi_get_internet_status() ? ICON_WIFI_CONNECTED : ICON_WIFI_DISCONNECTED);
    gui_set_text(DOWN_I, false, downloading_data ? ICON_DOWNLOAD : "");

    prior_screen = screen;
}

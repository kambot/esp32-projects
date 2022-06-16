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


// ====================================================================
// VARIABLES
// ====================================================================

esp_timer_handle_t timer_handle;
esp_timer_handle_t sensor_timer_handle;
task_data_t gui_task_cg = {0};
task_data_t bd_task_cg = {0};
uint8_t mac_address[6] = {0};
uint8_t bt_mac_address[6] = {0};


char* bd_data = NULL; // string (csv format)

#define BD_LIST_MAX 300
birthday_t bd_list[BD_LIST_MAX] = {0};
int bd_list_count = 0;
bool refresh_bd_rank = true;
bool bd_list_updated = false;

int yday = 0;
int year = 0;

bool downloading_data = false;
bool update_gui_date = false;
bool selection_changed = false;
int sel_idx = 0;
int sel_bd_idx = 0;

// ====================================================================
// MACROS/DEFINES
// ====================================================================

#define BD_DOWNLOAD_SECONDS  (10*60)

#define BTN_PIN 27

#define NOT_SELECTED "     "
#define SELECTED     ICON_SELECTOR " "

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
void btn_single_press();

// gui
void gui_task(void* arg);

// ====================================================================
// MAIN
// ====================================================================

void app_main()
{
    init();
    print_task_list();


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
    wifi_set_max_reconnect_attempts(10);
    wifi_set_reconnect_time(10);
    // wifi_debug_logs(true);

    wifi_load_credentials();
    wifi_set_all("STUDIO_1837_CORP", "soap4life", 3, WIFI_CONTEXT_PRIMARY);

    wifi_enable();
    wifi_connect(); // must always call this after setting wifi credentials


    gui_task_cg.task_func = gui_task;
    gui_task_cg.task_name = "gui_task";
    gui_task_cg.config.core = 0;
    gui_task_cg.config.priority = 11;
    gui_task_cg.config.stack_size = 5000;
    create_task(gui_task_cg);

    bd_task_cg.task_func = birthdays_task;
    bd_task_cg.task_name = "birthdays_task";
    bd_task_cg.config.core = 1;
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

    button_params_t btn_cfg = BUTTON_DEFAULT_PARAMS();
    button_init(btn_cfg, 1, 5);
    button_config_pin(PIN_MASK(BTN_PIN), 0);
    button_add_event_press(PIN_MASK(BTN_PIN), PRESSES, 1, btn_single_press);
    button_add_event_hold_mod(PIN_MASK(BTN_PIN), 1000, 1000000, 300, btn_single_press);
}


void birthdays_task(void* arg)
{
    LOGI("Entered birthday task");

    // wait for sntp sync
    for(;;)
    {
        if(yday != 0) break;
        delay(200);
    }

    load_bd_data();
    parse_bd_data();

    // char buf[100] = {0};
    // sprintf(buf, "%s %u/%u (%u)", bd_list[0].name, bd_list[0].month, bd_list[0].day, bd_list[0].countdown);
    // set_line_scrolling(0, buf, 0,0, 0, INF, 3, true);
    // set_line_no_scroll(1, buf, 0, 16, false);

    // set_line_textf(0, "%s test", ICON_WIFI_DISCONNECTED);


    static int64_t refresh_timer = 0;

    for(;;)
    {

        if(wifi_get_internet_status() && esp_timer_get_time() >= refresh_timer)
        {
            // if this is successful, it will also update bd_data and store to NVS partition
            downloading_data = true;
            bool ret = get_birthdays();
            downloading_data = false;

            if(ret)
            {
                refresh_timer = esp_timer_get_time() + BD_DOWNLOAD_SECONDS*1e6;
            }
            else
            {
                refresh_timer = esp_timer_get_time() + 10*1e6;
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

    rank_bd_list();
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
        print_birthday_t(b);
    }
    refresh_bd_rank = false;
    bd_list_updated = true;
}

// button

void btn_single_press()
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




// gui stuff

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


void gui_task(void* arg)
{
    LOGI("Entered gui task");

    oled_init();
    ssd1306_select_font(FONT_TAHOMA);

    set_line_no_scroll(WIFI_I, ICON_WIFI_DISCONNECTED, WIFI_X, WIFI_Y, false);
    set_line_no_scroll(DOWN_I, "", DOWN_X, DOWN_Y, false);
    set_line_no_scroll(DATE_I, "", DATE_X, DATE_Y, false);
    set_line_no_scroll(CNT_I,  "", CNT_X,  CNT_Y,  true);
    set_line_no_scroll(BDD_I,  "", BDD_X,  BDD_Y,  false);
    set_line_no_scroll(DAYS_I, "", DAYS_X, DAYS_Y, false);
    set_line_no_scroll(BD1_I,  "", BD1_X,  BD1_Y,  false);
    set_line_no_scroll(BD2_I,  "", BD2_X,  BD2_Y,  false);


    set_line_textf(CNT_I, "%2d of %-2d", 0, bd_list_count);


    for(;;)
    {


        if(bd_list_updated)
        {
            set_line_textf(CNT_I, "%2d of %-2d", bd_list_count > 0 ? 1 : 0, bd_list_count);

            sel_idx = 0;
            sel_bd_idx = 0;
            selection_changed = true;
            bd_list_updated = false;
        }

        if(selection_changed)
        {
            LOGI("sel_idx: %d, sel_bd_idx, %d", sel_idx, sel_bd_idx);

            set_line_textf(CNT_I, "%2d of %-2d", bd_list_count > 0 ? sel_bd_idx+1 : 0, bd_list_count);

            if(bd_list_count > 0)
            {
                if(sel_idx == 0)
                {
                    set_line_textf(BD1_I, SELECTED " %s", bd_list[sel_bd_idx].name);

                    if((sel_bd_idx+1) < bd_list_count)
                    {
                        set_line_textf(BD2_I, NOT_SELECTED " %s", bd_list[sel_bd_idx+1].name);
                    }
                    else
                    {
                        // at the end of the list
                        set_line_textf(BD2_I, "");
                    }

                }
                else
                {
                    set_line_textf(BD1_I, NOT_SELECTED " %s", bd_list[sel_bd_idx-1].name);
                    set_line_textf(BD2_I, SELECTED " %s", bd_list[sel_bd_idx].name);
                }

            }
            else
            {
                set_line_textf(BD1_I, "");
                set_line_textf(BD2_I, "");
            }

            set_line_textf(BDD_I, "Birthday: %u/%u", bd_list[sel_bd_idx].month, bd_list[sel_bd_idx].day);
            set_line_textf(DAYS_I, "Days: %u", bd_list[sel_bd_idx].countdown);

            selection_changed = false;
        }

        if(update_gui_date)
        {
            struct tm t = time_now_tm_local();
            set_line_textf(DATE_I, "%d/%d", t.tm_mon+1, t.tm_mday);
            update_gui_date = false;
        }

        set_line_textf(WIFI_I, wifi_get_internet_status() ? ICON_WIFI_CONNECTED : ICON_WIFI_DISCONNECTED);
        set_line_textf(DOWN_I, downloading_data ? ICON_DOWNLOAD : "");

        delay(100);
    }

    LOGI("Leaving gui task");
    vTaskDelete(NULL);
}



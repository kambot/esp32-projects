// ====================================================================
// INCLUDES
// ====================================================================

#include "common.h"
#include "https.h"

#include "esp_http_client.h"

// ====================================================================
// DEFINES
// ====================================================================

// #define URL_TEST "https://script.google.com/macros/s/AKfycbzOO7HPafFKjfDwXwZi10nLMJTA4tvg-bXGLQ-mMuQOL5-2oCPcskbkz_UiOS6VkzRz/exec?sheet_name=test&timestamp=1642594728&device=test1&mac=F8:8A:5E:3D:42:FF&temp=10.2&raw=1124&event=sample&value=10.21"
#define URL_TEST "https://script.google.com/macros/s/AKfycbzOO7HPafFKjfDwXwZi10nLMJTA4tvg-bXGLQ-mMuQOL5-2oCPcskbkz_UiOS6VkzRz/exec?sheet_name=test&timestamp=1642702169&device=living_room&mac=24:6F:28:9C:E6:54&data_type=IN_TEMP&value=72.68"

#define PUB_URL_MAX 300
#define PUB_URL_PREFIX "https://script.google.com/macros/s/AKfycbzOO7HPafFKjfDwXwZi10nLMJTA4tvg-bXGLQ-mMuQOL5-2oCPcskbkz_UiOS6VkzRz/exec?"
// sheet, timestamp, uptime, device, mac, data_type
#define PUB_URL_DATA_FMT "sheet_name=%s&timestamp=%ld&uptime=%u&device=%s&mac=" MAC_FMT "&data_type=%s"


#define WEATHER_URL "https://darksky.net/forecast/39.1087,-84.6235/us12/en"

#define WEATHER_HTML_MAX 1500

// ====================================================================
// MACROS
// ====================================================================

#define TAKE_SEMPHR() if(semphr != NULL) xSemaphoreTake(semphr, portMAX_DELAY);
#define GIVE_SEMPHR() if(semphr != NULL) xSemaphoreGive(semphr);

// ====================================================================
// STATIC VARIABLES
// ====================================================================

static esp_http_client_handle_t pub_client = NULL;
static char pub_url[PUB_URL_MAX+1] = {0};
static int data_event_count = 0;
static int got_success = false;

static esp_http_client_handle_t weather_client = NULL;
static char weather_html[WEATHER_HTML_MAX+1] = {0};
static int weather_html_len = 0;

static SemaphoreHandle_t semphr = NULL;

// ====================================================================
// GLOBAL VARIABLES
// ====================================================================

// ====================================================================
// STATIC PROTOTYPES
// ====================================================================

static esp_err_t http_weather_event_handler(esp_http_client_event_t *evt);
static esp_err_t http_pub_event_handler(esp_http_client_event_t *evt);
// static bool http_configure();

// ====================================================================
// GLOBAL FUNCTIONS
// ====================================================================

bool publish_data(data_t item)
{
    if(semphr == NULL) semphr = xSemaphoreCreateMutex();

    bool ret = false;
    esp_err_t err;

    if(pub_client != NULL)
    {
        err = esp_http_client_cleanup(pub_client);
        if(err != ESP_OK)
        {
            LOGE("Failed to cleanup http pub_client resources, error: 0x%x (%s)", err, esp_err_to_name(err));
            return false;
        }

        pub_client = NULL;
    }

    memset(pub_url, 0, sizeof(char)*PUB_URL_MAX);
    str_append(pub_url, PUB_URL_MAX, PUB_URL_PREFIX);
    str_append(pub_url, PUB_URL_MAX, PUB_URL_DATA_FMT, "test", item.timestamp, item.uptime, DEVICE_NAME, MAC_LIST(mac_address), data_type_to_str(item.data_type));

    if(item.data_type == DATA_TYPE_IN_TEMP || item.data_type == DATA_TYPE_OUT_TEMP)
    {
        str_append(pub_url, PUB_URL_MAX, "&value=%.2f", item.value_num);
    }

    LOGI("Publishing data (%ld, %u, %d, %.2f)", item.timestamp, item.uptime, item.data_type, item.value_num);

    // printf("%s\n", pub_url);

    esp_http_client_config_t config = {};
    config.url = pub_url;
    config.cert_pem = HTTPS_PUB_CERT;
    config.method = HTTP_METHOD_GET;
    config.transport_type = HTTP_TRANSPORT_OVER_SSL;
    config.event_handler = http_pub_event_handler;
    config.timeout_ms = 10*1000; // 10 seconds
    config.disable_auto_redirect = false;
    config.buffer_size = 2048;
    config.buffer_size_tx = 2048;

    if(pub_client == NULL)
    {
        pub_client = esp_http_client_init(&config);
    }

    if(pub_client == NULL)
    {
        LOGE("Failed to init http pub_client");
        return false;
    }

    data_event_count = 0;
    got_success = false;

    TAKE_SEMPHR();
    err = esp_http_client_perform(pub_client);

    int sc = esp_http_client_get_status_code(pub_client);
    // if(sc == 200 && err == ESP_OK)
    if(got_success)
    {
        LOGI("Publish succeeded");
        ret = true;
    }
    else
    {
        LOGE("Failed, status code: %d, pub_client perform error: 0x%x (%s)", sc, err, esp_err_to_name(err));
        ret = false;
    }

    esp_http_client_close(pub_client);

    GIVE_SEMPHR();
    return ret;
}

bool get_out_temp(float* out_temp)
{
    if(semphr == NULL) semphr = xSemaphoreCreateMutex();

    bool ret = false;
    esp_err_t err;

    if(weather_client != NULL)
    {
        err = esp_http_client_cleanup(weather_client);
        if(err != ESP_OK)
        {
            LOGE("Failed to cleanup http weather_client resources, error: 0x%x (%s)", err, esp_err_to_name(err));
            return false;
        }

        weather_client = NULL;
    }

    memset(weather_html, 0, WEATHER_HTML_MAX*sizeof(char));
    weather_html_len = 0;

    esp_http_client_config_t config = {};
    config.url = WEATHER_URL;
    config.cert_pem = HTTPS_WEATHER_CERT;
    config.method = HTTP_METHOD_GET;
    config.transport_type = HTTP_TRANSPORT_OVER_SSL;
    config.event_handler = http_weather_event_handler;
    config.timeout_ms = 10*1000; // 10 seconds
    config.disable_auto_redirect = false;
    config.buffer_size = 2048;
    config.buffer_size_tx = 2048;

    if(weather_client == NULL)
    {
        weather_client = esp_http_client_init(&config);
    }

    if(weather_client == NULL)
    {
        LOGE("Failed to init http weather_client");
        return false;
    }

    TAKE_SEMPHR();
    err = esp_http_client_perform(weather_client);

    int sc = esp_http_client_get_status_code(weather_client);
    if(sc == 200 && err == ESP_OK)
    {

        char* check1 = strstr(weather_html,  "currently = {\"");
        char* check2 = strstr(weather_html, "\"temperature\":");

        if(!check1 || !check2)
        {
            LOGE("Weather failed");
            ret = false;
        }
        else
        {

            char* p = check2+14;
            char temp_buf[10] = {0};
            int buf_idx = 0;
            for(;;)
            {
                if(*p == ',') break;
                if(*p == 0) break;
                if(*p != ' ')
                {
                    temp_buf[buf_idx++] = *p;
                }
                p++;
            }

            // printf("temp_buf: %s\n", temp_buf);
            float current_temp = atof(temp_buf);
            // printf("current_temp: %.2f\n", current_temp);
            *out_temp = current_temp;
            ret = true;

        }


    }
    else
    {
        LOGE("Failed, status code: %d, weather_client perform error: 0x%x (%s)", sc, err, esp_err_to_name(err));
        ret = false;
    }

    esp_http_client_close(weather_client);

    GIVE_SEMPHR();
    return ret;
}

// ====================================================================
// STATIC FUNCTIONS
// ====================================================================


static esp_err_t http_pub_event_handler(esp_http_client_event_t *evt)
{

    switch(evt->event_id)
    {
        case HTTP_EVENT_ERROR:
        {
            LOGE("HTTP_EVENT_ERROR");
        } break;

        case HTTP_EVENT_ON_CONNECTED:
        {
            // LOGI("HTTP_EVENT_ON_CONNECTED");
        } break;

        case HTTP_EVENT_HEADER_SENT:
        {
            // LOGI("HTTP_EVENT_HEADER_SENT");
        } break;

        case HTTP_EVENT_ON_HEADER:
        {
            // LOGI("HTTP_EVENT_ON_HEADER");
            // printf("%s %s\n", evt->header_key, evt->header_value);
        } break;

        case HTTP_EVENT_ON_DATA:
        {
            data_event_count++;
            if(data_event_count == 2)
            {
                // LOGI("HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
                if(evt->data_len != 0) LOGI("Publish result: %.*s", evt->data_len, (char*)evt->data);

                if(evt->data_len == 7)
                {
                    if(memcmp("success",evt->data, 7) == 0)
                        got_success = true;
                }
            }

        } break;

        case HTTP_EVENT_ON_FINISH:
        {
            // LOGI("HTTP_EVENT_ON_FINISH");
        } break;

        case HTTP_EVENT_DISCONNECTED:
        {
            // LOGI("HTTP_EVENT_DISCONNECTED");
        } break;

        default:
        {
            LOGI("event: %d", evt->event_id);
        } break;

    }
    return ESP_OK;
}


static esp_err_t http_weather_event_handler(esp_http_client_event_t *evt)
{

    switch(evt->event_id)
    {
        case HTTP_EVENT_ERROR:
        {
            LOGE("HTTP_EVENT_ERROR");
        } break;

        case HTTP_EVENT_ON_CONNECTED:
        {
            // LOGI("HTTP_EVENT_ON_CONNECTED");
        } break;

        case HTTP_EVENT_HEADER_SENT:
        {
            // LOGI("HTTP_EVENT_HEADER_SENT");
        } break;

        case HTTP_EVENT_ON_HEADER:
        {
            // LOGI("HTTP_EVENT_ON_HEADER");
            // printf("%s %s\n", evt->header_key, evt->header_value);
        } break;

        case HTTP_EVENT_ON_DATA:
        {
            int len = evt->data_len;
            // LOGI("HTTP_EVENT_ON_DATA, len=%d", len);

            // always save the last WEATHER_HTML_MAX characters of data
            if(len > WEATHER_HTML_MAX)
            {
                int copy_idx = len - WEATHER_HTML_MAX;
                memcpy(weather_html, evt->data + copy_idx, WEATHER_HTML_MAX*sizeof(char));
                weather_html_len = WEATHER_HTML_MAX;
                // printf("copied last WEATHER_HTML_MAX of new data\n");
            }
            else if((len+weather_html_len) <= WEATHER_HTML_MAX)
            {
                // printf("appending data at %d\n", weather_html_len);
                memcpy(weather_html+weather_html_len, evt->data, len*sizeof(char));
                weather_html_len += len;
                // printf("weather_html_len = %d, strlen = %d\n", weather_html_len, strlen(weather_html));
            }
            else
            {
                int from_prev_copy_len = MIN(WEATHER_HTML_MAX - len, weather_html_len);
                for(int i = 0; i < from_prev_copy_len; ++i)
                {
                    weather_html[i] = weather_html[i+(weather_html_len-from_prev_copy_len)];
                }
                memset(weather_html+from_prev_copy_len, 0, (WEATHER_HTML_MAX-from_prev_copy_len)*sizeof(char));

                memcpy(weather_html+from_prev_copy_len, evt->data, len*sizeof(char));
                weather_html_len = len+from_prev_copy_len;

                // printf("from_prev_copy_len = %d, weather_html_len = %d, strlen = %d\n", from_prev_copy_len, weather_html_len, strlen(weather_html));

            }

        } break;

        case HTTP_EVENT_ON_FINISH:
        {
            // LOGI("HTTP_EVENT_ON_FINISH");
        } break;

        case HTTP_EVENT_DISCONNECTED:
        {
            // LOGI("HTTP_EVENT_DISCONNECTED");
        } break;

        default:
        {
            LOGI("event: %d", evt->event_id);
        } break;

    }
    return ESP_OK;
}
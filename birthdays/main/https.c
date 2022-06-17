// ====================================================================
// INCLUDES
// ====================================================================

#include "common.h"
#include "https.h"

#include "esp_http_client.h"

// ====================================================================
// DEFINES
// ====================================================================

#define URL "https://script.google.com/macros/s/AKfycbzec30qonlzTAWnisb_CLsjdsq7-jEdejkrTdgzmluW48tU3IENSdARUbbUdGcPXLKm/exec"

#define BUFFER_MAX_LEN  6000

// ====================================================================
// MACROS
// ====================================================================


// ====================================================================
// STATIC VARIABLES
// ====================================================================

static esp_http_client_handle_t client = NULL;
static int data_event_count = 0;
static bool data_success = false;

static char download_buffer[BUFFER_MAX_LEN] = {0};
static int download_buffer_index = 0;

// ====================================================================
// GLOBAL VARIABLES
// ====================================================================

// ====================================================================
// STATIC PROTOTYPES
// ====================================================================

static esp_err_t http_event_handler(esp_http_client_event_t* evt);

// ====================================================================
// GLOBAL FUNCTIONS
// ====================================================================

bool get_birthdays()
{

    esp_err_t err;

    if(client != NULL)
    {
        err = esp_http_client_cleanup(client);
        if(err != ESP_OK)
        {
            LOGE("Failed to cleanup http client resources, error: 0x%x (%s)", err, esp_err_to_name(err));
            return false;
        }

        client = NULL;
    }

    // printf("%s\n", pub_url);

    esp_http_client_config_t config = {};
    config.url = URL;
    // config.url = URL_TEST_ERROR;
    config.cert_pem = HTTPS_BD_CERT;
    config.method = HTTP_METHOD_GET;
    config.transport_type = HTTP_TRANSPORT_OVER_SSL;
    config.event_handler = http_event_handler;
    config.timeout_ms = 20*1000; // 20 seconds
    config.disable_auto_redirect = false;
    config.buffer_size = 4000;
    config.buffer_size_tx = 4000;

    if(client == NULL)
    {
        client = esp_http_client_init(&config);
    }

    if(client == NULL)
    {
        LOGE("Failed to init http client");
        return false;
    }

    data_event_count = 0;
    data_success = false;
    memset(download_buffer, 0, BUFFER_MAX_LEN);
    download_buffer_index = 0;
    
    esp_log_level_set("HTTP_CLIENT", ESP_LOG_NONE);
    LOGI("client performing");
    err = esp_http_client_perform(client);
    LOGI("client finished performing");
    esp_http_client_close(client);
    esp_log_level_set("HTTP_CLIENT", ESP_LOG_ERROR);

    LOGI("data success: %s", BOOLSTR(data_success));

    if(!data_success) return false;

    // printf("%s\n", download_buffer);

    char* data = decrypt_data(download_buffer);
    if(data == NULL)
    {
        LOGE("Failed to decrypt data");
        return false;
    }

    if(data != NULL)
    {
        // printf("%s\n", data);
    }

    bool new_data = false;
    if(bd_data != NULL)
    {
        if(!STR_EQUAL(data, bd_data))
        {
            new_data = true;
        }
    }
    else
    {
        new_data = true;
    }

    if(new_data)
    {
        if(bd_data) free(bd_data);
        bd_data = calloc(strlen(data)+1, sizeof(char));
        memcpy(bd_data, data, strlen(data));

        printf("%s\n", bd_data);

        save_bd_data();
        parse_bd_data();
        refresh_bd_rank = true; // trigger sorting
    }

    free(data);

    printf("Free Heap:  %d\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));

    return true;
}


// ====================================================================
// STATIC FUNCTIONS
// ====================================================================


static esp_err_t http_event_handler(esp_http_client_event_t* evt)
{

    switch(evt->event_id)
    {
        case HTTP_EVENT_ERROR:
        {
            LOGE("ERROR");
        } break;

        case HTTP_EVENT_ON_CONNECTED:
        {
            // LOGI("Connected");
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
            LOGI("DATA, len: %d", evt->data_len);
            data_event_count++;

            if(evt->data_len > 0 && data_event_count == 1)
            {
                // redirect
                char* check = strstr((char*)evt->data, "<TITLE>Moved Temporarily</TITLE>");
                if(check != NULL)
                    break;
            }

            if(data_event_count >= 2)
            {
                // if(evt->data_len != 0) LOGI("%.*s", evt->data_len, (char*)evt->data);
                memcpy(download_buffer+download_buffer_index, evt->data, evt->data_len);
                download_buffer_index += evt->data_len;
                data_success = true; //assumes we'll get ALL of the data before being disconnected
            }

        } break;

        case HTTP_EVENT_ON_FINISH:
        {
            LOGI("Finished");
        } break;

        case HTTP_EVENT_DISCONNECTED:
        {
            // LOGI("DISCONNECTED");
        } break;

        default:
        {
            LOGI("event: %d", evt->event_id);
        } break;

    }
    return ESP_OK;
}


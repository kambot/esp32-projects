// ====================================================================
// INCLUDES 
// ====================================================================

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <inttypes.h>

#include "nvs.h"
#include "nvs_flash.h"

#include "common.h"
#include "storage.h"

// ====================================================================
// STATIC PROTOTYPES
// ====================================================================

// ====================================================================
// STATIC VARIABLES
// ====================================================================


// ====================================================================
// MACROS
// ====================================================================


#define Create_NvsWriteFunc(type,name) \
    bool store_nvs_write_##name(const char* name_space, const char* label, type value) \
    { \
        esp_err_t err; \
        nvs_handle handle; \
        err = nvs_open(name_space, NVS_READWRITE, &handle); \
        if(err != ESP_OK) { \
            LOGE("[NVS] Failed to open handle, name_space: %s, error: 0x%x (%s)", name_space, err, esp_err_to_name(err)); \
            return false; \
        } \
        err = nvs_set_##name(handle,label,value); \
        if(err != ESP_OK) { \
            LOGE("[NVS] Failed to set "#type", key: %s, error: 0x%x (%s)", label, err, esp_err_to_name(err)); \
            nvs_close(handle); \
            return false; \
        } \
        err = nvs_commit(handle); \
        if(err != ESP_OK) { \
            LOGE("[NVS] Failed to commit "#type" data, key: %s, error: 0x%x (%s)", label, err, esp_err_to_name(err)); \
            nvs_close(handle); \
            return false; \
        } \
        nvs_close(handle); \
        return true; \
    }

#define Create_NvsReadFunc(type,name) \
    bool store_nvs_read_##name(const char* name_space, const char* label, type *value) \
    { \
        esp_err_t err; \
        nvs_handle handle; \
        err = nvs_open(name_space, NVS_READONLY, &handle); \
        if(err != ESP_OK) { \
            LOGW("[NVS] Failed to open handle, name_space: %s, error: 0x%x (%s)", name_space, err, esp_err_to_name(err)); \
            return false; \
        } \
        err = nvs_get_##name(handle,label,value); \
        if(err == ESP_ERR_NVS_NOT_FOUND) { \
            LOGW("[NVS] Failed to get "#type" because it does not exist, key: %s", label); \
            nvs_close(handle); \
            return false; \
        } else if(err != ESP_OK) { \
            LOGE("[NVS] Failed to get "#type", key: %s, error: 0x%x (%s)", label, err, esp_err_to_name(err)); \
            nvs_close(handle); return false; \
        } \
        nvs_close(handle); \
        return true; \
    }




// ====================================================================
// GLOBAL FUNCTIONS
// ====================================================================

bool storage_erase_all()
{
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);
    while(it != NULL)
    {
        const esp_partition_t* part = esp_partition_get(it);
        LOGI("[NVS] Erasing data: %s", part->label);

        nvs_iterator_t it2 = nvs_entry_find(part->label, NULL, NVS_TYPE_ANY);
        while(it2 != NULL)
        {
            nvs_entry_info_t info;
            nvs_entry_info(it2, &info);
            it2 = nvs_entry_next(it2);

            if(strcmp("cal_data",info.key) == 0)
                continue;
            if(strcmp("cal_mac",info.key) == 0)
                continue;
            if(strcmp("cal_version",info.key) == 0)
                continue;
            if(strcmp("bt_config.conf",info.namespace_name) == 0)
                continue;

            store_nvs_erase_key(info.namespace_name, info.key);
            delay(100); // delay just to give the esp_timer (handles top LEDs) some time to process
        }
        nvs_release_iterator(it2);
        it = esp_partition_next(it);
    }
    esp_partition_iterator_release(it);

    return true;
}



// ====================================================================
// NVS
// ====================================================================

bool store_nvs_init()
{
    static bool is_nvs_init = false;
    if(is_nvs_init) return true;

    esp_err_t err = nvs_flash_init();

    if(err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    if(err != ESP_OK) {
        LOGE("Failed to initialize NVS, error: 0x%x (%s)", err, esp_err_to_name(err));
        return false;
    } else {
        is_nvs_init = true;
    }
    return true;
}


bool store_nvs_key_exists(const char* partition, const char* label, const char* name_space)
{
#if IDF_VERSION == 4
    bool match = false;
    nvs_iterator_t it = nvs_entry_find(partition, name_space, NVS_TYPE_ANY);
    while(it != NULL)
    {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        it = nvs_entry_next(it);
        if(strncmp(label, info.key, strlen(label)) == 0) {
            match = true;
            break;
        }
    }
    nvs_release_iterator(it);
    return match;
#else
    nvs_handle handle;
    esp_err_t err = nvs_open(name_space, NVS_READONLY, &handle);
    if(err != ESP_OK) {
        return false;
    }

    #define CHECK_NVS_KEY(handle,label,type,name) {type val = 0; if(nvs_get_##name(handle,label,&val) == ESP_OK) goto success;}

    CHECK_NVS_KEY(handle,label,int8_t, i8);
    CHECK_NVS_KEY(handle,label,uint8_t, u8);
    CHECK_NVS_KEY(handle,label,int16_t, i16);
    CHECK_NVS_KEY(handle,label,uint16_t, u16);
    CHECK_NVS_KEY(handle,label,int32_t, i32);
    CHECK_NVS_KEY(handle,label,uint32_t, u32);
    CHECK_NVS_KEY(handle,label,int64_t, i64);
    CHECK_NVS_KEY(handle,label,uint64_t, u64);

    size_t size = 0;
    err = nvs_get_str(handle,label,NULL,&size);
    if(err == ESP_OK) goto success;

    err = nvs_get_blob(handle,label,NULL,&size);
    if(err == ESP_OK) goto success;

    nvs_close(handle);
    return false;

success:
    nvs_close(handle);
    return true;
#endif
}

void store_nvs_print_keys(const char* partition, const char* name_space)
{
#if IDF_VERSION == 4
    LOGI("[NVS] Listing Entries, partition: %s, name_space: %s", partition, (name_space == NULL ? "*" : name_space));

    nvs_iterator_t it = nvs_entry_find(partition, NULL, NVS_TYPE_ANY);
    int c = 0;
    while(it != NULL)
    {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        it = nvs_entry_next(it);
        LOGI("[NVS] %d) key: %-15s, type: 0x%02x, name_space: %s", c++, info.key, info.type, info.namespace_name);
    }
    nvs_release_iterator(it);
#else

#endif
}

void store_nvs_get_stats(const char* partition, nvs_stats_t* stats)
{
    nvs_stats_t nvs_stats = {};
    nvs_get_stats(partition, &nvs_stats);
    LOGI("[NVS] Partion: %s, Used: %d, Free: %d, Total: %d", partition, nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries);
    if(stats != NULL) memcpy(stats, &nvs_stats, sizeof(nvs_stats));
}

bool store_nvs_erase_key(const char* name_space, const char* label)
{
    nvs_handle handle;
    esp_err_t err = nvs_open(name_space, NVS_READWRITE, &handle);

    if(err != ESP_OK) {
        LOGW("[NVS] Failed to open handle, name_space: %s, error: 0x%x (%s)", name_space, err, esp_err_to_name(err));
        return false;
    }
    err = nvs_erase_key(handle, (const char*)label);
    if(err == ESP_ERR_NVS_NOT_FOUND) {
        LOGW("[NVS] Failed to erase because it does not exist, key: %s", label);
        return false;
    } if(err != ESP_OK) {
        LOGE("[NVS] Error erasing data, key: %s, error: 0x%x (%s)", label, err, esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    err = nvs_commit(handle);
    if(err != ESP_OK) {
        LOGE("[NVS] Error committing erase, key: %s, error: 0x%x (%s)", label, err, esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    nvs_close(handle);
    return true;
}

bool store_nvs_erase_all(const char* name_space)
{
    nvs_handle handle;
    esp_err_t err = nvs_open(name_space, NVS_READWRITE, &handle);

    if(err != ESP_OK) {
        LOGE("[NVS] Failed to open handle, name_space: %s, error: 0x%x (%s)", name_space, err, esp_err_to_name(err));
        return false;
    }

    err = nvs_erase_all(handle);
    if(err != ESP_OK) {
        LOGE("[NVS] Error erasing all, name_space: %s, error: 0x%x (%s)", name_space, err, esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    err = nvs_commit(handle);
    if(err != ESP_OK) {
        LOGE("[NVS] Error committing erase all, name_space: %s, error: 0x%x (%s)", name_space, err, esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    nvs_close(handle);
    return true;
}

Create_NvsReadFunc(int8_t,i8)
Create_NvsReadFunc(uint8_t,u8)
Create_NvsReadFunc(int16_t,i16)
Create_NvsReadFunc(uint16_t,u16)
Create_NvsReadFunc(int32_t,i32)
Create_NvsReadFunc(uint32_t,u32)
Create_NvsReadFunc(int64_t,i64)
Create_NvsReadFunc(uint64_t,u64)
Create_NvsWriteFunc(int8_t,i8)
Create_NvsWriteFunc(uint8_t,u8)
Create_NvsWriteFunc(int16_t,i16)
Create_NvsWriteFunc(uint16_t,u16)
Create_NvsWriteFunc(int32_t,i32)
Create_NvsWriteFunc(uint32_t,u32)
Create_NvsWriteFunc(int64_t,i64)
Create_NvsWriteFunc(uint64_t,u64)

bool store_nvs_read_str(const char* name_space, const char* label, char* value, size_t* size) 
{
    esp_err_t err;
    nvs_handle handle;

    err = nvs_open(name_space, NVS_READONLY, &handle);
    if(err != ESP_OK) {
        LOGW("[NVS] Failed to open handle, name_space: %s, error: 0x%x (%s)", name_space, err, esp_err_to_name(err));
        return false;
    }

    err = nvs_get_str(handle,label,NULL,size);
    if(err == ESP_ERR_NVS_NOT_FOUND) {
        LOGW("[NVS] Failed to get str because it does not exist, key: %s", label);
        return false;
    } else if(err != ESP_OK) {
        LOGE("[NVS] Failed to get str size, key: %s, error: 0x%x (%s)", label, err, esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    if(value != NULL) {
        err = nvs_get_str(handle,label,value,size);
        if(err != ESP_OK) {
            LOGE("[NVS] Failed to get str, key: %s, error: 0x%x (%s)", label, err, esp_err_to_name(err));
            nvs_close(handle);
            return false;
        }
    }

    nvs_close(handle);
    return true;
}

bool store_nvs_read_blob(const char* name_space, const char* label, void *value, size_t *size)
{
    esp_err_t err;
    nvs_handle handle;

    err = nvs_open(name_space, NVS_READONLY, &handle);
    if(err != ESP_OK) {
        LOGW("[NVS] Failed to open handle, name_space: %s, error: 0x%x (%s)", name_space, err, esp_err_to_name(err));
        return false;
    }

    err = nvs_get_blob(handle,label,NULL,size);
    if(err == ESP_ERR_NVS_NOT_FOUND) {
        LOGW("[NVS] Failed to get blob because it does not exist, key: %s", label);
        return false;
    } else if(err != ESP_OK) {
        LOGE("[NVS] Failed to get blob size, key: %s, error: 0x%x (%s)", label, err, esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    if(value != NULL) {
        err = nvs_get_blob(handle,label,value,size);
        if(err != ESP_OK) {
            LOGE("[NVS] Failed to get blob, key: %s, error: 0x%x (%s)", label, err, esp_err_to_name(err));
            nvs_close(handle);
            return false;
        }
    }

    nvs_close(handle);
    return true;
}


bool store_nvs_write_str(const char* name_space, const char* label, char* value)
{
    esp_err_t err;
    nvs_handle handle;

    err = nvs_open(name_space, NVS_READWRITE, &handle);
    if(err != ESP_OK) {
        LOGE("[NVS] Failed to open handle, name_space: %s, error: 0x%x (%s)", name_space, err, esp_err_to_name(err));
        return false;
    }

    err = nvs_set_str(handle,label,value);
    if(err != ESP_OK) {
        LOGE("[NVS] Failed to set str, key: %s, error: 0x%x (%s)", label, err, esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    err = nvs_commit(handle);
    if(err != ESP_OK) {
        LOGE("[NVS] Failed to commit str data, key: %s, error: 0x%x (%s)", label, err, esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    nvs_close(handle);
    return true;
}

bool store_nvs_write_blob(const char* name_space, const char *label, void *value, size_t size)
{
    esp_err_t err;
    nvs_handle handle;

    err = nvs_open(name_space, NVS_READWRITE, &handle);
    if(err != ESP_OK) {
        LOGE("[NVS] Failed to open handle, name_space: %s, error: 0x%x (%s)", name_space, err, esp_err_to_name(err));
        return false;
    }

    err = nvs_set_blob(handle,label,value,size);
    if(err != ESP_OK) {
        LOGE("[NVS] Failed to set blob, key: %s, error: 0x%x (%s)", label, err, esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    err = nvs_commit(handle);
    if(err != ESP_OK) {
        LOGE("[NVS] Failed to commit blob data, key: %s, error: 0x%x (%s)", label, err, esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    nvs_close(handle);
    return true;
}

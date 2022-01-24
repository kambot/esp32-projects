#pragma once

// ====================================================================
// INCLUDES 
// ====================================================================

#include "common.h"

#include "nvs.h"
#include "esp_partition.h"


// ====================================================================
// GLOBAL FUNCTIONS 
// ====================================================================


// NVS
bool store_nvs_init();
bool store_nvs_key_exists(const char* partition, const char* label, const char* name_space); //NULL for namespace is OK
void store_nvs_print_keys(const char* partition, const char* name_space); //NULL for namespace is OK
void store_nvs_get_stats(const char* partition, nvs_stats_t* stats);
bool store_nvs_erase_key(const char* name_space, const char* label);
bool store_nvs_erase_all(const char* name_space);
bool store_nvs_read_i8(const char* name_space, const char* label, int8_t *value);
bool store_nvs_read_u8(const char* name_space, const char* label, uint8_t *value);
bool store_nvs_read_i16(const char* name_space, const char* label, int16_t *value);
bool store_nvs_read_u16(const char* name_space, const char* label, uint16_t *value);
bool store_nvs_read_i32(const char* name_space, const char* label, int32_t *value);
bool store_nvs_read_u32(const char* name_space, const char* label, uint32_t *value);
bool store_nvs_read_i64(const char* name_space, const char* label, int64_t *value);
bool store_nvs_read_u64(const char* name_space, const char* label, uint64_t *value);
bool store_nvs_read_str(const char* name_space, const char* label, char* value, size_t* size);
bool store_nvs_read_blob(const char* name_space, const char* label, void * value, size_t *size);
bool store_nvs_write_i8(const char* name_space, const char* label, int8_t value);
bool store_nvs_write_u8(const char* name_space, const char* label, uint8_t value);
bool store_nvs_write_i16(const char* name_space, const char* label, int16_t value);
bool store_nvs_write_u16(const char* name_space, const char* label, uint16_t value);
bool store_nvs_write_i32(const char* name_space, const char* label, int32_t value);
bool store_nvs_write_u32(const char* name_space, const char* label, uint32_t value);
bool store_nvs_write_i64(const char* name_space, const char* label, int64_t value);
bool store_nvs_write_u64(const char* name_space, const char* label, uint64_t value);
bool store_nvs_write_str(const char* name_space, const char* label, char* value);
bool store_nvs_write_blob(const char* name_space, const char* label, void *value, size_t size);

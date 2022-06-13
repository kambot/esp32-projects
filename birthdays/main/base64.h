#include "common.h"

char base64_get_char(uint8_t value);
uint8_t base64_get_value(char base64);
int base64_det_strlen(int num_bytes);
int base64_det_num_bytes(char* base64_str);
void base64_convert_to_str(uint8_t* bytes, int num_bytes, char* ret_str);
void base64_convert_to_bytes(char* base64_str, uint8_t* ret_bytes);

#include "base64.h"

char base64_get_char(uint8_t value)
{
    if(value <= 25) return ('A' + value);
    if(value >= 26 && value <= 51) return ('a' - 26 + value);
    if(value >= 52 && value <= 61) return ('0' - 52 + value);
    if(value == 62) return '+';
    if(value == 63) return '/';
    return '!';
}


uint8_t base64_get_value(char base64)
{
    if(base64 >= 'A' && base64 <= 'Z') return (base64 - 'A');
    if(base64 >= 'a' && base64 <= 'z') return (base64 - ('a' - 26));
    if(base64 >= '0' && base64 <= '9') return (base64 - ('0' - 52));
    if(base64 == '+') return 62;
    if(base64 == '/') return 63;
    return 0;
}


int base64_det_strlen(int num_bytes)
{
    int groups = num_bytes / 3;
    if(groups * 3 != num_bytes) groups++;
    return groups * 4;
}


int base64_det_num_bytes(char* base64_str)
{
    int pads = 0;
    for(int i = 0; i < strlen(base64_str); ++i)
        if(base64_str[i] == '=') pads++;

    int fours = strlen(base64_str) / 4;
    fours = pads == 0 ? fours : fours-1;
    int total_bytes = fours * 3;
    if(pads == 1) total_bytes += 2;
    if(pads == 2) total_bytes += 1;
    return total_bytes;
}


void base64_convert_to_str(uint8_t* bytes, int num_bytes, char* ret_str)
{
    int total_bits = 8 * num_bytes;
    int groups = num_bytes / 3; // groups of 3 bytes
    int remainder_bits = total_bits - groups * 24;

    int str_index = 0;
    int data_index = 0;
    // for each group of 3 bytes
    for(int i = 0; i < groups; ++i) {
        uint32_t b = 0; // the concatenated bytes
        b |= ( bytes[data_index++] << 16);
        b |= ( bytes[data_index++] << 8 );
        b |= ( bytes[data_index++] );

        ret_str[str_index++] = base64_get_char( (b >> 18) & 0x3F );
        ret_str[str_index++] = base64_get_char( (b >> 12) & 0x3F );
        ret_str[str_index++] = base64_get_char( (b >> 6 ) & 0x3F );
        ret_str[str_index++] = base64_get_char( (b      ) & 0x3F );
    }

    if(remainder_bits != 0) {
        uint32_t b = 0;
        b |= (bytes[data_index++] << 16);
        if(data_index < num_bytes) b |= (bytes[data_index++] << 8 );
        if(data_index < num_bytes) b |= (bytes[data_index++] );

        ret_str[str_index++] = base64_get_char( (b >> 18) & 0x3F );
        ret_str[str_index++] = base64_get_char( (b >> 12) & 0x3F );
        ret_str[str_index++] = base64_get_char( (b >> 6 ) & 0x3F );
        ret_str[str_index++] = base64_get_char( (b      ) & 0x3F );

        int padding = (24 - remainder_bits) / 6;
        for(int j = 0; j < padding; ++j) {
            ret_str[str_index - j - 1] = '=';
        }
    }

}


void base64_convert_to_bytes(char* base64_str, uint8_t* ret_bytes)
{
    if(strlen(base64_str) == 0) return;

    int str_index = 0;
    int data_index = 0;
    int pads = 0;
    for(int i = 0; i < (strlen(base64_str) / 4); ++i) {
        uint32_t b = 0;
        for(int j = 0; j < 4; ++j) {
            char base64 = base64_str[str_index++];
            if(base64 == '=') {pads++; continue;}
            uint32_t sixbits = base64_get_value(base64);
            b |= (sixbits << (6 * ((4-1) - j)));
        }

        for(int j = 0; j < MIN(3, 3 - pads); ++j) {
            uint8_t byte = ( b >> (8 * ((3-1) - j)) ) & 0xFF;
            ret_bytes[data_index++] = byte;
        }
    }
    return;
}
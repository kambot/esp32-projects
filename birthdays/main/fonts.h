#pragma once

#include "common.h"

#define FONT_GLCD         0
#define FONT_TAHOMA       1

#define  ICON_WIFI_DISCONNECTED    "\x7f"
#define  ICON_WIFI_CONNECTED       "\x80"
#define  ICON_IOT_DISCONNECTED     "\x81"
#define  ICON_IOT_CONNECTED        "\x82"
#define  ICON_HOME                 "\x83"
#define  ICON_GEAR                 "\x84"
#define  ICON_INFO                 "\x85"
#define  ICON_EXCLAMATION          "\x86"
#define  ICON_QUESTION             "\x87"
#define  ICON_DOWNLOAD             "\x88"
#define  ICON_SELECTOR             "\x89"
#define  ICON_BLUETOOTH            "\x8a"
#define  ICON_BLUETOOTH_INVERTED   "\x8b"
#define  ICON_EMPTY_RECT           "\x8d"
#define  ICON_FULL_RECT            "\x8e"

//! @brief Character descriptor
typedef struct _font_char_desc
{
    uint8_t width;      //!< Character width in pixel
    uint16_t offset;    //!< Offset of this character in bitmap
} font_char_desc_t;


//! @brief Font information
typedef struct _font_info
{
    uint8_t height;         //!< Character height in pixel, all characters have same height
    uint8_t c;              //!< Simulation of "C" width in TrueType term, the space between adjacent characters
    char char_start;        //!< First character
    char char_end;          //!< Last character
    const font_char_desc_t* char_descriptors; //! descriptor for each character
    const uint8_t *bitmap;  //!< Character bitmap
} font_info_t;

#define NUM_FONTS 2    //!< Number of built-in fonts

extern const font_info_t * fonts[NUM_FONTS];  //!< Built-in fonts

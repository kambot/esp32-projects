#pragma once

#include "common.h"
#include "oled.h"


typedef void (*gui_func)();


#define LINE_MAX_LEN 100


typedef enum
{
    GUI_SCREEN_NONE = 0,
    GUI_SCREEN_1 = 1<<0,
    GUI_SCREEN_2 = 1<<1,
    GUI_SCREEN_3 = 1<<2,
    GUI_SCREEN_4 = 1<<3,
    GUI_SCREEN_5 = 1<<4,
    GUI_SCREEN_6 = 1<<5,
    GUI_SCREEN_7 = 1<<6,
    GUI_SCREEN_8 = 1<<7
} gui_screen_t;


typedef struct 
{
    uint8_t screen;

    int x;
    int y;

    bool centered;
    int xu;
    int yu;

    char text[LINE_MAX_LEN+1];
    uint8_t len; // don't need to set

    ssd1306_color_t fg;
    ssd1306_color_t bg;

    bool update; // don't need to set
} gui_item_t;


void gui_init(gui_func updater, gui_func drawer);
void gui_set_screen(gui_screen_t screen);
gui_screen_t gui_get_screen();
void gui_reset_all_items();
void gui_reset_item(int index);
void gui_update_item(int index);
void gui_set_item(int index, uint8_t screen, int x, int y, bool centered, char* format, ...);
void gui_set_text(int index, char* format, ...);


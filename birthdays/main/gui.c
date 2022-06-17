#include "gui.h"
#include "oled.h"
#include "fonts.h"


static void gui_task(void* arg);


#define MAX_ITEMS   20

static gui_item_t items[MAX_ITEMS] = {0};
static gui_func gui_updater = NULL;
static gui_func gui_drawer = NULL;
static gui_screen_t active_screen = GUI_SCREEN_NONE;

void gui_init(gui_func updater, gui_func drawer)
{
    ssd1306_init();
    ssd1306_select_font(FONT_TAHOMA);
    gui_updater = updater;
    gui_drawer = drawer;

    gui_task_cfg.task_func = gui_task;
    gui_task_cfg.task_name = "gui_task";
    gui_task_cfg.config.core = 0;
    gui_task_cfg.config.priority = 11;
    gui_task_cfg.config.stack_size = 5000;
    create_task(gui_task_cfg);
}

void gui_set_screen(gui_screen_t screen)
{
    if(active_screen != screen)
    {
        LOGI("Screen %d -> %d", active_screen, screen);
    }

    for(int i = 0; i < MAX_ITEMS; ++i)
    {
        gui_item_t* g = &items[i];
        if(is_bit_set(g->screen, screen))
        {
            g->update = true;
        }
    }

    active_screen = screen;
}

gui_screen_t gui_get_screen()
{
    return active_screen;
}

void gui_reset_all_items()
{
    for(int i = 0; i < MAX_ITEMS; ++i)
        gui_reset_item(i);
}

void gui_reset_item(int index)
{
    gui_item_t* g = &items[index];
    g->screen = GUI_SCREEN_NONE;
    g->x = 0;
    g->y = 0;
    g->centered = false;
    g->xu = 0;
    g->yu = 0;
    memset(g->text,0,LINE_MAX_LEN);
    g->len = 0;
    g->fg = SSD1306_COLOR_WHITE;
    g->bg = SSD1306_COLOR_BLACK;
    g->update = true;
}

void gui_update_item(int index)
{
    items[index].update = true;
}

void gui_set_item(int index, uint8_t screen, int x, int y, bool centered, char* format, ...)
{
    gui_reset_item(index);

    gui_item_t* g = &items[index];
    g->screen = screen;
    g->update = false;

    char* str = NULL;
    va_list args;
    va_start(args, format);
    int len = vasprintf(&str, format, args);
    va_end(args);

    if(len == -1)
    {
        LOGE("Failed to format string");
        return;
    }

    memcpy(g->text, str, MIN(len, LINE_MAX_LEN));
    free(str);

    g->len = ssd1306_measure_string(g->text);
    g->xu = x;
    g->yu = y;
    g->centered = centered;
    g->y = y;
    if(g->centered)
    {
        g->x = g->xu - g->len/2;
    }
    else
    {
        g->x = x;
    }
    
    g->fg = SSD1306_COLOR_WHITE;
    g->bg = SSD1306_COLOR_BLACK;
    g->update = true;
}

void gui_set_text(int index, char* format, ...)
{
    gui_item_t* g = &items[index];


    char* str = NULL;
    va_list args;
    va_start(args, format);
    int len = vasprintf(&str, format, args);
    va_end(args);

    if(len == -1)
    {
        LOGE("Failed to format string");
        return;
    }

    int nlen = ssd1306_measure_string(str);
    if(g->len == nlen)
    {
        // if(index == 1){
        //      printf("same length: %d\n", STR_EQUAL(g->text, str));
        //      LOGI_HEX(g->text, strlen(g->text));
        //      LOGI_HEX(str, strlen(str));
        // }
        if(STR_EQUAL(g->text, str))
        {
            free(str);
            return;
        }
    }

    memset(g->text,0,LINE_MAX_LEN);
    memcpy(g->text, str, MIN(len, LINE_MAX_LEN));
    free(str);

    g->len = nlen;
    if(g->centered)
    {
        g->x = g->xu - g->len/2;
    }
    // if(index == 1) printf("set update to true\n");
    g->update = true;
}




static void gui_draw()
{
    // bool refresh = false;

    // for(int i = 0; i < MAX_ITEMS; ++i)
    // {
    //     gui_item_t* g = &items[i];
    //     if(is_bit_set(g->screen, active_screen))
    //     {
    //         if(g->update)
    //         {
    //             refresh = true;
    //             break;
    //         }
    //     }
    // }

    // if(refresh)
    {
        ssd1306_clear();

        if(gui_drawer != NULL)
        {
            gui_drawer();
        }

        for(int i = 0; i < MAX_ITEMS; ++i)
        {
            gui_item_t* g = &items[i];
            if(is_bit_set(g->screen, active_screen))
            {
                if(g->len == 0) continue;
                ssd1306_draw_string(g->x, g->y, g->text, g->fg, g->bg);
                g->update = false;
            }
        }
        ssd1306_refresh(true);
    }

}

static void gui_task(void* arg)
{
    LOGI("Entered gui task");

    for(;;)
    {

        if(gui_updater != NULL)
        {
            gui_updater();
        }

        gui_draw();

        delay(50);
    }

    LOGI("Leaving gui task");
    vTaskDelete(NULL);
}

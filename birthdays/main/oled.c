// https://github.com/baoshi/ESP-I2C-OLED

#include "common.h"
#include "oled.h"
#include "i2c.h"
#include "fonts.h"

#define PIN_SDA 25
#define PIN_SCL 26

#define I2C_ADDRESS 0x3C

#define TIMEOUT_TICKS 5000
#define CLK_SPEED 400000

#define WRITE_COMMAND   0x00  //((0 << 7) | (0 << 6))
#define WRITE_DATA      0x40  //((1 << 7) | (0 << 6))
// #define READ_STATUS     ((0 << 7) | (1 << 6))
// #define READ_DATA       ((1 << 7) | (1 << 6))

typedef struct
{
    uint8_t buffer[1024];   // display buffer // 128 * 64 / 8
    uint8_t width;          // panel width (128)
    uint8_t height;         // panel height (32 or 64)
    uint8_t refresh_top;    // "Dirty" window
    uint8_t refresh_left;
    uint8_t refresh_right;
    uint8_t refresh_bottom;
    const font_info_t* font;    // current font
} oled_ctx;

static oled_ctx ctx = {0};

static void _i2c_write(uint8_t type, uint8_t* data_send, int data_len)
{

    uint8_t* data = calloc(data_len + 1, sizeof(uint8_t));
    data[0] = type;
    memcpy(&data[1], data_send, data_len);
    i2c_write(OLED_I2C_PORT, I2C_ADDRESS, data, data_len+1, TIMEOUT_TICKS);
    free(data);
}

static void send_cmd(uint8_t command)
{
    _i2c_write(WRITE_COMMAND, &command, 1);
}

static void send_data(uint8_t* data_send, int data_len)
{
    _i2c_write(WRITE_DATA, data_send, data_len);
}


void ssd1306_init()
{
    i2c_init(OLED_I2C_PORT, CLK_SPEED, PIN_SDA, PIN_SCL);

    // ssd1306_term();
    memset(ctx.buffer, 0, 1024);
    ctx.width = 128;
    ctx.height = 64;

    send_cmd(0xae); // SSD1306_DISPLAYOFF
    send_cmd(0xd5); // SSD1306_SETDISPLAYCLOCKDIV
    send_cmd(0x80); // Suggested value 0x80
    send_cmd(0xa8); // SSD1306_SETMULTIPLEX
    send_cmd(0x3f); // 1/64
    send_cmd(0xd3); // SSD1306_SETDISPLAYOFFSET
    send_cmd(0x00); // 0 no offset
    send_cmd(0x40); // SSD1306_SETSTARTLINE line #0
    send_cmd(0x20); // SSD1306_MEMORYMODE
    send_cmd(0x00); // 0x0 act like ks0108
    send_cmd(0xa1); // SSD1306_SEGREMAP | 1
    send_cmd(0xc8); // SSD1306_COMSCANDEC
    send_cmd(0xda); // SSD1306_SETCOMPINS
    send_cmd(0x12);
    send_cmd(0x81); // SSD1306_SETCONTRAST
    send_cmd(0xcf);
    send_cmd(0xd9); // SSD1306_SETPRECHARGE
    send_cmd(0xf1);
    send_cmd(0xdb); // SSD1306_SETVCOMDETECT
    send_cmd(0x30);
    send_cmd(0x8d); // SSD1306_CHARGEPUMP
    send_cmd(0x14); // Charge pump on
    send_cmd(0x2e); // SSD1306_DEACTIVATE_SCROLL
    send_cmd(0xa4); // SSD1306_DISPLAYALLON_RESUME
    send_cmd(0xa6); // SSD1306_NORMALDISPLAY

    ssd1306_clear();
    ssd1306_refresh(true);

    send_cmd(0xaf); // SSD1306_DISPLAYON
}




void ssd1306_term()
{
    send_cmd(0xae); // SSD_DISPLAYOFF
    send_cmd(0x8d); // SSD1306_CHARGEPUMP
    send_cmd(0x10); // Charge pump off
    memset(ctx.buffer, 0, 1024);
}

void ssd1306_clear()
{
    memset(ctx.buffer, 0, 1024);
    ctx.refresh_right = ctx.width - 1;
    ctx.refresh_bottom = ctx.height - 1;
    ctx.refresh_top = 0;
    ctx.refresh_left = 0;
}

void ssd1306_refresh(bool force)
{
    uint8_t i,j;
    uint16_t k;
    uint8_t page_start, page_end;

    if (force)
    {
        send_cmd(0x21); // SSD1306_COLUMNADDR
        send_cmd(0);    // column start
        send_cmd(127);  // column end
        send_cmd(0x22); // SSD1306_PAGEADDR
        send_cmd(0);    // page start
        send_cmd(7);    // page end (8 pages for 64 rows OLED)
        for (k = 0; k < 1024; k++)
        {
            uint8_t _data[16] = {0};

            for (j = 0; j < 16; ++j)
            {
                _data[j] = ctx.buffer[k];
                ++k;
            }
            --k;
            send_data(_data, 16);
        }
    }
    else
    {
        if ((ctx.refresh_top <= ctx.refresh_bottom) && (ctx.refresh_left <= ctx.refresh_right))
        {
            page_start = ctx.refresh_top / 8;
            page_end = ctx.refresh_bottom / 8;
            send_cmd(0x21); // SSD1306_COLUMNADDR
            send_cmd(ctx.refresh_left);    // column start
            send_cmd(ctx.refresh_right);   // column end
            send_cmd(0x22); // SSD1306_PAGEADDR
            send_cmd(page_start);    // page start
            send_cmd(page_end); // page end
            k = 0;
            uint8_t _data[16] = {0};
            for (i = page_start; i <= page_end; ++i)
            {
                for (j = ctx.refresh_left; j <= ctx.refresh_right; ++j)
                {
                    _data[k] = ctx.buffer[i * ctx.width + j];
                    ++k;
                    if (k == 16)
                    {
                        send_data(_data,16);
                        memset(_data,0,16);
                        k = 0;
                    }
                }
            }

            if (k != 0) // for last batch if stop was not sent
                send_data(_data,k);
        }
    }
    // reset dirty area
    ctx.refresh_top = 255;
    ctx.refresh_left = 255;
    ctx.refresh_right = 0;
    ctx.refresh_bottom = 0;
}


void ssd1306_draw_pixel(int8_t x, int8_t y, ssd1306_color_t color)
{
    uint16_t index;

    if ((x >= ctx.width) || (x < 0) || (y >= ctx.height) || (y < 0))
        return;

    index = x + (y / 8) * ctx.width;
    switch (color)
    {
        case SSD1306_COLOR_WHITE:
            ctx.buffer[index] |= (1 << (y & 7));
            break;
        case SSD1306_COLOR_BLACK:
            ctx.buffer[index] &= ~(1 << (y & 7));
            break;
        case SSD1306_COLOR_INVERT:
            ctx.buffer[index] ^= (1 << (y & 7));
            break;
        default:
            break;
    }
    if (ctx.refresh_left > x) ctx.refresh_left = x;
    if (ctx.refresh_right < x) ctx.refresh_right = x;
    if (ctx.refresh_top > y) ctx.refresh_top = y;
    if (ctx.refresh_bottom < y) ctx.refresh_bottom = y;
}

void ssd1306_draw_hline(int8_t x, int8_t y, uint8_t w, ssd1306_color_t color)
{
    uint16_t index;
    uint8_t mask, t;

    // boundary check
    if ((x >= ctx.width) || (x < 0) || (y >= ctx.height) || (y < 0))
        return;
    if (w == 0)
        return;
    if (x + w > ctx.width)
        w = ctx.width - x;

    t = w;
    index = x + (y / 8) * ctx.width;
    mask = 1 << (y & 7);
    switch(color)
    {
        case SSD1306_COLOR_WHITE:
            while (t--)
            {
                ctx.buffer[index] |= mask;
                ++index;
            }
            break;
        case SSD1306_COLOR_BLACK:
            mask = ~mask;
            while (t--)
            {
                ctx.buffer[index] &= mask;
                ++index;
            }
            break;
        case SSD1306_COLOR_INVERT:
            while (t--)
            {
                ctx.buffer[index] ^= mask;
                ++index;
            }
            break;
        default:
            break;
    }
    if (ctx.refresh_left > x) ctx.refresh_left = x;
    if (ctx.refresh_right < x + w - 1) ctx.refresh_right = x + w - 1;
    if (ctx.refresh_top > y) ctx.refresh_top = y;
    if (ctx.refresh_bottom < y) ctx.refresh_bottom = y;
}


void ssd1306_draw_vline(int8_t x, int8_t y, uint8_t h, ssd1306_color_t color)
{
    uint16_t index;
    uint8_t mask, mod, t;

    // boundary check
    if ((x >= ctx.width) || (x < 0) || (y >= ctx.height) || (y < 0))
        return;
    if (h == 0)
        return;
    if (y + h > ctx.height)
        h = ctx.height - y;

    t = h;
    index = x + (y / 8) * ctx.width;
    mod = y & 7;
    if (mod) // partial line that does not fit into byte at top
    {
        // Magic from Adafruit
        mod = 8 - mod;
        static const uint8_t premask[8] = { 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE };
        mask = premask[mod];
        if (t < mod)
            mask &= (0xFF >> (mod - t));
        switch (color)
        {
            case SSD1306_COLOR_WHITE:
                ctx.buffer[index] |= mask;
                break;
            case SSD1306_COLOR_BLACK:
                ctx.buffer[index] &= ~mask;
                break;
            case SSD1306_COLOR_INVERT:
                ctx.buffer[index] ^= mask;
                break;
            default:
                break;
        }
        if (t < mod)
            goto draw_vline_finish;
        t -= mod;
        index += ctx.width;
    }
    if (t >= 8) // byte aligned line at middle
    {
        switch (color)
        {
            case SSD1306_COLOR_WHITE:
                do
            {
                ctx.buffer[index] = 0xff;
                index += ctx.width;
                t -= 8;
            } while (t >= 8);
                break;
            case SSD1306_COLOR_BLACK:
                do
                {
                ctx.buffer[index] = 0x00;
                index += ctx.width;
                t -= 8;
                } while (t >= 8);
                break;
            case SSD1306_COLOR_INVERT:
                do
                {
                    ctx.buffer[index] = ~ctx.buffer[index];
                    index += ctx.width;
                    t -= 8;
                } while (t >= 8);
                break;
            default:
                break;
        }
    }
    if (t) // // partial line at bottom
    {
        mod = t & 7;
        static const uint8_t postmask[8] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F };
        mask = postmask[mod];
        switch (color)
        {
            case SSD1306_COLOR_WHITE:
                ctx.buffer[index] |= mask;
                break;
            case SSD1306_COLOR_BLACK:
                ctx.buffer[index] &= ~mask;
                break;
            case SSD1306_COLOR_INVERT:
                ctx.buffer[index] ^= mask;
                break;
            default:
                break;
        }
    }
draw_vline_finish:
    if (ctx.refresh_left > x) ctx.refresh_left = x;
    if (ctx.refresh_right < x) ctx.refresh_right = x;
    if (ctx.refresh_top > y) ctx.refresh_top = y;
    if (ctx.refresh_bottom < y + h - 1) ctx.refresh_bottom = y + h - 1;
    return;
}


void ssd1306_draw_rectangle(int8_t x, int8_t y, uint8_t w, uint8_t h, ssd1306_color_t color)
{
    ssd1306_draw_hline(x, y, w, color);
    ssd1306_draw_hline(x, y + h - 1, w, color);
    ssd1306_draw_vline(x, y, h, color);
    ssd1306_draw_vline(x + w - 1, y, h, color);
}


void ssd1306_fill_rectangle(int8_t x, int8_t y, uint8_t w, uint8_t h, ssd1306_color_t color)
{
    // Can be optimized?
    uint8_t i;
    for (i = x; i < x + w; ++i)
        ssd1306_draw_vline(i, y, h, color);
}

void ssd1306_draw_circle(int8_t x0, int8_t y0, uint8_t r, ssd1306_color_t color)
{
    // Refer to http://en.wikipedia.org/wiki/Midpoint_circle_algorithm for the algorithm

    int8_t x = r;
    int8_t y = 1;
    int16_t radius_err = 1 - x;

    if (r == 0)
        return;

    ssd1306_draw_pixel(x0 - r, y0,     color);
    ssd1306_draw_pixel(x0 + r, y0,     color);
    ssd1306_draw_pixel(x0,     y0 - r, color);
    ssd1306_draw_pixel(x0,     y0 + r, color);

    while (x >= y)
    {
        ssd1306_draw_pixel(x0 + x, y0 + y, color);
        ssd1306_draw_pixel(x0 - x, y0 + y, color);
        ssd1306_draw_pixel(x0 + x, y0 - y, color);
        ssd1306_draw_pixel(x0 - x, y0 - y, color);
        if (x != y)
        {
            /* Otherwise the 4 drawings below are the same as above, causing
             * problem when color is INVERT
             */
            ssd1306_draw_pixel(x0 + y, y0 + x, color);
            ssd1306_draw_pixel(x0 - y, y0 + x, color);
            ssd1306_draw_pixel(x0 + y, y0 - x, color);
            ssd1306_draw_pixel(x0 - y, y0 - x, color);
        }
        ++y;
        if (radius_err < 0)
        {
            radius_err += 2 * y + 1;
        }
        else
        {
            --x;
            radius_err += 2 * (y - x + 1);
        }

    }
}


void ssd1306_fill_circle(int8_t x0, int8_t y0, uint8_t r, ssd1306_color_t color)
{
    int8_t x = 1;
    int8_t y = r;
    int16_t radius_err = 1 - y;
    int8_t x1;


    if (r == 0)
        return;

    ssd1306_draw_vline(x0, y0 - r, 2 * r + 1, color); // Center vertical line
    while (y >= x)
    {
        ssd1306_draw_vline(x0 - x, y0 - y, 2 * y + 1, color);
        ssd1306_draw_vline(x0 + x, y0 - y, 2 * y + 1, color);
        if (color != SSD1306_COLOR_INVERT)
        {
            ssd1306_draw_vline(x0 - y, y0 - x, 2 * x + 1, color);
            ssd1306_draw_vline(x0 + y, y0 - x, 2 * x + 1, color);
        }
        ++x;
        if (radius_err < 0)
        {
            radius_err += 2 * x + 1;
        }
        else
        {
            --y;
            radius_err += 2 * (x - y + 1);
        }
    }

    if (color == SSD1306_COLOR_INVERT)
    {
        x1 = x; // Save where we stopped

        y = 1;
        x = r;
        radius_err = 1 - x;
        ssd1306_draw_hline(x0 + x1, y0, r - x1 + 1, color);
        ssd1306_draw_hline(x0 - r, y0, r - x1 + 1, color);
        while (x >= y)
        {
            ssd1306_draw_hline(x0 + x1, y0 - y, x - x1 + 1, color);
            ssd1306_draw_hline(x0 + x1, y0 + y, x - x1 + 1, color);
            ssd1306_draw_hline(x0 - x,  y0 - y, x - x1 + 1, color);
            ssd1306_draw_hline(x0 - x,  y0 + y, x - x1 + 1, color);
            ++y;
            if (radius_err < 0)
            {
                radius_err += 2 * y + 1;
            }
            else
            {
                --x;
                radius_err += 2 * (y - x + 1);
            }
        }
    }
}


void ssd1306_select_font(uint8_t idx)
{
    if (idx < NUM_FONTS)
        ctx.font = fonts[idx];
}


// return character width


// uint8_t ssd1306_draw_char(uint8_t x, uint8_t y, char c, bool reversed)
// {
//     return ssd1306_draw_char2(x, y, c, SSD1306_COLOR_WHITE, SSD1306_COLOR_BLACK, reversed);
// }


uint8_t ssd1306_draw_char(uint8_t x, uint8_t y, char c, ssd1306_color_t foreground, ssd1306_color_t background, bool reversed)
{
    uint8_t i, j;
    const uint8_t *bitmap;
    uint8_t line = 0;

    if (ctx.font == NULL)
        return 0;

    // we always have space in the font set
    if ((c < ctx.font->char_start) || (c > ctx.font->char_end))
        c = ' ';
    c = c - ctx.font->char_start;   // c now become index to tables
    bitmap = ctx.font->bitmap + ctx.font->char_descriptors[(int)c].offset;
    uint8_t w = ctx.font->char_descriptors[(int)c].width;
    for (j = 0; j < ctx.font->height; ++j)
    {
        for (i = 0; i < w; ++i)
        {
            if (i % 8 == 0)
            {
                line = bitmap[(w + 7) / 8 * j + i / 8]; // line data
            }
            if (line & 0x80)
            {
                if(reversed)
                    ssd1306_draw_pixel(x + (w-i), y + j, foreground);
                else
                    ssd1306_draw_pixel(x + i, y + j, foreground);
            }
            else
            {
                switch (background)
                {
                    case SSD1306_COLOR_TRANSPARENT:
                        // Not drawing for transparent background
                        break;
                    case SSD1306_COLOR_WHITE:
                    case SSD1306_COLOR_BLACK:
                        if(reversed)
                            ssd1306_draw_pixel(x + (w-i), y + j, background);
                        else
                            ssd1306_draw_pixel(x + i, y + j, background);
                        break;
                    case SSD1306_COLOR_INVERT:
                        // I don't know why I need invert background
                        break;
                    default:
                        break;
                }
            }
            line = line << 1;
        }
    }
    return (w);
}


uint8_t ssd1306_draw_string(uint8_t x, uint8_t y, char *str, ssd1306_color_t foreground, ssd1306_color_t background, bool reversed)
{
    uint8_t t = x;

    if (ctx.font == NULL)
        return 0;

    if (str == NULL)
        return 0;

    int v = 1;
    char* start = str;
    char* end = &str[strlen(str)-1];
    if(reversed)
    {
        char* temp = start;
        start = end;
        end = temp;
        v = -1;
    }

    for(;;)
    {
        x += ssd1306_draw_char(x, y, *start, foreground, background, reversed);
        if(start == end) break;
        start += v;
        x += ctx.font->c;   // c represents space between characters
    }
    return (x - t);
}


// return width of string
uint8_t ssd1306_measure_string(char *str)
{
    uint8_t w = 0;
    char c;

    if (ctx.font == NULL)
        return 0;

    while (*str)
    {
        c = *str;
        // we always have space in the font set
        if ((c < ctx.font->char_start) || (c > ctx.font->char_end))
            c = ' ';
        c = c - ctx.font->char_start;   // c now become index to tables
        w += ctx.font->char_descriptors[(int)c].width;
        ++str;
       if (*str)
           w += ctx.font->c;
    }
    return w;
}


uint8_t ssd1306_get_font_height()
{
    if (ctx.font == NULL)
        return 0;

    return (ctx.font->height);
}


uint8_t ssd1306_get_font_c()
{
    if (ctx.font == NULL)
        return 0;

    return (ctx.font->c);
}


void ssd1306_invert_display(bool invert)
{
    if (invert)
        send_cmd(0xa7); // SSD1306_INVERTDISPLAY
    else
        send_cmd(0xa6); // SSD1306_NORMALDISPLAY
}


void ssd1306_update_buffer(uint8_t* data, uint16_t length)
{
    memcpy(ctx.buffer, data, (length < 1024) ? length : 1024);
    ctx.refresh_right = ctx.width - 1;
    ctx.refresh_bottom = ctx.height - 1;
    ctx.refresh_top = 0;
    ctx.refresh_left = 0;
}

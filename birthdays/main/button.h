#pragma once

// ====================================================================
// INCLUDES
// ====================================================================

#include "common.h"
#include <driver/gpio.h>



// ====================================================================
// DEFINES
// ====================================================================

#define NULL_PIN 0

#define PIN_MASK(p) ((uint64_t)1<<p)

#define BUTTON_GPIO_LOW  0
#define BUTTON_GPIO_HIGH 1

#define BUTTON_DEFAULT_PARAMS() { \
    .task_delay_ms = 10, \
    .presses_cooldown_ms = 700, \
    .press_debounce_ms = 30, \
    .hold_threshold_ms = 400, \
    .continuous_press_ms = 1000, \
}

// ====================================================================
// TYPEDEFS
// ====================================================================

typedef void (*button_cb_t)();


typedef struct
{
    uint8_t task_delay_ms;
    uint16_t presses_cooldown_ms;
    uint8_t press_debounce_ms;
    uint16_t hold_threshold_ms;
    uint16_t continuous_press_ms;
} button_params_t;


typedef enum
{
    HOLD_TIME,
    HOLD_TIME_MOD,  // every x seconds of holding the button down
    PRESSES, // mod presses
    PRESSES_EXACT, // no more, no less presses
    PRESSES_ONE_TIME // on the nth press, even if there are more afterwards
} ButtonEventTrigger;

typedef struct
{
    uint32_t val;
    uint64_t timestamp;
} button_stat_t;

typedef struct
{
    uint64_t pin_mask; // gpio pin mask
    uint8_t pressed_logic_level;
    bool held; // button is/was held down
    button_stat_t pressed_time;
    button_stat_t released_time;
    button_stat_t num_presses;
    button_stat_t temp_presses;
    button_stat_t end_presses;
    uint16_t counter; // counter for the button cooldown
} button_data_t;

typedef struct
{
    uint64_t pin_mask;

    // trigger
    ButtonEventTrigger trigger_type;
    
    // hold and hold mod
    uint32_t hold_time_start;
    uint32_t hold_time_end;

    // hold mod
    uint32_t hold_time_mod;

    // press events
    uint8_t num_presses;

} button_event_t;

typedef struct
{
    button_event_t event;
    button_cb_t button_cb_0;
    button_cb_t button_cb_1;
    button_cb_t button_cb_2;
    bool processed_cb_0;
    bool processed_cb_1;
    bool processed_cb_2;
} button_event_params_t;

// ====================================================================
// GLOBAL FUNCTIONS
// ====================================================================

bool button_init(button_params_t _params, uint8_t _num_buttons, uint8_t _num_events);
void button_deinit();
bool button_add_event_hold(uint64_t pin_mask, uint32_t hold_time_start, uint32_t hold_time_end, button_cb_t press_cb, button_cb_t release_cb, button_cb_t end_cb);
bool button_add_event_hold_mod(uint64_t pin_mask, uint32_t hold_time_start, uint32_t hold_time_end, uint32_t hold_time_mod, button_cb_t press_cb);
bool button_add_event_press(uint64_t pin_mask, ButtonEventTrigger trigger_type, uint8_t num_presses, button_cb_t cb);
bool button_config_pin(uint64_t pin_mask, uint8_t pressed_logic_level);
bool button_get_data(uint64_t pin_mask, button_data_t* data);
button_event_params_t* button_get_event();
void button_process();
uint8_t button_convert_mask(uint64_t pin_mask, uint8_t pins[2], uint64_t masks[2]);
void button_clear_events();

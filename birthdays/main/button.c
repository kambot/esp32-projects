// ====================================================================
// INCLUDES
// ====================================================================

#include "common.h"
#include "button.h"

static TaskHandle_t button_task_handle = NULL;

static uint8_t button_count = 0;
static uint8_t num_buttons = 0;
static uint8_t event_count = 0;
static uint8_t num_events = 0;
static uint64_t timer = 0;

static button_params_t params = {0};
static button_data_t* buttons = NULL;
static button_event_params_t* events = NULL;
static button_event_params_t* curr_event = NULL;

static void button_task();
static button_data_t* get_button_data(uint64_t pin_mask);


bool button_init(button_params_t _params, uint8_t _num_buttons, uint8_t _num_events)
{
    num_buttons = _num_buttons;
    num_events = _num_events;
    memcpy(&params, &_params, sizeof(button_params_t));

    buttons = (button_data_t*)calloc(num_buttons,sizeof(button_data_t));
    events = (button_event_params_t*)calloc(num_events,sizeof(button_event_params_t));

    for(int i = 0; i < num_buttons; ++i)
    {
        buttons[i].pin_mask = NULL_PIN;
    }

    for(int i = 0; i < num_events; ++i)
    {
        events[i].event.pin_mask = NULL_PIN;
    }

    xTaskCreatePinnedToCore(button_task, "button_task", 4000, NULL, 2, NULL, 0);

    return true;
}

void button_deinit()
{
    if(button_task_handle != NULL)
    {
        vTaskDelete(button_task_handle);
        button_task_handle = NULL;
    }
    button_clear_events();
}


bool button_add_event_hold(uint64_t pin_mask, uint32_t hold_time_start, uint32_t hold_time_end, button_cb_t press_cb, button_cb_t release_cb, button_cb_t end_cb)
{
    if(event_count >= num_events) {
        LOGE("too many events");
        return false;
    }

    button_event_params_t* e = &events[event_count];

    e->event.pin_mask = pin_mask;
    e->event.trigger_type = HOLD_TIME;
    e->event.hold_time_start = hold_time_start;
    e->event.hold_time_end = hold_time_end;
    e->button_cb_0 = press_cb;
    e->button_cb_1 = release_cb;
    e->button_cb_2 = end_cb;
    e->processed_cb_0 = false;
    e->processed_cb_1 = false;
    e->processed_cb_2 = false;

    event_count++;
    return true;
}


bool button_add_event_hold_mod(uint64_t pin_mask, uint32_t hold_time_start, uint32_t hold_time_end, uint32_t hold_time_mod, button_cb_t press_cb)
{
    if(event_count >= num_events) {
        LOGE("too many events");
        return false;
    }

    button_event_params_t* e = &events[event_count];

    e->event.pin_mask = pin_mask;
    e->event.trigger_type = HOLD_TIME_MOD;
    e->event.hold_time_start = hold_time_start;
    e->event.hold_time_end = hold_time_end;
    e->event.hold_time_mod = hold_time_mod;
    e->button_cb_0 = press_cb;
    e->processed_cb_0 = false;

    event_count++;
    return true;
}


bool button_add_event_press(uint64_t pin_mask, ButtonEventTrigger trigger_type, uint8_t num_presses, button_cb_t cb)
{
    if(event_count >= num_events) {
        LOGE("too many events");
        return false;
    }

    button_event_params_t* e = &events[event_count];

    e->event.pin_mask = pin_mask;
    e->event.trigger_type = trigger_type;
    e->event.num_presses = num_presses;
    e->button_cb_0 = cb;
    e->button_cb_1 = NULL;
    e->button_cb_2 = NULL;
    e->processed_cb_0 = false;
    e->processed_cb_1 = false;
    e->processed_cb_2 = false;

    event_count++;
    return true;
}

bool button_config_pin(uint64_t pin_mask, uint8_t pressed_logic_level)
{
    if(button_count >= num_buttons) {
        LOGE("too many buttons");
        return false;
    }

    uint8_t pins[2] = {0};
    if(button_convert_mask(pin_mask, pins, NULL) != 1) {
        return false;
    }

    gpio_config_t conf_button = {0};
    conf_button.intr_type    = GPIO_INTR_DISABLE;
    conf_button.mode         = GPIO_MODE_INPUT;
    conf_button.pin_bit_mask = pin_mask;
    if(pressed_logic_level == 0)
    {
        conf_button.pull_down_en = GPIO_PULLDOWN_DISABLE;
        conf_button.pull_up_en   = GPIO_PULLUP_ENABLE;
    }
    else
    {
        conf_button.pull_down_en = GPIO_PULLDOWN_ENABLE;
        conf_button.pull_up_en   = GPIO_PULLUP_DISABLE;
    }
    esp_err_t err = gpio_config(&conf_button);

    if(err) {
        LOGE("Failed to config gpio %u: 0x%X (%s)", pins[0], err, esp_err_to_name(err));
        return false;
    }

    button_data_t* button = &buttons[button_count];

    button->pin_mask = pin_mask;
    button->pressed_logic_level = pressed_logic_level;
    button->held = false;

    button_count++;
    return true;
}

bool button_get_data(uint64_t pin_mask, button_data_t* data)
{
    button_data_t* temp = get_button_data(pin_mask);
    if(temp == NULL) return false;
    memcpy(data, temp, sizeof(button_data_t));
    return true;
}

button_event_params_t* button_get_event()
{
    return curr_event;
}


void button_process()
{
    timer += params.task_delay_ms;

    for(int i = 0; i < num_buttons; i++)
    {
        button_data_t* b = &buttons[i];

        if(b->pin_mask == NULL_PIN) {
            continue;
        }

        b->counter += params.task_delay_ms;
        if(b->counter >= params.presses_cooldown_ms && b->num_presses.val > 0 && ( timer - b->num_presses.timestamp == params.presses_cooldown_ms))
        {
            // printf("%u, %u, %llu, %llu\n", b->counter, params.presses_cooldown_ms, timer, b->num_presses.timestamp);
            b->end_presses.val = b->num_presses.val;
            b->end_presses.timestamp = timer;
            b->counter = 0;
            LOGI("End presses (%d): %u", i, b->num_presses.val);
        }

        uint8_t pins[2] = {0};
        button_convert_mask(b->pin_mask, pins, NULL);

        if(gpio_get_level(pins[0]) == b->pressed_logic_level)
        {
            if(!b->held)
            {
                b->released_time.val = 0;
                b->end_presses.val = 0;
                b->pressed_time.val = 0;

                if(timer - b->temp_presses.timestamp >= params.continuous_press_ms)
                    b->temp_presses.val = 0;

                b->temp_presses.val += 1;
                b->temp_presses.timestamp = timer;
                b->counter = 0;
                // LOGI("TEMP Continuous presses (%d): %u", i, b->temp_presses.val);
            }

            b->released_time.val = 0;
            b->pressed_time.val += params.task_delay_ms;
            b->pressed_time.timestamp = timer;
            b->held = true;
            if(b->pressed_time.val % 1000 == 0) LOGI("Pressed time (%d): %u", i, b->pressed_time.val);

            // 'press' now becomes a 'hold'
            if(b->pressed_time.val >= params.hold_threshold_ms) {
                b->num_presses.val = 0;
                b->temp_presses.val = 0;
                b->end_presses.val = 0;
            }
        }
        else if(b->held)
        {
            if(b->pressed_time.val < params.hold_threshold_ms && b->pressed_time.val >= params.press_debounce_ms)
            {
                b->num_presses.val = b->temp_presses.val;
                b->num_presses.timestamp = timer;
                LOGI("Continuous presses (%d): %u", i, b->num_presses.val);
                b->counter = 0;
            }

            if(b->num_presses.val == 0 && b->num_presses.timestamp != timer)
            {
                b->released_time.val = b->pressed_time.val;
                b->released_time.timestamp = timer;
                LOGI("Released time (%d): %u", i, b->released_time.val);
            }
            b->held = false;
        }

    }//button loop

    // b = &buttons[0];
    // if(timer % 1000 == 0)
    // LOGI("%8u | %8u | %8u | %8u", b->num_presses.val, b->end_presses.val, b->pressed_time.val, b->released_time.val);

    // for these buttons, single events will be ignored

    bool* ignore_single_events = (bool*)calloc(num_buttons, sizeof(bool));

    //double button events
    for(int i = 0; i < event_count; ++i)
    {
        button_event_params_t* e = &events[i];
        uint8_t pins[2] = {0};
        uint64_t masks[2] = {0};
        if(button_convert_mask(e->event.pin_mask, pins, masks) != 2) continue;

        button_data_t* b1 = get_button_data(masks[0]);
        if(b1 == NULL) continue;

        button_data_t* b2 = get_button_data(masks[1]);
        if(b2 == NULL) continue;

        for(int j = 0; j < num_buttons; j++) {
            if((buttons[j].pin_mask & e->event.pin_mask) != 0) {
                if(b1->held && b2->held)
                    ignore_single_events[j] = true;
            }
        }

        curr_event = e;

        ButtonEventTrigger trigger = e->event.trigger_type;

        if(trigger == PRESSES_ONE_TIME)
        {
            if(b1->num_presses.val == 0) continue;
            if(b2->num_presses.val == 0) continue;

            bool c0 = (b1->num_presses.val == e->event.num_presses) && (b2->num_presses.val == e->event.num_presses);
            bool c1 = timer - b1->num_presses.timestamp < 500;
            bool c2 = timer - b2->num_presses.timestamp < 500;

            if(c0 && c1 && c2)
            {
                if(e->button_cb_0 && !e->processed_cb_0) e->button_cb_0();
                e->processed_cb_0 = true;
                continue;
            } else {
                e->processed_cb_0 = false;
            }
        }
        else if(trigger == PRESSES)
        {
            if(b1->num_presses.val == 0) continue;
            if(b2->num_presses.val == 0) continue;

            bool c0 = (b1->num_presses.val % e->event.num_presses) == 0 && (b2->num_presses.val % e->event.num_presses) == 0;
            bool c1 = timer - b1->num_presses.timestamp < 500;
            bool c2 = timer - b2->num_presses.timestamp < 500;

            if(c0 && c1 && c2)
            {
                if(e->button_cb_0 && !e->processed_cb_0) e->button_cb_0();
                e->processed_cb_0 = true;
                continue;
            } else {
                e->processed_cb_0 = false;
            }
        }
        else if(trigger == PRESSES_EXACT)
        {
            if(b1->end_presses.val == 0) continue;
            if(b2->end_presses.val == 0) continue;

            bool c0 = (b1->end_presses.val == e->event.num_presses) && (b2->end_presses.val == e->event.num_presses);
            bool c1 = timer - b1->end_presses.timestamp < 500;
            bool c2 = timer - b2->end_presses.timestamp < 500;

            if(c0 && c1 && c2)
            {
                if(e->button_cb_0 && !e->processed_cb_0) e->button_cb_0();
                e->processed_cb_0 = true;
                continue;
            } else {
                e->processed_cb_0 = false;
            }
        }
        else if(trigger == HOLD_TIME)
        {
            if(b1->pressed_time.val == 0) continue;
            if(b2->pressed_time.val == 0) continue;

            uint32_t erange = e->event.hold_time_end - e->event.hold_time_start;

            // conditions
            bool c0 = (b1->pressed_time.val >= e->event.hold_time_start) && (b2->pressed_time.val >= e->event.hold_time_start);
            bool c1 = timer - b1->pressed_time.timestamp < 500;
            bool c2 = timer - b2->pressed_time.timestamp < 500;

            bool c3 = (b1->released_time.val <= e->event.hold_time_end) && (b2->released_time.val <= e->event.hold_time_end);
            bool c4 = timer - b1->released_time.timestamp < erange && b1->released_time.timestamp != 0;
            bool c5 = timer - b2->released_time.timestamp < erange && b2->released_time.timestamp != 0;

            bool c6 = (b1->pressed_time.val > e->event.hold_time_end) || (b2->pressed_time.val > e->event.hold_time_end);
            bool c7 = (b1->pressed_time.val > e->event.hold_time_end) && (b2->pressed_time.val > e->event.hold_time_end);

            // held buttons long enough but not too long
            if(c0 && c1 && c2 && !c7 && !c4 && !c5)
            {
                if(e->button_cb_0 && !e->processed_cb_0) e->button_cb_0();
                e->processed_cb_0 = true;
                continue;
            }
            // held buttons long enough and released both before event ending time
            else if(c0 && c3 && c4 && c5)
            {
                if(e->button_cb_1 && !e->processed_cb_1) e->button_cb_1();
                e->processed_cb_1 = true;
                continue;
            }
            // held one or both buttons for too long
            // else if(c6)
            else if(c6 && (c4 || c5 || c7))
            {
                if(e->button_cb_2 && !e->processed_cb_2) e->button_cb_2();
                e->processed_cb_2 = true;
                continue;
            }
            else
            {
                e->processed_cb_0 = false;
                e->processed_cb_1 = false;
                e->processed_cb_2 = false;
            }
        }
        else if(trigger == HOLD_TIME_MOD)
        {

            if(b1->pressed_time.val == 0) continue;
            if(b2->pressed_time.val == 0) continue;

            // conditions
            bool c0 = (b1->pressed_time.val >= e->event.hold_time_start) && (b2->pressed_time.val >= e->event.hold_time_start);
            bool c1 = timer - b1->pressed_time.timestamp < 500;
            bool c2 = timer - b2->pressed_time.timestamp < 500;
            bool c3 = (b1->pressed_time.val <= e->event.hold_time_end) && (b2->pressed_time.val <= e->event.hold_time_end);

            bool c4 = false;
            if(b1->pressed_time.timestamp < b2->pressed_time.timestamp)
                c4 = ((b1->pressed_time.val % e->event.hold_time_mod) == 0);
            else
                c4 = ((b2->pressed_time.val % e->event.hold_time_mod) == 0);
            
            // held button within range and mod time
            if(c0 && c1 && c2 && c3 && c4)
            {
                if(e->button_cb_0 && !e->processed_cb_0) e->button_cb_0();
                e->processed_cb_0 = true;
                continue;
            } else {
                e->processed_cb_0 = false;
            }


        }


    }//double button events


    //single button events
    for(int i = 0; i < event_count; ++i)
    {
        button_event_params_t* e = &events[i];
        if(button_convert_mask(e->event.pin_mask, NULL, NULL) != 1) continue;

        button_data_t* b = get_button_data(e->event.pin_mask);
        if(b == NULL) continue;

        bool ignore = false;
        for(int j = 0; j < event_count; j++) {
            if(buttons[j].pin_mask == e->event.pin_mask) {
                if(ignore_single_events[j])
                    ignore = true;
                break;
            }
        }
        if(ignore) continue;

        curr_event = e;

        ButtonEventTrigger trigger = e->event.trigger_type;

        if(trigger == PRESSES_ONE_TIME)
        {
            if(b->num_presses.val == 0) continue;
            if(b->num_presses.val == e->event.num_presses && b->num_presses.timestamp == timer)
            {
                if(e->button_cb_0) e->button_cb_0();
                continue;
            }
        }
        else if(trigger == PRESSES)
        {
            if(b->num_presses.val == 0) continue;
            if((b->num_presses.val % e->event.num_presses) == 0 && b->num_presses.timestamp == timer)
            {
                if(e->button_cb_0) e->button_cb_0();
                continue;
            }
        }
        else if(trigger == PRESSES_EXACT)
        {
            if(b->end_presses.val == 0) continue;
            if(b->end_presses.val == e->event.num_presses && b->end_presses.timestamp == timer)
            {
                if(e->button_cb_0) e->button_cb_0();
                continue;
            }
        }
        else if(trigger == HOLD_TIME)
        {
            // conditions
            bool c0 = (b->pressed_time.val >= e->event.hold_time_start) && (b->pressed_time.timestamp == timer);
            bool c1 = b->pressed_time.val >= e->event.hold_time_start;
            bool c2 = (b->released_time.val <= e->event.hold_time_end) && (b->released_time.timestamp == timer);
            bool c3 = (b->pressed_time.val > e->event.hold_time_end) && (b->pressed_time.timestamp == timer);

            // held button long enough
            if(c0 && !c3)
            {
                if(e->button_cb_0 && !e->processed_cb_0) e->button_cb_0();
                e->processed_cb_0 = true;
                continue;
            }
            // held button long enough and released before event ending time
            else if(c1 && c2)
            {
                if(e->button_cb_1 && !e->processed_cb_1) e->button_cb_1();
                e->processed_cb_1 = true;
                continue;
            }
            // held button for too long
            else if(c3)
            {
                if(e->button_cb_2 && !e->processed_cb_2) e->button_cb_2();
                e->processed_cb_2 = true;
                continue;
            }
            else
            {
                e->processed_cb_0 = false;
                e->processed_cb_1 = false;
                e->processed_cb_2 = false;
            }
        }
        else if(trigger == HOLD_TIME_MOD)
        {
            if(b->pressed_time.val == 0) continue;

            // conditions
            bool c0 = (b->pressed_time.val >= e->event.hold_time_start) && (b->pressed_time.val <= e->event.hold_time_end);
            // bool c1 = ((b->pressed_time.val - e->event.hold_time_start) % e->event.hold_time_mod == 0) && (b->pressed_time.timestamp == timer);
            bool c1 = ((b->pressed_time.val % e->event.hold_time_mod) == 0) && (b->pressed_time.timestamp == timer);

            // held button within range and mod time
            if(c0 && c1)
            {
                if(e->button_cb_0 && !e->processed_cb_0) e->button_cb_0();
                e->processed_cb_0 = true;
                continue;
            } else {
                e->processed_cb_0 = false;
            }
        }
        else
        {
            LOGE("Unknown trigger type: %d", trigger);
        }

    }//single button events

    free(ignore_single_events);

}

uint8_t button_convert_mask(uint64_t pin_mask, uint8_t pins[2], uint64_t masks[2])
{
    if(pin_mask == 0) return 0;

    uint8_t count = 0;
    for(int i = 0; i < 63; ++i)
    {
        uint8_t bit = (pin_mask >> i) & 1;
        if(bit == 1)
        {
            if(masks != NULL) masks[count] = PIN_MASK(i);
            if(pins != NULL) pins[count] = i;
            count++;
        }
        if(count >= 2) break;
    }
    return count;
}


void button_clear_events()
{
    if(event_count == 0) return;
    for(int i = 0; i < event_count; ++i) {
        events[i].event.pin_mask = NULL_PIN;
    }
    event_count = 0;
}



// static functions

static void button_task()
{
    LOGI("Entered button task");
    button_task_handle = xTaskGetCurrentTaskHandle();

    TickType_t last_wake_time = xTaskGetTickCount();
    for(;;)
    {
        button_process();
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(params.task_delay_ms));
    }
}

static button_data_t* get_button_data(uint64_t pin_mask)
{
    if(button_convert_mask(pin_mask, NULL, NULL) != 1) {
        return NULL;
    }

    int index = -1;
    for(int i = 0; i < num_buttons; ++i) {
        if(buttons[i].pin_mask == pin_mask) {
            index = i;
            break;
        }
    }
    if(index == -1) return NULL;
    return &buttons[index];
}


#include <stdlib.h>
#include <string.h>
#include "watch_utility.h"
#include "watch_rtc.h"
#include "micro_timer_face.h"

static const uint16_t ADDTIME_TICKS_START = 8;
static const uint8_t ADDTIME_TICKS_FREQ = 8;
static const uint8_t NORMAL_TICKS_FREQ = 1;
static int8_t BEEP_SEQ[] = {
	BUZZER_NOTE_C8, 3, BUZZER_NOTE_REST, 3, -2, 2, BUZZER_NOTE_C8, 5, BUZZER_NOTE_REST, 25,
	BUZZER_NOTE_C8, 3, BUZZER_NOTE_REST, 3, -2, 2, BUZZER_NOTE_C8, 5, BUZZER_NOTE_REST, 25,
	0};

typedef struct {
    // How much they care about us (lower is more)
    uint8_t watch_face_index;

    // State while adding time
    uint8_t addtime_step;
    uint16_t addtime_ticks_left;

    // Core timer state
    uint32_t now_ts;
    uint32_t target_ts;
} micro_timer_state_t;

static void draw(micro_timer_state_t* state);
static void reset(micro_timer_state_t* state);

static uint32_t addtime_time_for_step(uint8_t step);
static void addtime_tick(micro_timer_state_t* state);
static void addtime_add(micro_timer_state_t* state);

static void draw(micro_timer_state_t* state) {
    static char buf[11] = {};

    watch_duration_t time_to_display;
    if (state->addtime_step == 0) {
        uint32_t const secs_left =
            state->target_ts > state->now_ts
                ? state->target_ts - state->now_ts
                : 0;

        time_to_display = watch_utility_seconds_to_duration(secs_left);
    } else {
        time_to_display = watch_utility_seconds_to_duration(addtime_time_for_step(state->addtime_step));
    }

    // Flash the colon each tick while we count down
    // When not ticking, addtime_ticks_left is 0, so the colon is always on
    if (state->addtime_ticks_left % 2 == 1) {
        watch_clear_colon();
    } else {
        watch_set_colon();
    }

    // Header
    watch_display_string("uT", 0);

    // Main time
    snprintf(buf, sizeof(buf),
        "%02d%02d%02d",
        time_to_display.hours,
        time_to_display.minutes,
        time_to_display.seconds);
    watch_display_string(buf, 4);

    // Day code
    if (time_to_display.days == 0) {
        // No days -- leave it blank
        watch_display_string("  ", 2);
    } else if (time_to_display.days <= 39) {
        // A normal number of days -- set it to the integer
        snprintf(buf, sizeof(buf), "%2lu", time_to_display.days);
        watch_display_string(buf, 2);
    } else {
        // An excessive number of days. We'll count them right, but can't
        // display them. Draw some horizontal bars.
        //
        // Given that this would require hitting the increment button over 400
        // times, not the biggest concern.
        watch_display_string("  ", 2);
        watch_set_pixel(1, 9);
        watch_set_pixel(0, 7);
        watch_set_pixel(1, 8);
        watch_set_pixel(2, 6);
    }
}

static void reset(micro_timer_state_t* state) {
    state->addtime_step = 0;
    state->addtime_ticks_left = 0;
    state->now_ts = 0;
    state->target_ts = 0;

    movement_cancel_background_task_for_face(state->watch_face_index);
}

static uint32_t addtime_time_for_step(uint8_t step) {
    static uint32_t const times[] = {
        0,     //  UNUSED
        30,    //     :30
        60,    //    1:00
        120,   //    2:00
        180,   //    3:00
        240,   //    4:00
        300,   //    5:00
        600,   //   10:00
        900,   //   15:00
        1200,  //   20:00
        1800,  //   30:00
    };
    static uint8_t const times_len = sizeof(times) / sizeof(times[0]);

    if (step < times_len) {
        return times[step];
    } else {
        uint32_t hours = step - times_len + 1;
        return hours * 3600;
    }
}

static void addtime_tick(micro_timer_state_t* state) {
    if (state->addtime_ticks_left == 0) {
        // No-op: We're not currently ticking
    } else if (state->addtime_ticks_left == 1) {
        // We're just now running out of ticks.
        //
        // Add the required time based on the current step.

        if (state->target_ts < state->now_ts) {
            // If there was an old target timestamp, we might've run past it.
            state->target_ts = state->now_ts;
        }

        state->target_ts += addtime_time_for_step(state->addtime_step);

        state->addtime_step = 0;
        state->addtime_ticks_left = 0;

        movement_request_tick_frequency(NORMAL_TICKS_FREQ);
        movement_schedule_background_task_for_face(
            state->watch_face_index,
            watch_utility_date_time_from_unix_time(state->target_ts, 0));
    } else {
        // Another tick gone, but we're not done yet.
        state->addtime_ticks_left -= 1;
    }
}

static void addtime_add(micro_timer_state_t* state) {
    if (state->addtime_step == 0) {
        // This is the start of the addtime sequence
        movement_request_tick_frequency(ADDTIME_TICKS_FREQ);
    }

    state->addtime_step += 1;
    state->addtime_ticks_left = ADDTIME_TICKS_START;
}

void micro_timer_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(micro_timer_state_t));
    }

    micro_timer_state_t* state = *context_ptr;
    state->watch_face_index = watch_face_index;
    reset(state);
}

void micro_timer_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;

    micro_timer_state_t* state = (micro_timer_state_t*)context;

    watch_date_time now = watch_rtc_get_date_time();
    state->now_ts = watch_utility_date_time_to_unix_time(now, 0);
}

bool micro_timer_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    bool ok_to_sleep = true;
    micro_timer_state_t* state = (micro_timer_state_t*)context;

    // Always update the current time first
    state->now_ts = watch_utility_date_time_to_unix_time(watch_rtc_get_date_time(), 0);

    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            addtime_tick(state);
            draw(state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            addtime_add(state);
            draw(state);
            break;
        case EVENT_ALARM_LONG_PRESS:
            reset(state);
            draw(state);
            break;
        case EVENT_BACKGROUND_TASK:
            // the alarm just went off
            watch_buzzer_play_sequence(BEEP_SEQ, NULL);
            ok_to_sleep = false;
            break;
        default:
            movement_default_loop_handler(event, settings);
            break;
    }

    return ok_to_sleep;
}

void micro_timer_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;

    micro_timer_state_t* state = (micro_timer_state_t*)context;

    // If we're in the middle of adding time, finish that up right now.
    state->addtime_ticks_left = 1;
    addtime_tick(state);
}

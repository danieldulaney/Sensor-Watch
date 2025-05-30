/*
 * MIT License
 *
 * Copyright (c) 2025 Daniel Dulaney
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef MICRO_TIMER_FACE_H_
#define MICRO_TIMER_FACE_H_

/*
 * The smallest timer face.
 *
 * For setting up a single timer real fast.
 *
 * Press Alarm repeatedly to add time. The time you're currently adding shows
 * temporarily. When you stop pressing Alarm, the time you selected gets added
 * to the running timer.
 */

#include "movement.h"

void micro_timer_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr);
void micro_timer_face_activate(movement_settings_t *settings, void *context);
bool micro_timer_face_loop(movement_event_t event, movement_settings_t *settings, void *context);
void micro_timer_face_resign(movement_settings_t *settings, void *context);

#define micro_timer_face ((const watch_face_t){ \
    micro_timer_face_setup, \
    micro_timer_face_activate, \
    micro_timer_face_loop, \
    micro_timer_face_resign, \
    NULL, \
})

#endif // MICRO_TIMER_FACE_H_

/*
 *  THEC64 Mini
 *  Copyright (C) 2017 Chris Smith
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef C64_MANAGER_H
#define C64_MANAGER_H

#include <stdio.h>
#include <stdlib.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <maliaw.h>

#include "flashlight.h"
#include "joystick.h"
#include "gamelibrary.h"
#include "ig_menu.h"
#include "uniforms.h"

typedef enum {
    C64_MANAGER_STATIC = 0,
    C64_MANAGER_FADE_IN,
    C64_MANAGER_FADE_OUT
} C64State;

typedef struct {
    Flashlight *f;
    FLVerticeGroup *vertice_group;
    FLTexture      *texture;
    Uniforms      uniforms;

    unsigned char *screen;

    C64State      state;
    int           fade_level;
    int           focus;
    int           display_on;
    InGameMenu   *igmenu;
    game_t       *game;   // The game being loaded
    int           controller_primary;
    int           port_remap[4 + 1]; // First element is NOT used

    int           got_input;

    int           suspend_point_request;
    unsigned long display_on_timer_ms;
    unsigned long sound_on_timer_ms;
    unsigned long sound_off_timer_ms;

    float target_offset_x;
    float offset_x;
    unsigned long target_offset_x_t;
    int silence_for_frames;
    int silence_enabled;

    display_mode_t display_mode;

    mali_texture   *maliFLTexture;

} C64Manager;

C64Manager * c64_manager_init( Flashlight *f, fonts_t *fonts );
void c64_silence_for_frames( C64Manager *g, int frames );
#endif

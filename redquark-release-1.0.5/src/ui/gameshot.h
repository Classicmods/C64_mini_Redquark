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
#ifndef GAMESHOT_H
#define GAMESHOT_H

#include <stdio.h>
#include <stdlib.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "flashlight_vertice.h"
#include "flashlight_texture.h"
#include "flashlight_event.h"
#include "flashlight_shader.h"

#include "uniforms.h"
#include "gamelibrary.h"

#define IMG_SLOTS 2

typedef enum {
    GAMESHOT_STATIC = 0,
    GAMESHOT_FADE_IN,
    GAMESHOT_FADE_OUT,
    GAMESHOT_FADE_DELAY,
    GAMESHOT_SWAP_IMAGE,
} GameshotState;

typedef struct {
    Flashlight *f;

    float fade_level       [IMG_SLOTS];
    float target_fade_level[IMG_SLOTS];

    unsigned long fade_target_t[IMG_SLOTS];

    int active_slot;
    int inactive_slot;
    int redraw;
    int current_image;

    unsigned long fade_delay_target_t;
    unsigned long shot_toggle_time_ms;

    game_t *game;

    GameshotState state;

    FLVerticeGroup *vertice_groups[IMG_SLOTS];
    FLVerticeGroup *frame_vertice_group;

    FLTexture      *texture;

    Uniforms uniforms;

    int focus;
} Gameshot;

Gameshot * gameshot_init( Flashlight *f );
#endif

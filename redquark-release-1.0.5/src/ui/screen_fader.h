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
#ifndef SCREEN_FADER_H
#define SCREEN_FADER_H

#include <stdio.h>
#include <stdlib.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "flashlight.h"

#include "uniforms.h"
#include "fontatlas.h"
#include "fonts.h"

typedef struct {
    Flashlight *f;
    FLVerticeGroup *vertice_group;
    FLTexture      *texture;

    Uniforms uniforms;

    fonts_t      *fonts; 

    int focus;
    int fading;

    float   a;
    float   target_a;
    float   delta_a;
} Fader;

typedef enum {
    FADE_BLACK = 0,
    FADE_MID   = 50,
    FADE_OFF   = 100
} fade_level_t;

Fader * fader_init( Flashlight *f, fonts_t *fonts );
void fader_set_level( Fader *f, int level, int steps );
void fader_shutdown_message( Fader *f );
void fader_reboot_message( Fader *f );
#endif

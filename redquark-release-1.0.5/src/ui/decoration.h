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
#ifndef DECORATION_H
#define DECORATION_H

#include <stdio.h>
#include <stdlib.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "flashlight.h"

#include "uniforms.h"

typedef struct {
    Flashlight *f;

    FLVerticeGroup *vertice_group;
    FLTexture *texture;

    Uniforms uniforms;

    int enable;

    int sound_enabled;
} Decoration;

Decoration * decoration_init( Flashlight *f );
void decoration_enable( Decoration *);
void decoration_disable( Decoration *);

void decorate_music_enable ( Decoration *d );
void decorate_music_disable( Decoration *d );
#endif

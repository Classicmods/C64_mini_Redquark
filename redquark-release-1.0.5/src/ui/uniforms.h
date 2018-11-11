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
#ifndef UNIFORMS_H
#define UNIFORMS_H

#include "flashlight_screen.h"

extern int need_flush;

typedef struct {
    GLint   texture_unit;

    GLint   rotation_matrix;
    GLint   translation_matrix;
    GLint   texture_fade;
    GLint   texture_fade_grey;

    int     blending;
    float   fade;
} Uniforms;

void  uni_init    ( Uniforms *u );
int   uni_blending( Uniforms *u, int state );
float uni_set_fade( Uniforms *u, float fade );
#endif

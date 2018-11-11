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
#ifndef FONT_ATLAS_H
#define FONT_ATLAS_H

#include "flashlight.h"

#include "font-manager.h"
#include "harfbuzz_kerning.h"

#define MAX_FONTS 5

typedef int font_id_t;

typedef struct {
    Flashlight            *f;
    FLTexture             *atlas;
    font_manager_t        *font_manager;

    int                    font_count;

    int                    font_max;
    kerned_texture_font_t *kerned_font[MAX_FONTS];
    int                    font_size  [MAX_FONTS];

} fontatlas_t;

fontatlas_t * fontatlas_init( Flashlight *f, int atlas_size );
int fontatlas_add_font( fontatlas_t *fa, const char *font_filename, int font_size );
void register_fontatlas_with_shader( fontatlas_t *fa );

#endif

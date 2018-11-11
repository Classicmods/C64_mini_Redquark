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
#include "path.h"
#include "fonts.h"

static fonts_t fonts;

// ----------------------------------------------------------------------------

#define STEM "/usr/share/the64/ui/fonts/"

fonts_t *
font_init( Flashlight *f )
{
    char *path;

    fontatlas_t *fa = fontatlas_init( f, 512 );

    path = get_path( STEM "Roboto-Regular.ttf" );
    fonts.heading = fontatlas_add_font( fa, path, 36 );

    path = get_path( STEM "Roboto-Regular.ttf" );
    fonts.body    = fontatlas_add_font( fa, path, 22 );

    path = get_path( STEM "Roboto-Italic.ttf" );
    fonts.italic  = fontatlas_add_font( fa, path,  22 );

    register_fontatlas_with_shader( fa ); // Must be done AFTER fonts have been added
    fonts.fontatlas = fa;

    return &fonts;
}

// ----------------------------------------------------------------------------

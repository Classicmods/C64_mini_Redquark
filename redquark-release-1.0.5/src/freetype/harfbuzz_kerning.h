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
#ifndef HARFBUZZ_KERNING_H
#define HARFBUZZ_KERNING_H
// Use Harfbuzz to provide glyph advance information
//
#include <ft2build.h>
#include "texture-font.h"
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>

#define STATIC_FONT_LOADING

#include "tsffont.h"

#ifdef STATIC_FONT_LOADING
#define BUCKETS      127
#define BUCKET_DEPTH 10

typedef struct {
    tsf_texture_glyph_t *glyph_ptr[ BUCKET_DEPTH ];
    int length;
} glyph_bucket_t;

typedef struct {
    texture_font_index_t *font;
    glyph_bucket_t        glyph_bucket[ BUCKETS ];
} font_table_t;
#endif


typedef struct {
#ifdef STATIC_FONT_LOADING
    font_table_t    font_table;
#else 
    texture_font_t  texture_font;
#endif

    FT_Library      library;
    FT_Face         face;
    hb_buffer_t    *buf;

    const char     *text;
    int             len;
    int             index;

    hb_font_t           *font;
    hb_glyph_position_t *glyph_pos;
    unsigned int         glyph_count;

} kerned_texture_font_t;

// ----------------------------------------------------------------------------

#ifdef STATIC_FONT_LOADING
kerned_texture_font_t * kerned_texture_font_init( texture_font_index_t *font, const char *path );
texture_font_index_t * find_precompiled_font(  char *filename, int size );
tsf_texture_glyph_t * kerned_texture_font_get_glyph( kerned_texture_font_t *ktf, const char * codepoint );
#else
kerned_texture_font_t * kerned_texture_font_init( texture_font_t *font );
texture_glyph_t * kerned_texture_font_get_glyph( kerned_texture_font_t *ktf, uint32_t codepoint );
#endif

void kerned_texture_font_destroy( kerned_texture_font_t *ktf );
float kerned_texture_font_get_advance( kerned_texture_font_t *ktf, int glyph_index );

unsigned int kerned_texture_font_process_text( kerned_texture_font_t *ktf,  const char* text, int len );

#endif

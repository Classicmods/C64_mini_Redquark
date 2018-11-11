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
#ifndef TEXTPANE_H
#define TEXTPANE_H

#include "flashlight_vertice.h"
#include "flashlight_texture.h"
#include "flashlight_event.h"
#include "flashlight_shader.h"
#include "uniforms.h"
#include "fontatlas.h"
#include "font-manager.h"

typedef enum {
    TEXT_LEFT,
    TEXT_RIGHT,
    TEXT_CENTER
} textpane_justify_t;

typedef enum {
    TP_FLAG_NONE     = 0,
    TP_FLAG_DOCUMENT = 1<<0,
} textpane_flag_t;

typedef enum {
    TP_COLOUR_DARK  = 0,
    TP_COLOUR_LIGHT = 1<<0,
} textpane_text_colour_t;

typedef struct {
    kerned_texture_font_t   *kerned_font;
    FLVerticeGroup            *vertice_group;

    fontatlas_t             *fontatlas;
    font_id_t                font_id;

    unsigned int             glyph_count;   // Number of UTF* glyphs in buffer
    int                      current_glyph; // Which UTF8 glyph we're on 
             char           *text;          // Text buffer
    int                      text_index;    // Byte index into text buffer for current glyph

    textpane_flag_t          flags;
    textpane_justify_t       justify;

    int                      width;
    int                      height;
    int                      display_width;
    int                      display_height;

    int                      largest_width; // The length of the longest line in the pane

    float x;
    float y;

    float                    min_x;         // The leftmost edge of display text. Useful with justification

} textpane_t;

textpane_t * textpane_init( FLVerticeGroup *vg, fontatlas_t *font_atlas, int font_id, int width, int height, int dwidth, int dheight, textpane_justify_t justify );
int build_text( textpane_t *tp, unsigned char *text, textpane_text_colour_t colour, float x, float y );

#endif

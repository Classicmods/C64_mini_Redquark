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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "flashlight.h"

#include "summary.h"
#include "user_events.h"

#include "vec234.h"
#include "texture-atlas.h"
#include "texture-font.h"
#include "utf8-utils.h"
#include "font-manager.h"

#include <ft2build.h>
#include "harfbuzz_kerning.h"

#include "textpane.h"

// ----------------------------------------------------------------------------------
//
textpane_t *
textpane_init( FLVerticeGroup *vg, fontatlas_t *fontatlas, int font_id, int width, int height, int dwidth, int dheight, textpane_justify_t justify )
{
    textpane_t *tp = NULL;

    tp = malloc( sizeof(textpane_t) );
    if(tp == NULL) return NULL;

    tp->fontatlas = fontatlas;
    tp->font_id   = font_id;

    tp->vertice_group = vg;

    tp->width  = width;
    tp->height = height;
    tp->display_width  = dwidth;
    tp->display_height = dheight;
    tp->current_glyph = 0;
    tp->text_index = 0;
    tp->justify    = justify;

    tp->x = tp->y = 0.f;

    tp->largest_width = 0;

    return tp;
}

// ----------------------------------------------------------------------------------
//
static void
justify( textpane_t *tp, int count, float x )
{
    float offset = -10.f;
    FLVerticeGroup *g = tp->vertice_group;
    FLVertice * v[4];

    int i;
    for(i = 0; i < count; i++ ) {
        int ix = g->index_len - i - 1;
        flashlight_vertice_get_quad2D( g, ix, &v[0], &v[1], &v[2], &v[3] );

        if( offset < -9.f ) {
//printf("RH Edge is at %f (%f)\n", x + tp->width, X_TO_GL(tp->display_width, x + tp->width));
//printf("x is %f, width is %d\n", x, tp->width );

            offset = (1.f + X_TO_GL(tp->display_width, x + tp->width)) - (1.f + v[2]->x) - 0.f;
            if( tp->justify == TEXT_CENTER ) offset /= 2;
            //printf("Offset to right is %f   (x2 = %f)\n", offset, v[2]->x );
        }
        v[0]->x += offset;
        v[1]->x += offset;
        v[2]->x += offset;
        v[3]->x += offset;

        if( tp->min_x > v[0]->x ) tp->min_x = v[0]->x;
    }
}

// ----------------------------------------------------------------------------------
// Returns the number of characters still to render
static int
build_line( textpane_t *tp, kerned_texture_font_t *kfont, int *raw_index, int *glyph_index, int glyph_count, float x, float y, int color )
{
    unsigned char prev_codepoint = 0;

    int break_glyph_ix = -1;
    int break_raw_ix   = -1;

    float lx      = x;
    float covered = 0.f;
    float break_width = 0.f;
    int   ri      = *raw_index;
    int   gi      = *glyph_index;

    // The glyph bounding box is 1 pixel too large, so calulate the texture width ajust here
    float x_one_pixel = S_TO_GL(tp->fontatlas->atlas->width,  0.1);
    float y_one_pixel = T_TO_GL(tp->fontatlas->atlas->height, 0.1);

    while( tp->text[ri] == 0x20 ) ri++, gi++; // Eat leading space

#ifdef STATIC_FONT_LOADING
    tsf_texture_glyph_t *glyph = NULL;
#else
    texture_font_t  *tfont = &(kfont->texture_font);
    texture_glyph_t *glyph = NULL;
#endif

    for( ; gi < glyph_count; ++gi, ri += utf8_surrogate_len(tp->text + ri) )
    {
        if( (covered > (float)tp->width) && (break_glyph_ix >= 0) ) {
            // wind back to the last linebreak'able point (space, hyphen, comma, fullstop...)

            // remove overshot verticies
            flashlight_vertice_remove_quad2D( tp->vertice_group, gi - break_glyph_ix );
            
            if( tp->justify != TEXT_LEFT  ) justify( tp, break_glyph_ix - *glyph_index, lx );

            *glyph_index = break_glyph_ix;
            *raw_index   = break_raw_ix;
            printd( "break_glyph_ix = %d   break_raw_ix = %d\n", break_glyph_ix, break_raw_ix );

            if( break_width >= (float)(tp->largest_width) ) tp->largest_width = ceil(break_width);

            return glyph_count - *glyph_index;
        }

#ifdef STATIC_FONT_LOADING
        glyph = kerned_texture_font_get_glyph( kfont, tp->text + ri );
#else
        glyph = texture_font_get_glyph( tfont, tp->text + ri );
#endif

        if( glyph->codepoint == 0x0a || glyph->codepoint == 0x0d ) { // \n or \r
            // Deal with linefeed
            ri += utf8_surrogate_len( tp->text + ri);
            break;
        }

        if( prev_codepoint == 0x20 || (prev_codepoint >= 0x2c && prev_codepoint <= 0x2f) ) {
            break_glyph_ix = gi;
            break_raw_ix   = ri;
            break_width    = covered;
        }
        prev_codepoint = glyph->codepoint;

        if( glyph != NULL )
        {
            float x0  = x + glyph->offset_x;
            float y0  = y + glyph->offset_y;
            float x1  = x0 + glyph->width; // - 1; // glyph bounding box is 1 texel too wide
            float y1  = y0 - glyph->height;// + 1; // glyph bounding box is 1 texel too wide
//printf("CP: %d  x0: %f  y0: %f  x1: %f  y1: %f\n", prev_codepoint, x0, y0, x1, y1 );
//printf("CP: %3d  x0: %d  y0: %d  w: %ld  h: %ld\n", prev_codepoint, glyph->offset_x, glyph->offset_y, glyph->width, glyph->height );

            float s0 = glyph->s0 + x_one_pixel;
            float t0 = glyph->t0 + y_one_pixel;
            float s1 = glyph->s1 - x_one_pixel;
            float t1 = glyph->t1 - y_one_pixel;

            FLVertice vi[4];
            vi[0].x = vi[1].x = X_TO_GL(tp->display_width,  x0);
            vi[0].y = vi[2].y = Y_TO_GL(tp->display_height, y0);
            vi[0].s = vi[1].s = s0;
            vi[1].t = vi[3].t = t1;

            vi[2].x = vi[3].x = X_TO_GL(tp->display_width,  x1);
            vi[1].y = vi[3].y = Y_TO_GL(tp->display_height, y1);
            vi[2].s = vi[3].s = s1;
            vi[0].t = vi[2].t = t0;

            vi[0].u = vi[1].u = vi[2].u = vi[3].u = (color == TP_COLOUR_LIGHT) ? (float)0.6 : (float)0.4;

            flashlight_vertice_add_quad2D( tp->vertice_group, &vi[0], &vi[1], &vi[2], &vi[3] );

            float adx = kerned_texture_font_get_advance( kfont, gi );
            x += adx;
            covered += adx;
        }
    }

    if( tp->justify != TEXT_LEFT  ) justify( tp, gi - *glyph_index, lx );
    
    if( glyph->codepoint == 0x0a || glyph->codepoint == 0x0d ) gi++;  // \n or \r

    *glyph_index = gi;
    *raw_index   = ri;

    if( covered >= (float)(tp->largest_width) ) tp->largest_width = ceil(covered);

    return glyph_count - *glyph_index;
}

// ----------------------------------------------------------------------------------
// Returns the number of BYTES rendered, not glyphs.
int
build_text( textpane_t *tp, unsigned char *text, textpane_text_colour_t colour, float x, float y )
{
    // TODO Alter x and y so that the textpane is centred on the origin (0,0)
    // and use a translation matrix to position correctly.
    int remaining = 0;
    int line_height = 20;
    int line_space = 4;
    int skipped = 0;

    int l = 0;

    if( text[0] == '\0' ) return 0;

    int len = 0;
    if( tp->flags & TP_FLAG_DOCUMENT ) {
        // Skip leading blank lines from document
        while( text[skipped] == ' ' || text[skipped] == '\n' ) skipped++;

        // Calculate the length (and height) of the remainder.
        l = tp->height;
        while( text[skipped + len] != 0 ) {
            if( text[skipped + len] == '\n' ) {
                l -= (line_height + line_space);
                if( l < line_height ) break;
            }
            len++;
        }
    } else {
        len = strlen( (char *)text );
    }

    kerned_texture_font_t *kfont = tp->fontatlas->kerned_font[ tp->font_id ];;

    // calculate kerned glyph x advance data FIXME Move into "compile text" or something
    tp->text = (char *)text + skipped;
    tp->glyph_count = kerned_texture_font_process_text( kfont, tp->text, len );
    tp->current_glyph = 0;
    tp->text_index    = 0;
    tp->x             = x;
    tp->y             = y;
    tp->min_x         = 1.1;

    l = 0;
    do {
        remaining = build_line( tp, kfont, &(tp->text_index), &(tp->current_glyph), tp->glyph_count, x, y, colour );
        l += line_height + line_space;
        y -= line_height + line_space; 

        if( (tp->height - l) < line_height ) {
            // No room for another Cannot get a whole line-height 
            return tp->text_index + skipped;
        }
    } while( remaining > 0 );

    return tp->text_index + skipped;
}

// ----------------------------------------------------------------------------------

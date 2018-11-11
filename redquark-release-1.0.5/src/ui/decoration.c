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

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "flashlight.h"

#include "decoration.h"
#include "path.h"
#include "user_events.h"

#define WIDTH 1280
#define HEIGHT 720

#define SE  30

static int atlas_width;
static int atlas_height;

static FLVertice decoration_verticies[] = {
    { 0,      0,  7.1,  60},  // Background strip
    { 0,     60,  7.1,  59},
    { 1280,   0,  8,  60},
    { 1280,  60,  8,  59},

    { 1080 - SE,   0 + SE - 4,   0, 120},  // Logo 200 x 
    { 1080 - SE,  44 + SE - 4,   0,  76},
    { 1280 - SE,   0 + SE - 4, 200, 120},
    { 1280 - SE,  44 + SE - 4, 200,  76},

    {    0 + SE + 31,   0 + SE - 6, 563, 119},  // "Sound" icon
    {    0 + SE + 31,  29 + SE - 6, 563,  90},
    {   77 + SE + 31,   0 + SE - 6, 640, 119},
    {   77 + SE + 31,  29 + SE - 6, 640,  90},
    
    {    0 + SE + 86,   0 + SE - 4, 483,  81},  // "X" icon
    {    0 + SE + 86,  21 + SE - 4, 483,  60},
    {   21 + SE + 86,   0 + SE - 4, 504,  81},
    {   21 + SE + 86,  21 + SE - 4, 504,  60},
};

static void decoration_event( void *self, FLEvent *event );

// ----------------------------------------------------------------------------------
//
static void
add_image_quads( Flashlight *f, Decoration *d )
{
    FLVertice vi[4];

    FLVerticeGroup *vg = flashlight_vertice_create_group(f, 8);
	d->vertice_group = vg;

    // Make sure the screen asset atlas texture is loaded
    FLTexture *tx = flashlight_load_texture( f, get_path("/usr/share/the64/ui/textures/menu_atlas.png") );
    if( tx == NULL ) { printf("Error tx\n"); exit(1); }

    atlas_width = tx->width;
    atlas_height = tx->height;

    flashlight_register_texture    ( tx, GL_NEAREST );
    flashlight_shader_attach_teture( f, tx, "u_DecorationUnit" );

    // Build triangles
    int i,j;
    for(i = 0; i < sizeof(decoration_verticies)/sizeof(FLVertice); i += 4 ) {
        for(j = 0; j < 4; j++ ) {

            vi[j].x = X_TO_GL(WIDTH,      decoration_verticies[i + j].x);
            vi[j].y = Y_TO_GL(HEIGHT,     decoration_verticies[i + j].y);
            vi[j].s = S_TO_GL(tx->width,  decoration_verticies[i + j].s);
            vi[j].t = T_TO_GL(tx->height, decoration_verticies[i + j].t);
            vi[j].u = 0.2;
        }
        flashlight_vertice_add_quad2D( vg, &vi[0], &vi[1], &vi[2], &vi[3] );
    }

    return;
}

// ----------------------------------------------------------------------------------
// Initialise in-game shot display
Decoration *
decoration_init( Flashlight *f )
{
    Decoration *d = malloc( sizeof(Decoration) );
    if( d == NULL ) {
    }
    memset( d, 0, sizeof(Decoration) );

    d->f = f;

    add_image_quads(f, d);

    d->uniforms.texture_fade       = flashlight_shader_assign_uniform( f, "u_fade"               );
    d->uniforms.translation_matrix = flashlight_shader_assign_uniform( f, "u_translation_matrix" );

    flashlight_event_subscribe( f, (void*)d, decoration_event, FLEventMaskAll );

    d->enable = 1;

    decorate_music_enable( d );

    return d;
}

// ----------------------------------------------------------------------------------

void
decoration_finish( Decoration **g ) {
    free(*g);
    *g = NULL;
}

// ----------------------------------------------------------------------------------
//
void
decorate_music_enable( Decoration *d )
{
    FLVertice *v[4];

    if( d->sound_enabled ) return;

    d->sound_enabled = 1;

    // Move X offscreen
    flashlight_vertice_get_quad2D( d->vertice_group, 3, &v[0], &v[1], &v[2], &v[3] );

    v[0]->x -= 2;
    v[1]->x -= 2;
    v[2]->x -= 2;
    v[3]->x -= 2;
}
    
// ----------------------------------------------------------------------------------
//
void
decorate_music_disable( Decoration *d )
{
    FLVertice *v[4];

    if( ! d->sound_enabled ) return;

    d->sound_enabled = 0;

    // Move X on-screen
    flashlight_vertice_get_quad2D( d->vertice_group, 3, &v[0], &v[1], &v[2], &v[3] );

    v[0]->x += 2;
    v[1]->x += 2;
    v[2]->x += 2;
    v[3]->x += 2;
}

// ----------------------------------------------------------------------------------
//
void
decoration_enable( Decoration *d )
{
    d->enable = 1;
}

// ----------------------------------------------------------------------------------
//
void
decoration_disable( Decoration *d )
{
    d->enable = 0;
}

// ----------------------------------------------------------------------------------

static void
decoration_event( void *self, FLEvent *event )
{
    Decoration *g = (Decoration *)self;

    switch( event->type ) {
        case Redraw:
            if( g->enable ) {

                FLVerticeGroup *cg = g->vertice_group;

                glUniformMatrix4fv( g->uniforms.translation_matrix,1, GL_FALSE, flashlight_translate( 0.0, 0.0, 0.0 ) );
                glUniform1f( g->uniforms.texture_fade, 1.0 );
                flashlight_shader_blend_on(g->f);
                glDrawElements(GL_TRIANGLES, cg->indicies_len, GL_UNSIGNED_SHORT, flashlight_get_indicies(cg) );
            }
            break;
        case UserEvent:
            switch(((TSFUserEvent*)event)->user_type) {
                case FullScreenClaimed:
                    g->enable = 0;
                    break;
                case FullScreenReleased:
                    g->enable = 1;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

// ----------------------------------------------------------------------------------

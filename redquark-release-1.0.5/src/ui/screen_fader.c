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

#include "screen_fader.h"
#include "user_events.h"
#include "path.h"
#include "textpane.h"
#include "locale.h"

#define WIDTH 1280
#define HEIGHT 720

static void fader_event( void *self, FLEvent *event );

static Fader *fad = NULL;

typedef enum {
    Fade_Off = 0,
    Fade_On  = 1,
} FadeType;

static FLVertice fader_verticies[] = {
    { 0,    0,   0.1, 57.1, 0.2},
    { 0,    720, 0.1, 57.9, 0.2},
    { 1280, 0,   0.9, 57.1, 0.2},
    { 1280, 720, 0.9, 57.9, 0.2},
};

static int atlas_width  = 0;
static int atlas_height = 0;

// ----------------------------------------------------------------------------------
//
static FLVerticeGroup *
add_background_quad( Flashlight *f )
{
    int len = sizeof(fader_verticies) / sizeof(FLVertice);

    FLVertice vi[ 4 ];
    FLVerticeGroup *vgb = flashlight_vertice_create_group(f, 1);

    // Make sure the screen asset atlas texture is loaded
    FLTexture *tx = flashlight_load_texture( f, get_path( "/usr/share/the64/ui/textures/menu_atlas.png") );
    if( tx == NULL ) { printf("Error tx\n"); exit(1); }

    flashlight_register_texture    ( tx, GL_NEAREST );
    flashlight_shader_attach_teture( f, tx, "u_DecorationUnit" );

    atlas_width  = tx->width; // Global
    atlas_height = tx->height; // Global

    int i,j;
    for(i = 0; i < len; i += 4 ) {
        for(j = 0; j < 4; j++ ) {

            vi[j].x = X_TO_GL(WIDTH,        fader_verticies[i + j].x);
            vi[j].y = Y_TO_GL(HEIGHT,       fader_verticies[i + j].y);
            vi[j].s = S_TO_GL(atlas_width,  fader_verticies[i + j].s);
            vi[j].t = T_TO_GL(atlas_height, fader_verticies[i + j].t);
            vi[j].u = fader_verticies[i + j].u;
        }
        flashlight_vertice_add_quad2D( vgb, &vi[0], &vi[1], &vi[2], &vi[3] );
    }
    return vgb;
}

// ----------------------------------------------------------------------------------
// Initialise in-game shot display
Fader *
fader_init( Flashlight *f, fonts_t *fonts )
{
    if( fad != NULL ) return fad; // There can only be one fader

    fad = malloc( sizeof(Fader) );
    if( fad == NULL ) {
        return NULL;
    }

    fad->f = f;

    fad->vertice_group = add_background_quad( f );
    fad->fonts = fonts;

    fad->uniforms.texture_fade       = flashlight_shader_assign_uniform( f, "u_fade"               );
    fad->uniforms.translation_matrix = flashlight_shader_assign_uniform( f, "u_translation_matrix" );
   
    flashlight_event_subscribe( f, (void*)fad, fader_event, FLEventMaskAll );

    return fad;
}

// ----------------------------------------------------------------------------------

void
fader_finish( Fader **f ) {
    free(*f);
    *f = fad = NULL;
}

// ----------------------------------------------------------------------------------
// Level is between 0 = black and 100 = transparent/off
void
fader_set_level( Fader *f, int level, int steps )
{
    f->target_a = (float)(100 - level) / 100.f;

    if( steps == 0 ) {
        f->a = f->target_a; // Set level immediately
    } else {
        f->delta_a  = (f->target_a - f->a ) / steps;
    }

    f->fading = ( f->target_a == f->a ) ? 0 : 1;
} 

// ----------------------------------------------------------------------------------
//
static FadeType
test_for_fade( Fader *s )
{
    int fade = Fade_Off;

    if( s->target_a != s->a && s->delta_a != 0.0 ) fade |= Fade_On;

    return fade;
}

// ----------------------------------------------------------------------------------
//
static int
fader_process( Fader *s )
{
    int fade = 0; 
    static int last_fade = 0;

    fade = test_for_fade( s );

    if( fade & Fade_On ) {
        s->a += s->delta_a;
        if( s->delta_a < 0 && s->a < s->target_a ) s->a = s->target_a;
        if( s->delta_a > 0 && s->a > s->target_a ) s->a = s->target_a;
    }

    if( last_fade && !fade ) {
        TSFUserEvent e = { {UserEvent, (void*)s}, FadeComplete };
        flashlight_event_publish( s->f,(FLEvent*)&e );
    }
    last_fade = fade;

//    if( fade != Fade_Off ) {
//        // See if the next fade will be the last, and send the "stop" event.
//        // Doing it a frame early allows subscribes to respond and request that the selector
//        // continues to fade without missing a frame.
//        fade = test_for_fade( s );
//        if( !fade ) {
//            // Send event that fade has finished
//            TSFUserEvent e = { {UserEvent, (void*)s}, FadeComplete };
//            flashlight_event_publish( s->f,(FLEvent*)&e );
//        }
//    } 

    s->fading = ( fade || s->a != 0.f ) ? 1 : 0; // Fader is on unless transparent

    return 1;
}

// ----------------------------------------------------------------------------------
//
void
fader_shutdown_message( Fader *f )
{
    textpane_t *msg = textpane_init( f->vertice_group, f->fonts->fontatlas, f->fonts->heading, 1280, 720, 1280, 720, TEXT_CENTER ); 
    unsigned char *txt = locale_get_text( "SF_SHUTDOWN" );
    build_text( msg, txt, 1, 0, 720/2 - 20 );
}

// ----------------------------------------------------------------------------------
//
void
fader_reboot_message( Fader *f )
{
    textpane_t *msg = textpane_init( f->vertice_group, f->fonts->fontatlas, f->fonts->heading, 1280, 720, 1280, 720, TEXT_CENTER ); 
    unsigned char *txt = locale_get_text( "SF_REBOOTING" );
    build_text( msg, txt, 1, 0, 720/2 - 20 );
}

// ----------------------------------------------------------------------------------
static void
fader_event( void *self, FLEvent *event )
{
    Fader *fad = (Fader *)self;

    switch( event->type ) {
        case Redraw:
            {
                if( fad->fading ) {
                    glUniformMatrix4fv( fad->uniforms.translation_matrix,1, GL_FALSE, flashlight_translate( 0.0, 0.0, 0.0 ) );
                    glUniform1f( fad->uniforms.texture_fade, fad->a);

                    flashlight_shader_blend_on(fad->f);
                    FLVerticeGroup *vg = fad->vertice_group;
                    glDrawElements(GL_TRIANGLES, vg->indicies_len, GL_UNSIGNED_SHORT, flashlight_get_indicies(vg) );
                }
            }
            break;
        case Process:
            {
                if( fad->fading ) fader_process( fad );
            }
            break;
#ifdef FADER_USER_EVENTS
        case UserEvent:
            switch(((TSFUserEvent*)event)->user_type) {
            }
            break;
#endif
        default:
            break;
    }
}

// ----------------------------------------------------------------------------------
// end

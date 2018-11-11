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

#include "summary.h"
#include "user_events.h"

#include "vec234.h"
#include "texture-atlas.h"
#include "texture-font.h"
#include "utf8-utils.h"

#include <ft2build.h>
#include "harfbuzz_kerning.h"

#include "textpane.h"
#include "path.h"
#include "locale.h"
#include "event.h"

#define WIDTH 1280
#define HEIGHT 720

#define VP_LEFT  706
#define VP_RIGHT 1243
#define VP_TOP  (719 - 21)

#define FADE_RATE_IN    12  // 25
#define FADE_RATE_OUT   5   // 12

static FLVertice gamesummary_verticies[] = {
    { VP_LEFT,  376,         6.1,  57.9},     // Dark blue rectangle
    { VP_LEFT,  VP_TOP - 48, 6.1,  57.1},
    { VP_RIGHT, 376,         6.9,  57.9},
    { VP_RIGHT, VP_TOP - 48, 6.9,  57.1},
};

static void gamesummary_event( void *self, FLEvent *event );

// ----------------------------------------------------------------------------------
//
static FLVerticeGroup*
text_background_quad( Flashlight *f )
{
    FLVertice vi[sizeof(gamesummary_verticies)/sizeof(FLVertice)];

    FLVerticeGroup *vgb = flashlight_vertice_create_group(f, 4);

    // Make sure the screen asset atlas texture is loaded
    FLTexture *atx = flashlight_load_texture( f, get_path("/usr/share/the64/ui/textures/menu_atlas.png") );
    if( atx == NULL ) { printf("Error atx\n"); exit(1); }
    flashlight_register_texture    ( atx, GL_NEAREST );
    flashlight_shader_attach_teture( f, atx, "u_DecorationUnit" );

    int i,j;
    for(i = 0; i < sizeof(gamesummary_verticies)/sizeof(FLVertice); i += 4 ) {
        for(j = 0; j < 4; j++ ) {
            vi[j].x = X_TO_GL(WIDTH,       gamesummary_verticies[i + j].x);
            vi[j].y = Y_TO_GL(HEIGHT,      gamesummary_verticies[i + j].y);
            vi[j].s = S_TO_GL(atx->width,  gamesummary_verticies[i + j].s);
            vi[j].t = T_TO_GL(atx->height, gamesummary_verticies[i + j].t);
            vi[j].u = 0.2;
        }
        flashlight_vertice_add_quad2D( vgb, &vi[0], &vi[1], &vi[2], &vi[3] );
    }
    return vgb;
}

// ----------------------------------------------------------------------------------
//
static void
load_game_text( GameSummary *g, int slot )
{
    FLVerticeGroup *vg = g->vertice_groups[ slot ];

    flashlight_vertice_remove_all( vg );
//printf("load_game_text: Removed all vertices from group. Indicies length is now %d from a max of %d\n", vg->indicies_len, vg->indicies_max );

    locale_select_language( g->language );

    int left = VP_LEFT + 10;
    if( g->language == LANG_DE ) left +=  8;
    if( g->language == LANG_FR ) left += 23;
    if( g->language == LANG_IT ) left += 23;
    if( g->language == LANG_ES ) left += 13;

    build_text( g->title_pane [ slot ], g->game->title,       0, VP_LEFT,       VP_TOP - 28);
    build_text( g->desc_pane  [ slot ], g->game->description[ g->language - LANG_MIN ], 1, VP_LEFT + 6,   627 );
    build_text( g->year_pane  [ slot ], g->game->year,        0, VP_RIGHT - 54, 345 - 48 );

    build_text( g->yearl_pane [ slot ], locale_get_text("GS_YEAR"), 0, VP_RIGHT - 54 - 60, 345 - 48 );


    unsigned char text[512];
    unsigned char c;
    unsigned char *s;
    int j = 0;
    int i = 0;

    i = j = 0;
    s = locale_get_text( "GS_AUTHOR" );
    while( j < (sizeof(text)-1) && (c = s[i++]) != 0 ) text[j++] = c;
    text[j++] = '\n';

    i = 0;
    s = locale_get_text( "GS_MUSICIAN" );
    while( j < (sizeof(text)-1) && (c = s[i++]) != 0 ) text[j++] = c;
    text[j++] = '\n';

    i = 0;
    s = locale_get_text( "GS_GENRE" );
    while( j < (sizeof(text)-1) && (c = s[i++]) != 0 ) text[j++] = c;
    text[j++] = '\0';

    build_text( g->label_pane[ slot ], text, 0, left, 345 );

    i = j = 0;
    s = g->game->author[0];
    s = s[0] ? s : (unsigned char *)"-";
    while( j < (sizeof(text)-1) && (c = s[i++]) != 0 ) text[j++] = c;

    if( g->game->author_count > 1 ) {
        text[j++] = ',';
        text[j++] = ' ';
        i = 0;
        s = g->game->author[1];
        while( j < (sizeof(text)-1) && (c = s[i++]) != 0 ) text[j++] = c;
    }
    text[j++] = '\n';

    i = 0;
    s = g->game->music;
    s = s[0] ? s : (unsigned char *)"-";
    while( j < (sizeof(text)-1) && (c = s[i++]) != 0 ) text[j++] = c;
    text[j++] = '\n';

    i = 0;
    s = locale_get_text( (char *)g->game->genre );
    s = s[0] ? s : (unsigned char *)"-";
    while( j < (sizeof(text)-1) && (c = s[i++]) != 0 ) text[j++] = c;
    text[j++] = '\0';

    build_text( g->credit_pane[ slot ], text, 0, left + 85 + 20, 345 );

    
    return;
}

// ----------------------------------------------------------------------------------
// Initialise summary text handler
GameSummary *
gamesummary_init( Flashlight *f, fonts_t *fonts )
{
    GameSummary *g = malloc( sizeof(GameSummary) );
    if( g == NULL ) {
        printf("Failed to allocate GameSummary\n" );
        exit(1);
    }
    memset( g, 0, sizeof(GameSummary) );


    g->f = f;
    g->vertice_group_background = text_background_quad( f );

    g->active_slot   = 0;
    g->inactive_slot = 1;
    g->fade_level[0] = 100; // Slot 0 is full brightness
    g->fade_level[1] = 0;   // Slot 1 is transparent
    g->fade_delay_frame_count = 0;
    g->redraw = 0;
    g->state = GAMESUMMARY_STATIC;
    
    g->fonts = fonts;

    g->vertice_groups[0] = flashlight_vertice_create_group( f, 5);

    g->desc_pane[0]   = textpane_init( g->vertice_groups[0], g->fonts->fontatlas, fonts->body, VP_RIGHT - VP_LEFT - 6 * 2, 270, 1280, 720, TEXT_LEFT ); 
    g->title_pane[0]  = textpane_init( g->vertice_groups[0], g->fonts->fontatlas, fonts->heading, 529, 270, 1280, 720, TEXT_LEFT );
    g->label_pane[0]  = textpane_init( g->vertice_groups[0], g->fonts->fontatlas, fonts->italic,   90, 270, 1280, 720, TEXT_RIGHT );
    g->credit_pane[0] = textpane_init( g->vertice_groups[0], g->fonts->fontatlas, fonts->body,    459, 270, 1280, 720, TEXT_LEFT );
    g->yearl_pane [0] = textpane_init( g->vertice_groups[0], g->fonts->fontatlas, fonts->italic,   50,  30, 1280, 720, TEXT_RIGHT );
    g->year_pane  [0] = textpane_init( g->vertice_groups[0], g->fonts->fontatlas, fonts->body,     50,  30, 1280, 720, TEXT_LEFT  );

    g->uniforms.texture_fade       = flashlight_shader_assign_uniform( f, "u_fade"               );
    g->uniforms.translation_matrix = flashlight_shader_assign_uniform( f, "u_translation_matrix" );

    flashlight_event_subscribe( f, (void*)g, gamesummary_event, FLEventMaskAll );

    g->display_enabled = 1;
    g->language = LANG_EN;

    return g;
}

// ----------------------------------------------------------------------------------

void
gamesummary_finish( GameSummary **g ) {
    free(*g);
    *g = NULL;
}

// ----------------------------------------------------------------------------------
//
void
gamesummary_set_language( GameSummary *g, language_t lang )
{
    g->language = lang & LANG_MASK;

    if( g->fade_delay_frame_count > 0 ) return;

    load_game_text( g, g->active_slot );
}

// ----------------------------------------------------------------------------------
int
gamesummary_process( GameSummary *g )
{
    switch(g->state) {
        case GAMESUMMARY_STATIC:
            g->redraw = 0;
            break;

        case GAMESUMMARY_FADE_DELAY:
            {
                if( g->fade_delay_frame_count > 0 ) { // FIXME Make time based
                    g->fade_delay_frame_count--;
                    break;
                }

                g->state = GAMESUMMARY_STATIC;

                load_game_text( g, g->active_slot );
            }
            break;
    }
    return 1;
}

// ----------------------------------------------------------------------------------
static void
gamesummary_event( void *self, FLEvent *event )
{
    GameSummary *g = (GameSummary *)self;

    switch( event->type ) {
        case Redraw:
            {
                if( g->display_enabled ) {
                    FLVerticeGroup *cg;
                    glUniformMatrix4fv( g->uniforms.translation_matrix,1, GL_FALSE, flashlight_translate( 0.0, 0.0, 0.0 ) );

                    glUniform1f( g->uniforms.texture_fade, 1.0 );
                    cg = g->vertice_group_background;
                    flashlight_shader_blend_off(g->f);
                    glDrawElements(GL_TRIANGLES, cg->indicies_len, GL_UNSIGNED_SHORT, flashlight_get_indicies(cg) ); // Background

                    cg = g->vertice_groups[g->active_slot];
                    flashlight_shader_blend_on(g->f);
                    glDrawElements(GL_TRIANGLES, cg->indicies_len, GL_UNSIGNED_SHORT, flashlight_get_indicies(cg) );
                }
            }
            break;
        case Process:
            {
                gamesummary_process( g );
            }
            break;
        case Setting:
            {
                TSFEventSetting *es = (TSFEventSetting*)event;
                switch( es->id ) {
                    case Language: gamesummary_set_language( g, FL_EVENT_DATA_TO_TYPE(language_t, es->data) ); break;
                    default: break;
                }
            }
            break;
        case UserEvent:
            {
                TSFUserEvent *ue = (TSFUserEvent*)event;
                switch(ue->user_type) {
                    case CarouselMove:
                        break;
                    
                    case GameSelectionChange:
                        {
                            game_t *game = (game_t *)ue->data;
                            if( g->game != game ) {
                                g->game = game;
                                g->fade_delay_frame_count = 10;
                                g->state = GAMESUMMARY_FADE_DELAY;
                            } else {
                                g->fade_level[ g->inactive_slot ] = 0;
                                g->state = GAMESUMMARY_STATIC; // Ah, we've the user has gone round the carousel and selected the same game
                            }
                        }
                        break;
                case FullScreenClaimed:
                    g->display_enabled = 0;
                    break;

                case FullScreenReleased:
                    g->display_enabled = 1;
                    break;
                }
            }
            break;
        default:
            break;
    }
}

// ----------------------------------------------------------------------------------

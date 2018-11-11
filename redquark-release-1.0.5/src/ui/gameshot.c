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

#include "gameshot.h"
#include "gamelibrary.h"
#include "user_events.h"
#include "path.h"
#include "event.h"

#define WIDTH 1280
#define HEIGHT 720

#define IMG_HEIGHT  200
#define IMG_WIDTH   320
#define TEX_WIDTH   IMG_WIDTH
#define TEX_HEIGHT (IMG_HEIGHT * IMG_SLOTS) // We have two imagesd stacked in the texture

#define HW ((IMG_WIDTH/2)*2)
#define HH ((IMG_HEIGHT/2)*2)

#define SAFE 21

#define X (1280/2) - 312 + 21 + 7
#define Y (720/2)  + 151 - 21

static FLVertice border_verticies[] = {
    {    X - HW -  9, Y + HH -  5,   0, 15},  // Top Left   15x 15
    {    X - HW -  9, Y + HH + 10,   0,  0},
    {    X - HW +  6, Y + HH -  5,  15, 15},
    {    X - HW +  6, Y + HH + 10,  15,  0},

    {    X - HW +  6, Y + HH +  0,  15, 10},  // Top  1 x 9
    {    X - HW +  6, Y + HH + 10,  15,  0},
    {    X + HW -  3, Y + HH +  0,  16, 10},
    {    X + HW -  3, Y + HH + 10,  16,  0},

    {    X + HW -  6, Y + HH -  5, 16, 15},  // Top Right  24 x 15
    {    X + HW -  6, Y + HH + 10, 16,  0},
    {    X + HW + 17, Y + HH -  5, 40, 15},
    {    X + HW + 17, Y + HH + 10, 40,  0},

    {    X - HW -  9, Y - HH +  6,  0, 16},  // Left 9 x 1
    {    X - HW -  9, Y + HH -  5,  0, 15},
    {    X - HW +  1, Y - HH +  6, 10, 16},
    {    X - HW +  1, Y + HH -  5, 10, 15},

    {    X + HW +  0, Y - HH +  7, 22, 16},  // Right 18 x 1
    {    X + HW +  0, Y + HH -  5, 22, 15},
    {    X + HW + 17, Y - HH +  7, 40, 16},
    {    X + HW + 17, Y + HH -  5, 40, 15},

    {    X - HW -  9, Y - HH - 10,   0, 33},  // Bottom Left   15x 27
    {    X - HW -  9, Y - HH +  7,   0, 16},
    {    X - HW +  6, Y - HH - 10,  15, 33},
    {    X - HW +  6, Y - HH +  7,  15, 16},

    {    X - HW +  6, Y - HH - 16,  16, 40},  // Bottom  1 x 18
    {    X - HW +  6, Y - HH +  1,  16, 22},
    {    X + HW -  3, Y - HH - 16,  17, 40},
    {    X + HW -  3, Y - HH +  1,  17, 22},

    {    X + HW -  6, Y - HH - 10,  16, 33},  // Bottom Right  24 x 27
    {    X + HW -  6, Y - HH +  7,  16, 16},
    {    X + HW + 18, Y - HH - 10,  40, 33},
    {    X + HW + 18, Y - HH +  7,  40, 16},
};

static FLVertice gameshot_verticies[] = {
    {  X - HW, Y - HH,   0, 200},
    {  X - HW, Y + HH,   0,   0},
    {  X + HW, Y - HH, 320, 200},
    {  X + HW, Y + HH, 320,   0},
};

#define TOGGLE_SPEED_S  6
#define FADE_RATE_IN    (10)
#define FADE_RATE_OUT   (6)

#define FRAME_PERIOD    20 // In ms
extern unsigned long frame_time_ms;

// TODO Bring in game shot border decoration into this controller, and center at the origin. Use transform to place at desired location.

static void gameshot_event( void *self, FLEvent *event );
static void load_game_shot( Gameshot *g, int slot, const char* filename );

// ----------------------------------------------------------------------------------
// Populates GameShot->vertice_groups 
static void
add_image_quads( Flashlight *f, Gameshot *gs )
{
    FLVertice vi[4];

    FLTexture *tx = flashlight_load_texture( f, get_path("/usr/share/the64/ui/textures/menu_atlas.png") );
    if( tx == NULL ) { printf("Error tx\n"); exit(1); }
    flashlight_register_texture( tx, GL_NEAREST );

    int slot = 0;
    for( slot = 0; slot < IMG_SLOTS; slot++ ) {
        int j;
        for(j = 0; j < sizeof(gameshot_verticies)/sizeof(FLVertice); j++ ) {
            int w = gameshot_verticies[j].s; 
            int h = gameshot_verticies[j].t + IMG_HEIGHT * slot;
            vi[j].x = X_TO_GL(WIDTH,      gameshot_verticies[j].x);
            vi[j].y = Y_TO_GL(HEIGHT,     gameshot_verticies[j].y);
            vi[j].s = S_TO_GL(TEX_WIDTH,  w); // + 1.0; // Map to 1.0-1.9 range = Gameshot texture
            vi[j].t = T_TO_GL(TEX_HEIGHT, h);
            vi[j].u = 0.1;
        }

        FLVerticeGroup *vg = flashlight_vertice_create_group(f, 8);
        gs->vertice_groups[slot] = vg;

        flashlight_vertice_add_quad2D( vg, &vi[0], &vi[1], &vi[2], &vi[3] );
    }

    FLVerticeGroup *vg = flashlight_vertice_create_group(f, 8);
	gs->frame_vertice_group = vg;

    int i,j;
    for(i = 0; i < sizeof(border_verticies)/sizeof(FLVertice); i += 4 ) {
        for(j = 0; j < 4; j++ ) {

            vi[j].x = X_TO_GL(WIDTH,      border_verticies[i + j].x);
            vi[j].y = Y_TO_GL(HEIGHT,     border_verticies[i + j].y);
            vi[j].s = S_TO_GL(tx->width,  border_verticies[i + j].s); // + 2.0; // Map to 2.0-2.9 range = Decorations texture
            vi[j].t = T_TO_GL(tx->height, border_verticies[i + j].t);
            vi[j].u = 0.2;
        }
        flashlight_vertice_add_quad2D( vg, &vi[0], &vi[1], &vi[2], &vi[3] );
    }

    return;
}

// ----------------------------------------------------------------------------------
// Initialise in-game shot display
Gameshot *
gameshot_init( Flashlight *f )
{
    Gameshot *g = malloc( sizeof(Gameshot) );
    if( g == NULL ) {
    }
    memset( g, 0, sizeof(Gameshot) );

    g->f = f;

    // We don't create a memory backed texture as we load images directly to GPU, and
    // only need a memory buffer to set the initial black state of the texture.
    FLTexture *tx = flashlight_create_null_texture( f, TEX_WIDTH, TEX_HEIGHT, GL_RGBA );
    if( tx == NULL ) { printf("Error tx\n"); exit(1); }
    flashlight_register_texture( tx, GL_NEAREST ); // Assigns to first free texture unit
    flashlight_shader_attach_teture( f, tx, "u_GameshotUnit" );

    // Temporarily create a buffer to set the initial state of the texture, fill it with 
    // opaque black, and update texture in GPU.
    uint8_t *m = (uint8_t*)malloc(tx->size); // Must be 8bit size due to size check in while()
    uint32_t *p = (uint32_t*)m;
    while( (unsigned char*)p < (m + tx->size) ) { 
        *p = 0xff000000;
        p++;
    }
    flashlight_texture_update( tx, 0, 0, TEX_WIDTH, TEX_HEIGHT, m );
    free(m); // Free temp buffer

    g->texture = tx;

    add_image_quads(f, g);

    g->active_slot   = 0;
    g->inactive_slot = 1;
    g->fade_level[0] = 100; // Slot 0 is full brightness
    g->fade_level[1] = 0;   // Slot 1 is transparent
    g->fade_delay_target_t = 0;
    g->redraw = 0;
    g->state = GAMESHOT_STATIC;

    g->uniforms.texture_fade       = flashlight_shader_assign_uniform( f, "u_fade"               );
    g->uniforms.texture_fade_grey  = flashlight_shader_assign_uniform( f, "u_grey"               );
    g->uniforms.translation_matrix = flashlight_shader_assign_uniform( f, "u_translation_matrix" );

    flashlight_shader_attach_teture( f, tx, "u_DecorationUnit" );

    flashlight_event_subscribe( f, (void*)g, gameshot_event, FLEventMaskAll );

    return g;
}

// ----------------------------------------------------------------------------------
// Load a in-game shot image into the texture. Two slots are provided, indicated by
// 'slot' and the images are verticall stacked in the texture.
static void
load_game_shot( Gameshot *g, int slot, const char* filename )
{
    if( slot >= IMG_SLOTS ) {
        printf("Error: a maximum of %d image slots are supported\n", IMG_SLOTS);
        return;
    }

    int len = 0;
    void *img = flashlight_mapfile( (char *)filename, &len);
    if( img == NULL ) {
        fprintf(stderr, "Error: Could not load gameshot image file [%s]. Permissions or missing?\n", (char *)filename);
        return;
    }

    const FLImageData *raw = flashlight_png_get_raw_image( img, len );

    if( raw == NULL ) {
        fprintf(stderr, "Error: Could not import gameshot image [%s]. Invalid file.\n", (char *)filename);
        return;
    }

	int y = slot ? IMG_HEIGHT * slot: 0;

    if( raw->width == IMG_WIDTH && raw->height == IMG_HEIGHT ) {
        // Maybe TODO allow smaller images and change s,t in the vertex
        flashlight_texture_update( g->texture, 0, y, raw->width, raw->height, raw->data );
    } else {
        fprintf(stderr, "Error: gameshot image [%s] has dimensions %d:%d which do not match desired %d:%d\n", (char *)filename, raw->width, raw->height, IMG_WIDTH, IMG_HEIGHT );
    }

    flashlight_png_release_raw_image(raw);
    flashlight_unmapfile( img, len );
}

// ----------------------------------------------------------------------------------

void
gameshot_finish( Gameshot **g ) {
    free(*g);
    *g = NULL;
}

// ----------------------------------------------------------------------------------
#define AS (g->active_slot)
#define IS (g->inactive_slot)
int
gameshot_process( Gameshot *g )
{
    switch(g->state) {
        case GAMESHOT_STATIC:
            g->redraw = 0;

            if( g->shot_toggle_time_ms > 0 && g->shot_toggle_time_ms < frame_time_ms ) {
                g->current_image = (g->current_image + 1) % g->game->screen_image_count;
                g->fade_delay_target_t = frame_time_ms;
                g->state = GAMESHOT_FADE_DELAY;
            }
            break;

        case GAMESHOT_FADE_OUT:
            if( g->fade_target_t[ AS ] < frame_time_ms + FRAME_PERIOD ) g->fade_level[ AS ] = g->target_fade_level[ AS ];
            else g->fade_level[ AS ] += (FRAME_PERIOD * (g->target_fade_level[ AS ] - g->fade_level[ AS ] )) / (g->fade_target_t[ AS ] - frame_time_ms);

            g->redraw = 1;
            g->shot_toggle_time_ms = 0; // Stop toggling between game shots

            if( g->fade_level[ g->active_slot ] <= g->target_fade_level[ AS ] ) {
                g->fade_level[ g->active_slot ] = g->target_fade_level[ AS ];
                g->state = GAMESHOT_STATIC;
            }
            break;
        case GAMESHOT_FADE_IN:
            {
                // Continue to fade out completely what is now the inactive slot
                //
                if( g->fade_target_t[ IS ] < frame_time_ms + FRAME_PERIOD ) g->fade_level[ IS ] = g->target_fade_level[ IS ];
                else g->fade_level[ IS ] += (FRAME_PERIOD * (g->target_fade_level[ IS ] - g->fade_level[ IS ] )) / (g->fade_target_t[ IS ] - frame_time_ms);

                if( g->fade_target_t[ AS ] < frame_time_ms + FRAME_PERIOD ) g->fade_level[ AS ] = g->target_fade_level[ AS ];
                else g->fade_level[ AS ] += (FRAME_PERIOD * (g->target_fade_level[ AS ] - g->fade_level[ AS ] )) / (g->fade_target_t[ AS ] - frame_time_ms);

                g->redraw = 1;

                if( g->fade_level[ AS ] >= g->target_fade_level[ AS ] ) {
                    g->fade_level[ AS ] = g->target_fade_level[ AS ];
                    g->state = GAMESHOT_STATIC;
                    //notify_load_complete( g );
                }
            }
            break;
        case GAMESHOT_FADE_DELAY:
            {
                if( g->fade_delay_target_t < frame_time_ms + FRAME_PERIOD ) {
                    int s = g->active_slot;
                    g->active_slot = g->inactive_slot;
                    g->inactive_slot = s;

                    if( g->game->screen_image_count > 0 ) {
                        load_game_shot( g, g->active_slot, g->game->screen_img[ g->current_image] );
                        g->shot_toggle_time_ms = (TOGGLE_SPEED_S * 50 * 20) + frame_time_ms;
                    }

                    g->fade_target_t[ AS ]     = frame_time_ms + (FRAME_PERIOD * FADE_RATE_IN);
                    g->fade_level[ AS ]        = 0;
                    g->target_fade_level[ AS ] = 100;
                    g->state = GAMESHOT_FADE_IN;
                }
            }
            break;
        case GAMESHOT_SWAP_IMAGE:

            break;
    }
    return 1;
}

// ----------------------------------------------------------------------------------
static void
gameshot_event( void *self, FLEvent *event )
{
    static int display_enabled = 1;

    Gameshot *g = (Gameshot *)self;

    switch( event->type ) {
        case Redraw:
            {
                if( display_enabled) {
                    FLVerticeGroup *cg;
                    glUniformMatrix4fv( g->uniforms.translation_matrix,1, GL_FALSE, flashlight_translate( 0.0, 0.0, 0.0 ) );

                    glUniform1i( g->uniforms.texture_fade_grey, (int)1 );

                    if( g->state != GAMESHOT_STATIC ) {
                        flashlight_shader_blend_on(g->f);
                    }
                    else {
                        flashlight_shader_blend_off(g->f);
                    }

                    // We only need to render the inactive slot if we are fading in the active slot
                    // At this point the inactive slot will be at a grey level (though it's fade level is
                    // zero, the texture_fade_grey uniform forces the shader to render it as a 
                    // semi-transparent grey.
                    if( g->state == GAMESHOT_FADE_IN ) {
                        cg = g->vertice_groups[g->inactive_slot];
                        glUniform1f( g->uniforms.texture_fade, (float)g->fade_level[g->inactive_slot] / 100 );

                        glDrawElements(GL_TRIANGLES, cg->indicies_len, GL_UNSIGNED_SHORT, flashlight_get_indicies(cg) );

                        // The active slot is faded in "transparent" mode, not grey mode.
                        glUniform1i( g->uniforms.texture_fade_grey, (int)0 ); 
                    }

                    cg = g->vertice_groups[g->active_slot];
                    glUniform1f( g->uniforms.texture_fade, (float)g->fade_level[g->active_slot] / 100 );
                    glDrawElements(GL_TRIANGLES, cg->indicies_len, GL_UNSIGNED_SHORT, flashlight_get_indicies(cg) );

                    flashlight_shader_blend_on(g->f);
                    glUniform1f( g->uniforms.texture_fade, 1.0 );
                    cg = g->frame_vertice_group;
                    glDrawElements(GL_TRIANGLES, cg->indicies_len, GL_UNSIGNED_SHORT, flashlight_get_indicies(cg) );
                }
            }
            break;
        case Process:
            {
                gameshot_process( g );
            }
            break;
        case UserEvent:
            {
                TSFUserEvent *ue = (TSFUserEvent*)event;
                switch(ue->user_type) {
                    case CarouselMove:
                        if(g->state != GAMESHOT_FADE_OUT) {
                            g->fade_target_t[ AS ]     = frame_time_ms + (FRAME_PERIOD * FADE_RATE_OUT);
                            g->target_fade_level[ AS ] = 0;
                            g->state = GAMESHOT_FADE_OUT;
                        }
                        break;
                    
                    case GameSelectionChange:
                        {
                            game_t *game = (game_t *)ue->data;
                            if( g->game != game ) {
                                g->game = game;
                                g->fade_delay_target_t = frame_time_ms + (FRAME_PERIOD * 15);
                                g->current_image = g->game->screen_image_count - 1;
                                g->state = GAMESHOT_FADE_DELAY;
                            } else {
                                g->fade_level[ g->inactive_slot ] = 0;
                                g->state = GAMESHOT_FADE_IN; // Ah, we've the user has gone round the carousel and selected the same game
                            }
                        }
                        break;
                    case FullScreenClaimed:
                        display_enabled = 0;
                        g->shot_toggle_time_ms = 0;
                        break;
                    case FullScreenReleased:
                        display_enabled = 1;
                        g->shot_toggle_time_ms = (TOGGLE_SPEED_S * 50 * 20) + frame_time_ms;
                        break;
                }
            }
            break;
        case FocusChange:
            {
                TSFFocusChange *fce = (TSFFocusChange*)event;
                if( fce->who == g ) {
                    // We have the focus
                    g->focus = 1;
                    display_enabled = 1;
                } else {
                    // We've lost focus
                    g->focus = 0;
                }
            }
            break;
        default:
            break;
    }
}

// ----------------------------------------------------------------------------------

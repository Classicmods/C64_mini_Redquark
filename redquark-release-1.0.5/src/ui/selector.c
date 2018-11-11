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

#include "selector.h"
#include "user_events.h"
#include "path.h"

#define WIDTH 1280
#define HEIGHT 720

#define T 9
#define W 119
#define H 170
#define X ((WIDTH/2)  - (W/2))
#define Y ((HEIGHT/2) - (H/2))

typedef enum {
    Move_None    = 0,
    Move_Dir_X   = 1,
    Move_Dir_Y   = 2,
    Move_Scale_X = 4,
    Move_Scale_Y = 8,
} MoveType;

// DO NOT CHANGE THIS ORDER - It is manipulated at runtime to set size
static FLVertice selector_verticies[] = {
    {      X - T, Y+H + T,  480,  0},  // Top Left
    {      X - T, Y+H    ,  480,  9},
    {      X,     Y+H + T,  489,  0},
    {      X,     Y+H    ,  489,  9},
    
    {  W + X    , Y+H + T,  498,  0},  // Top Right
    {  W + X    , Y+H    ,  498,  9},
    {  W + X + T, Y+H + T,  507,  0},
    {  W + X + T, Y+H    ,  507,  9},

    {  W + X    , Y      ,  498, 18},  // Bottom Right
    {  W + X    , Y   - T,  498, 27},
    {  W + X + T, Y      ,  507, 18},
    {  W + X + T, Y   - T,  507, 27},

    {      X - T, Y      ,  480, 18},  // Bottom Left
    {      X - T, Y   - T,  480, 27},
    {      X    , Y      ,  489, 18},
    {      X    , Y   - T,  489, 27},

    {      X - T, Y+H    ,  480,  9},  // Left
    {      X - T,   Y    ,  480, 18},
    {      X,     Y+H    ,  489,  9},
    {      X,       Y    ,  489, 18},

    {  W + X    , Y+H    ,  498,  9},  // Right
    {  W + X    ,   Y    ,  498, 18},
    {  W + X + T, Y+H    ,  507,  9},
    {  W + X + T,   Y    ,  507, 18},

    {      X    , Y+H + T,  489,  0},  // Top
    {      X    , Y+H    ,  489,  9},
    {  W + X    , Y+H + T,  498,  0},
    {  W + X    , Y+H    ,  498,  9},

    {      X    , Y      ,  489, 18},  // Bottom
    {      X    , Y   - T,  489, 27},
    {  W + X    , Y      ,  498, 18},
    {  W + X    , Y   - T,  498, 27},
};

#define FRAME_PERIOD 20 // In ms
extern unsigned long frame_time_ms;

static void _set_position_and_size( Selector *s, int x, int y, int size_x, int size_y );
static void selector_event( void *self, FLEvent *event );

// ----------------------------------------------------------------------------------
//
static FLVerticeGroup *
add_image_quad( Flashlight *f, FLTexture *tx ) 
{
    FLVertice vi[sizeof(selector_verticies)/sizeof(FLVertice)];

    FLVerticeGroup *vgb = flashlight_vertice_create_group(f, 9);

    int i,j;
    for(i = 0; i < sizeof(selector_verticies)/sizeof(FLVertice); i += 4 ) {
        for(j = 0; j < 4; j++ ) {

            vi[j].x = X_TO_GL(WIDTH,      selector_verticies[i + j].x);
            vi[j].y = Y_TO_GL(HEIGHT,     selector_verticies[i + j].y);
            vi[j].s = S_TO_GL(tx->width,  selector_verticies[i + j].s);
            vi[j].t = T_TO_GL(tx->height, selector_verticies[i + j].t);
            vi[j].u = 0.2;
        }
        flashlight_vertice_add_quad2D( vgb, &vi[0], &vi[1], &vi[2], &vi[3] );
    }
    return vgb;
}

// ----------------------------------------------------------------------------------
// Initialise in-game shot display
Selector *
selector_init( Flashlight *f )
{
    Selector *d = malloc( sizeof(Selector) );
    if( d == NULL ) {
    }
    memset( d, 0, sizeof(Selector) );

    d->f = f;

    FLTexture *tx = flashlight_load_texture( f, get_path("/usr/share/the64/ui/textures/menu_atlas.png") );
    if( tx == NULL ) { printf("Error tx\n"); exit(1); }
    flashlight_register_texture( tx, GL_NEAREST );
    flashlight_shader_attach_teture( f, tx, "u_DecorationUnit" );

    d->texture = tx;
    d->vertice_group = add_image_quad(f, tx);

    d->uniforms.texture_fade       = flashlight_shader_assign_uniform( f, "u_fade"               );
    d->uniforms.translation_matrix = flashlight_shader_assign_uniform( f, "u_translation_matrix" );

    d->is_hires = 0;
    d->param.hires = 0;

    d->x = d->param.target_x = 639;
    d->y = d->param.target_y =  92 + H / 2;
    d->size_x = W;
    d->size_y = H;
    d->set_width  = W;
    d->set_height = H;

    d->param.enabled = 0;

    d->stackp = 0;

    _set_position_and_size( d, d->x, d->y, d->size_x, d->size_y );
    selector_set_hires( d );

    flashlight_event_subscribe( f, (void*)d, selector_event, FLEventMaskAll );

    return d;
}

// ----------------------------------------------------------------------------------

void
selector_finish( Selector **g ) {
    free(*g);
    *g = NULL;
}

// ----------------------------------------------------------------------------------

void
selector_set_position( Selector *s, int ox, int oy, int sx, int sy )
{
    s->x      = s->param.target_x      = ox;
    s->y      = s->param.target_y      = oy;
    s->size_x = s->param.target_size_x = sx;
    s->size_y = s->param.target_size_y = sy;

    _set_position_and_size( s, s->x, s->y, s->size_x, s->size_y );
}

// ----------------------------------------------------------------------------------
// move_time_fu is the time period allocated to move to target position and size, expressed
// in units of frame-time (which is 20ms @ 50Hz).
void
selector_set_target_position( Selector *s, int ox, int oy, int sx, int sy, uint32_t move_time_fu )
{
    s->param.target_x      = ox;
    s->param.target_y      = oy;
    s->param.target_size_x = sx;
    s->param.target_size_y = sy;

    s->param.move_time_fu   = move_time_fu;

    if( !(move_time_fu & S_NO_DECELERATE) && (move_time_fu > 1) ) {
        s->param.decel_count = 4;
    }
    move_time_fu &= ~S_NO_DECELERATE;

//printf("Set target position x %d, in time %d\n", ox, move_time_fu * FRAME_PERIOD);
    s->param.target_t = frame_time_ms + (move_time_fu * FRAME_PERIOD);
    // Apply deceleration to end of travel if sufficient frameperiods are specified.
}

// ----------------------------------------------------------------------------------
//
void
selector_reset_to_target( Selector *s )
{
    s->x      = s->param.target_x;
    s->y      = s->param.target_y;
    s->size_x = s->param.target_size_x;
    s->size_y = s->param.target_size_y;

    _set_position_and_size( s, s->x, s->y, s->size_x, s->size_y );
}

// ----------------------------------------------------------------------------------
//
void
selector_set_hires( Selector *s )
{
    if( s->is_hires ) return;

    s->is_hires = s->param.hires = 1;

    int i = 0;
    FLVertice *v[4];

    while( i < 8) {
        flashlight_vertice_get_quad2D( s->vertice_group, i, &v[0], &v[1], &v[2], &v[3] );

        v[0]->t += T_TO_GL( s->texture->height, 29 );
        v[1]->t += T_TO_GL( s->texture->height, 29 );
        v[2]->t += T_TO_GL( s->texture->height, 29 );
        v[3]->t += T_TO_GL( s->texture->height, 29 );

        i++;
    }
}

// ----------------------------------------------------------------------------------
//
void
selector_set_lowres( Selector *s )
{
    if( !s->is_hires ) return;

    s->is_hires = s->param.hires = 0;

    int i = 0;
    FLVertice *v[4];

    while( i < 8) {
        flashlight_vertice_get_quad2D( s->vertice_group, i, &v[0], &v[1], &v[2], &v[3] );

        v[0]->t -= T_TO_GL( s->texture->height, 29 );
        v[1]->t -= T_TO_GL( s->texture->height, 29 );
        v[2]->t -= T_TO_GL( s->texture->height, 29 );
        v[3]->t -= T_TO_GL( s->texture->height, 29 );

        i++;
    }
}

// ----------------------------------------------------------------------------------
//
static void
selector_set_size( Selector *s, int w, int h )
{
    FLVertice *v[4];
#define OX (WIDTH/2)
#define OY (HEIGHT/2)

    if( (s->set_width == w) && (s->set_height == h) ) return;

    s->set_width  = w;
    s->set_height = h;
    
    flashlight_vertice_get_quad2D( s->vertice_group, 0, &v[0], &v[1], &v[2], &v[3] );
    v[0]->x = v[1]->x = X_TO_GL(WIDTH,  OX - (w/2) - T ); // Top Left
    v[2]->x = v[3]->x = X_TO_GL(WIDTH,  OX - (w/2)     );
    v[0]->y = v[2]->y = Y_TO_GL(HEIGHT, OY + (h/2) + T );
    v[1]->y = v[3]->y = Y_TO_GL(HEIGHT, OY + (h/2)     );

    flashlight_vertice_get_quad2D( s->vertice_group, 1, &v[0], &v[1], &v[2], &v[3] );
    v[0]->x = v[1]->x = X_TO_GL(WIDTH,  OX + (w/2)     ); // Top Right
    v[2]->x = v[3]->x = X_TO_GL(WIDTH,  OX + (w/2) + T );
    v[0]->y = v[2]->y = Y_TO_GL(HEIGHT, OY + (h/2) + T );
    v[1]->y = v[3]->y = Y_TO_GL(HEIGHT, OY + (h/2)     );

    flashlight_vertice_get_quad2D( s->vertice_group, 2, &v[0], &v[1], &v[2], &v[3] );
    v[0]->x = v[1]->x = X_TO_GL(WIDTH,  OX + (w/2)     ); // Bottom Right
    v[2]->x = v[3]->x = X_TO_GL(WIDTH,  OX + (w/2) + T );
    v[0]->y = v[2]->y = Y_TO_GL(HEIGHT, OY - (h/2)     );
    v[1]->y = v[3]->y = Y_TO_GL(HEIGHT, OY - (h/2) - T );

    flashlight_vertice_get_quad2D( s->vertice_group, 3, &v[0], &v[1], &v[2], &v[3] );
    v[0]->x = v[1]->x = X_TO_GL(WIDTH,  OX - (w/2) - T ); // Bottom Left
    v[2]->x = v[3]->x = X_TO_GL(WIDTH,  OX - (w/2)     );
    v[0]->y = v[2]->y = Y_TO_GL(HEIGHT, OY - (h/2)     );
    v[1]->y = v[3]->y = Y_TO_GL(HEIGHT, OY - (h/2) - T );

    flashlight_vertice_get_quad2D( s->vertice_group, 4, &v[0], &v[1], &v[2], &v[3] );
    v[0]->x = v[1]->x = X_TO_GL(WIDTH,  OX - (w/2) - T ); // Left
    v[2]->x = v[3]->x = X_TO_GL(WIDTH,  OX - (w/2)     );
    v[0]->y = v[2]->y = Y_TO_GL(HEIGHT, OY + (h/2)     );
    v[1]->y = v[3]->y = Y_TO_GL(HEIGHT, OY - (h/2)     );

    flashlight_vertice_get_quad2D( s->vertice_group, 5, &v[0], &v[1], &v[2], &v[3] );
    v[0]->x = v[1]->x = X_TO_GL(WIDTH,  OX + (w/2)     ); // Right
    v[2]->x = v[3]->x = X_TO_GL(WIDTH,  OX + (w/2) + T );
    v[0]->y = v[2]->y = Y_TO_GL(HEIGHT, OY + (h/2)     );
    v[1]->y = v[3]->y = Y_TO_GL(HEIGHT, OY - (h/2)     );

    flashlight_vertice_get_quad2D( s->vertice_group, 6, &v[0], &v[1], &v[2], &v[3] );
    v[0]->x = v[1]->x = X_TO_GL(WIDTH,  OX - (w/2)     ); // Top
    v[2]->x = v[3]->x = X_TO_GL(WIDTH,  OX + (w/2)     );
    v[0]->y = v[2]->y = Y_TO_GL(HEIGHT, OY + (h/2) + T );
    v[1]->y = v[3]->y = Y_TO_GL(HEIGHT, OY + (h/2)     );

    flashlight_vertice_get_quad2D( s->vertice_group, 7, &v[0], &v[1], &v[2], &v[3] );
    v[0]->x = v[1]->x = X_TO_GL(WIDTH,  OX - (w/2)     ); // Bottom
    v[2]->x = v[3]->x = X_TO_GL(WIDTH,  OX + (w/2)     );
    v[0]->y = v[2]->y = Y_TO_GL(HEIGHT, OY - (h/2)     );
    v[1]->y = v[3]->y = Y_TO_GL(HEIGHT, OY - (h/2) - T );
}

// ----------------------------------------------------------------------------------
static MoveType
test_for_move( Selector *s )
{
    int move = Move_None;

    if( s->param.target_x != s->x  ) move |= Move_Dir_X;
    if( s->param.target_y != s->y  ) move |= Move_Dir_Y;

    if( s->param.target_size_x != s->size_x  ) move |= Move_Scale_X;
    if( s->param.target_size_y != s->size_y  ) move |= Move_Scale_Y;

    return move;
}

// ----------------------------------------------------------------------------------
int
selector_process( Selector *s )
{
    int move = 0; 

    if( !s->param.enabled ) return 0;

    move = test_for_move( s );

    int tdiff = s->param.target_t - frame_time_ms;
    if( s->param.decel_count > 0 && tdiff <= 30 ) {
        s->param.target_t += 30; //(s->param.target_t - frame_time_ms);
        s->param.decel_count--;
    }

    if( move & Move_Dir_X ) {
        if( s->param.target_t < frame_time_ms + FRAME_PERIOD ) s->x = s->param.target_x;
        else s->x += (FRAME_PERIOD * (s->param.target_x - s->x)) / (s->param.target_t - frame_time_ms);
    }
    if( move & Move_Dir_Y ) {
        if( s->param.target_t < frame_time_ms + FRAME_PERIOD ) s->y = s->param.target_y;
        else s->y += (FRAME_PERIOD * (s->param.target_y - s->y)) / (s->param.target_t - frame_time_ms);
    }
    if( move & Move_Scale_X ) {
        if( s->param.target_t < frame_time_ms + FRAME_PERIOD ) s->size_x = s->param.target_size_x;
        else s->size_x += (FRAME_PERIOD * (s->param.target_size_x - s->size_x)) / (s->param.target_t - frame_time_ms);
    }
    if( move & Move_Scale_Y ) {
        if( s->param.target_t < frame_time_ms + FRAME_PERIOD ) s->size_y = s->param.target_size_y;
        else s->size_y += (FRAME_PERIOD * (s->param.target_size_y - s->size_y)) / (s->param.target_t - frame_time_ms);
    }

    if( move != Move_None ) {
        _set_position_and_size( s, s->x, s->y, s->size_x, s->size_y );

        // See if the next movement will be the last, and send the "stop" event.
        // Doing it a frame early allows subscribes to respond and request that the selector
        // continues to move without missing a frame.
        move = test_for_move( s );
        if( !move ) {
            // Send event that selector has stopped moving
            TSFUserEvent e = { {UserEvent, (void*)s}, SelectorStop };
            flashlight_event_publish( s->f,(FLEvent*)&e );
        }
    }

    s->moving = move;


    return 1;
}

// ----------------------------------------------------------------------------------
//
static void
_set_position_and_size( Selector *s, int x, int y, int size_x, int size_y )
{
    s->scale_x = (float)s->size_x / W;
    s->scale_y = (float)s->size_y / H;

    float dest_offset_x = X_TO_GL(WIDTH,  s->x);    // Offset from origin to target position (scaled center)
    float dest_offset_y = Y_TO_GL(HEIGHT, s->y);    // Offset from origin to target position (scaled center)
    s->translate_x = dest_offset_x;
    s->translate_y = dest_offset_y;

    selector_set_size( s, size_x, size_y );
}

// ----------------------------------------------------------------------------------
static void
selector_event( void *self, FLEvent *event )
{
    Selector *s = (Selector *)self;

    switch( event->type ) {
        case Redraw:
            {
                if( !s->param.enabled ) break;
                if( s->translate_y < -1.1 ) break; // We're certanly off screen.

                FLVerticeGroup *cg = s->vertice_group;

                glUniformMatrix4fv( s->uniforms.translation_matrix,1, GL_FALSE, 
                        flashlight_translate( s->translate_x, s->translate_y, 1.0 ) );

                glUniform1f( s->uniforms.texture_fade, 1.0f );
                flashlight_shader_blend_on(s->f);
                glDrawElements(GL_TRIANGLES, cg->indicies_len, GL_UNSIGNED_SHORT, flashlight_get_indicies(cg) );
            }
            break;
        case Process:
            {
                // FIXME Selector is always active!
                selector_process( s );
            }
            break;

        case UserEvent:
            switch(((TSFUserEvent*)event)->user_type) {
                case FullScreenClaimed:
                    break;
                case FullScreenReleased:
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
//
int
selector_enable( Selector *s )
{
    return ++(s->param.enabled);
}

// ----------------------------------------------------------------------------------
// 
int
selector_disable( Selector *s )
{
    return (s->param.enabled > 0 ) ? --(s->param.enabled) : 0;
}

// ----------------------------------------------------------------------------------
int
selector_stack_push( Selector *s, void *owner )
{
    if( owner == s->param.owner ) return -2;

    if( s->stackp == SELECTOR_STACK_MAX ) return -1;
    memcpy( s->stack + (s->stackp)++, &(s->param), sizeof(SelectorParams) );

    selector_enable( s ); 

    s->param.owner = owner;

    return s->stackp;
}

// ----------------------------------------------------------------------------------
int
selector_stack_pop( Selector *s, void * owner )
{
    if( owner != s->param.owner ) return -2;

    SelectorParams *p = &(s->param);
    SelectorParams  op = {0};

    memcpy( &op, p, sizeof(SelectorParams) ); // Keep a copy of what the selector _was_

    // Crucially, alter the x,y scale_x,scale_y speed etc of the selector
    //
    if( s->stackp == 0 ) return -1;
    memcpy( p, s->stack + --(s->stackp), sizeof(SelectorParams) );

    if( p->hires ) selector_set_hires( s );
    else selector_set_lowres( s );

    selector_set_target_position( s, p->target_x, p->target_y, p->target_size_x, p->target_size_y, p->move_time_fu );

    // Post event notifying interested consumers of what the selector used to be
    TSFUserEvent e = { {UserEvent, (void*)s}, SelectorPop, (void *)&op };
    flashlight_event_publish( s->f,(FLEvent*)&e );

    return s->stackp;
}

// ----------------------------------------------------------------------------------
//
int
selector_owned_by( Selector *s, void *owner )
{
    return owner == s->param.owner;
}

// ----------------------------------------------------------------------------------
//
int
selector_owned_by_and_ready( Selector *s, void *owner )
{
    return (owner == s->param.owner) && !s->moving;
}

// ----------------------------------------------------------------------------------
//
int
selector_moving( Selector *s )
{
    return s->moving;
}

// ----------------------------------------------------------------------------------

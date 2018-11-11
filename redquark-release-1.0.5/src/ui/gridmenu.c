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
#include "tsf_joystick.h"

#include "mainmenu.h"
#include "gridmenu.h"
#include "user_events.h"
#include "ui_sound.h"

typedef enum {
    Move_None      = 0,
    Move_Dir_X     = 1,
    Move_Dir_Y     = 2,
    Move_Scale_W   = 4,
    Move_Scale_H   = 8,
    _Move_Negative = 16,

    // Special types for menu_move_selection()
    Move_Right     = Move_Dir_X,
    Move_Left      = Move_Dir_X | _Move_Negative,
    Move_Up        = Move_Dir_Y,
    Move_Down      = Move_Dir_Y | _Move_Negative,
} MoveType;

#define POINTER_FRAME_WIDTH     25  // Which is the width
#define POINTER_FRAME_HEIGHT    39
#define POINTER_ANIMATE_TIME_FU 20  // Number of frame units ( @20ms, 50 = 1 sec)

#define POINTER_FRAMES 6
static int pointer_frame_x[] = {
     45, 70,  
     70, 95,  
     95, 120, 
    120, 145,
     95, 120,  
     70, 95
};

static FLVertice pointer_vert[] = {
    {  0,  0, 45, 39 },
    {  0, 39, 45, 0  },
    { 25,  0, 70, 39 },
    { 25, 39, 70, 0  },
};

#define FRAME_PERIOD 20 // In ms
extern unsigned long frame_time_ms;

static void menu_event( void *self, FLEvent *event );

static void set_pointer_position_frame( menu_t *m );
static int test_for_pointer_animate( menu_t *m );
// ----------------------------------------------------------------------------------
// 
void
menu_add_pointer_verticies( menu_t *m, FLVerticeGroup *vg, FLTexture *menu_atlas_tx )
{
    m->awidth  = menu_atlas_tx->width;
    m->aheight = menu_atlas_tx->height;

    m->pointer_vertex_ix = vg->index_len; // Index of the pointer quad we're adding
    m->vertice_group     = vg;

    FLVertice vi[4];
    vi[0].u = vi[1].u = vi[2].u = vi[3].u = 0.2; // Select the menu atlas texture FIXME ENUM!

    int i,j;
    for(i = 0; i < sizeof(pointer_vert)/sizeof(FLVertice); i += 4 ) {
        for(j = 0; j < 4; j++ ) {

            vi[j].x = X_TO_GL(m->dwidth,  pointer_vert[i + j].x);
            vi[j].y = Y_TO_GL(m->dheight, pointer_vert[i + j].y);
            vi[j].s = S_TO_GL(m->awidth,  pointer_vert[i + j].s);
            vi[j].t = T_TO_GL(m->aheight, pointer_vert[i + j].t);
        }
        flashlight_vertice_add_quad2D( vg, &vi[0], &vi[1], &vi[2], &vi[3] );
    }
    m->pointer_frame    = 10.0;
    test_for_pointer_animate( m );// Makes sure fraem is valie
    set_pointer_position_frame( m );

    return;
}

// ----------------------------------------------------------------------------------
// Setting a marked item that is out of range ( negative or >= count ) disables the
// item pointer.
void
menu_set_marked_item( menu_t *m, int item )
{
    if( m->vertice_group == NULL ) return;

    if( item >= m->count ) item = -1;

    m->marked_item            = item;
    m->pointer_target_time_ms = 0;
    m->pointer_frame          = 10.0;
}

// ----------------------------------------------------------------------------------
//
static void 
set_pointer_position_frame( menu_t *m )
{
    if( m->marked_item < 0 ) return;

    menu_item_t *i = &(m->items[ m->marked_item ]);
    //float yoff = m->target_y - (m->dheight / 2);

    int x, y, left;
    switch( i->dim.marker_pos ) {
        case MARKER_NONE:
            return;
            break;

        case MARKER_LEFT:
            x    = i->x - (POINTER_FRAME_WIDTH / 2) - (i->dim.selector_w / 2) - POINTER_FRAME_WIDTH; // position pointer to the left of selector
            left = 0;
            break;
        case MARKER_RIGHT:
            x     = i->x + (POINTER_FRAME_WIDTH / 2) + (i->dim.selector_w / 2);; // position pointer to the right of selector
            left = 1;
            break;

        default:
            return;
    }

    y = i->y - POINTER_FRAME_HEIGHT / 2;

    FLVertice *v[4];
    flashlight_vertice_get_quad2D( m->vertice_group, m->pointer_vertex_ix, &v[0], &v[1], &v[2], &v[3] );

    v[0]->x = v[1]->x = X_TO_GL(m->dwidth, x);
    v[2]->x = v[3]->x = X_TO_GL(m->dwidth, x + POINTER_FRAME_WIDTH);

    v[0]->y = v[2]->y = Y_TO_GL(m->dheight, y);
    v[1]->y = v[3]->y = Y_TO_GL(m->dheight, y + POINTER_FRAME_HEIGHT);

    int pfi = (int)(m->pointer_frame) * 2;

    v[0]->s = v[1]->s = S_TO_GL(m->awidth,  pointer_frame_x[ pfi + left ] );
    v[2]->s = v[3]->s = S_TO_GL(m->awidth,  pointer_frame_x[ pfi + (1 - left) ] );
}

// ----------------------------------------------------------------------------------
//
void
set_item_visibility( menu_t *m, int id, int visible )
{
    if( visible && !m->items[id].enabled ) {
        m->enabled_count++;
        m->items[id].enabled = 1;
        return;
    }
    if( !visible && m->items[id].enabled ) {
        m->enabled_count--;
        m->items[id].enabled = 0;
        return;
    }
}

// ----------------------------------------------------------------------------------
//
void
menu_set_selector_move_rate( menu_t *m, uint32_t rate )
{
    m->selector_move_rate = rate;
}

// ----------------------------------------------------------------------------------
//
static void
menu_build_items( menu_t *m )
{
    menuSelectCallback ms_cb;
    int enabled;

    menu_item_t *i = m->items;
    int max_width = 0;

    // Initialise menu items. Items are indexed left-to-right, top-to-bottom
    int id;
    for( id = 0; id < m->count; id++ ) 
    {
        // Callback to get menu item X and Y, w, h
        m->create_menu_item( m, m->private, id, &enabled, &(i->dim), &ms_cb );

        i->menu_select_cb = ms_cb;
        i->enabled        = enabled;
        if( enabled ) m->enabled_count++;

        if( i->dim.w > max_width ) max_width = i->dim.w;

        i->delta_w  = 0.f;
        i->w        = (float)i->dim.w;
        i->target_w = (float)i->dim.w;
        i->delta_h  = 0.f;
        i->h        = (float)i->dim.h;
        i->target_h = (float)i->dim.h;

        i->delta_x  = 0.f;
        i->x        = (float)i->dim.x;
        i->target_x = (float)i->dim.x;
        i->delta_y  = 0.f;
        i->y        = (float)i->dim.y;
        i->target_y = (float)i->dim.y;

        i++;
    }

    if( m->flags & MENU_FLAG_MATCH_ITEM_WIDTH ) {
        // Fixup items so selectors all have the same width.
        i = m->items;
        for( id = 0; id < m->count; id++ ) {
            if( i->dim.w < max_width ) {

                int diff = max_width - i->dim.w;
                i->dim.w = max_width;

                i->w        = (float)i->dim.w;
                i->target_w = (float)i->dim.w;
                i->x        = (float)i->dim.x + diff / 2;
                i->target_x = (float)i->dim.x + diff / 2;

                i->dim.selected_w = i->dim.w;
                i->dim.selector_w = i->dim.w + 2;
            }
            i++;
        }
    }
}

// ----------------------------------------------------------------------------------
//
menu_t *
menu_init( int dwidth, int dheight, int count_w, int count_h, Flashlight *f, menuInitCallback mi_cb, menuSetPositionCallback msp_cb, menuLanguageCallback mlang_cb, menu_flags_t flags, void *private )
{
    if( mi_cb == NULL || msp_cb == NULL ) return NULL;

    menu_t *m = malloc( sizeof(menu_t) );
    if( m == NULL ) {
        return NULL;
    }
    memset((void*)m, 0, sizeof(menu_t));

    m->f = f;

    int total_count = count_w * count_h;

    menu_item_t *i = malloc( sizeof(menu_item_t) * total_count );
    if( i == NULL ) {
        free( m );
        return NULL;
    }
    memset((void*)i, 0, sizeof(menu_item_t) * total_count );

    m->items = i;
    m->count = total_count;
    m->count_w = count_w;
    m->count_h = count_h;
    m->current = 0;
    m->enabled_count = 0;
    m->private = private;
    m->dwidth  = dwidth;
    m->dheight = dheight;
    m->selector_move_rate = 5;
    m->enabled_ox  = dwidth/2;   // FIXME Set to be -100 (probably!) Leave for testing!
    m->enabled_oy  = dheight/2;
    m->disabled_ox = dwidth/2;
    m->disabled_oy = dheight/2;
    m->marked_item = -1;
    m->set_item_position = msp_cb;
    m->set_menu_language = mlang_cb;
    m->create_menu_item  = mi_cb;
    m->flags = flags;

    m->x = m->y = m->target_x = m->target_y = -100.f;

    menu_build_items( m );

    flashlight_event_subscribe( f, (void*)m, menu_event, FLEventMaskAll );

    return m;
}

// ----------------------------------------------------------------------------------

void
menu_finish( menu_t **m ) {
    free((*m)->items);
    free(*m);
    *m = NULL;
}

// ----------------------------------------------------------------------------------
// Set the menu origin's X and Y when enabled
void 
menu_set_enabled_position ( menu_t *m, int x, int y )
{
    m->enabled_ox = x;
    m->enabled_oy = y;
}

// ----------------------------------------------------------------------------------
// Set the menu origin's X and Y when disabled
void 
menu_set_disabled_position( menu_t *m, int x, int y )
{
    m->disabled_ox = x;
    m->disabled_oy = y;
}

// ----------------------------------------------------------------------------------
//
void
menu_reset_current( menu_t *m )
{
    int ix = -1;

    do {
        ix++;
        m->current = ix;
        
    } while( m->items[ix].enabled == 0 && ix < m->count );

    if( ix == m->count ) ix = 0;
    m->current = ix;
}

// ----------------------------------------------------------------------------------

void
menu_set_selector_offscreen( menu_t *m )
{
    menu_item_t *i = &(m->items[ m->current ]);
    // Sets the initial off-screen position of selector
    float yoff = m->target_y - (m->dheight / 2);
    mainmenu_selector_set_position( i->x, i->y + yoff, i->dim.selector_w, i->dim.selector_h ); // OFFSCREEN
}


// ----------------------------------------------------------------------------------
//
void
menu_set_target_position( menu_t *m, int ox, int oy, int move_rate )
{
    m->target_x = ox;
    m->target_y = oy;

    m->delta_x = (m->target_x - m->x ) / move_rate;
    m->delta_y = (m->target_y - m->y ) / move_rate;

    m->moving = 1;

    // menu_item_t *i = &(m->items[ m->current ]);
    //
    // Disable this, as the selector is now free-roaming. If we need to track the selector to the
    // menu coming into/out of view, then it's rate, initial and final position should be matched.
    // // This makes the selector track the menu item as the menu comes in view.
    // float yoff = m->target_y - (m->dheight / 2);
    // mainmenu_selector_set_target_position( i->x, i->y + yoff, i->dim.selector_w, i->dim.selector_h, move_rate );
}

// ----------------------------------------------------------------------------------
//
void
menu_set_position( menu_t *m, int ox, int oy )
{
    m->x = m->target_x = ox;
    m->y = m->target_y = oy;

    m->delta_x = 0.f;
    m->delta_y = 0.f;

    //menu_item_t *i = &(m->items[ m->current ]);
    // 
    // Disable this, as the selector is now free-roaming. If we need to track the selector to the
    // // This makes the selector track the menu item as the menu comes in and out of view.
    // float yoff = m->target_y - (m->dheight / 2);
    // mainmenu_selector_set_target_position( i->x, i->y + yoff, i->dim.selector_w, i->dim.selector_h, move_rate );
}

// ----------------------------------------------------------------------------------
//
static void
set_item_size( menu_t *m, int id, int w, int h, int move_rate )
{
    menu_item_t *i = &(m->items[id]);

    i->target_w = w;
    i->delta_w = (i->target_w - i->w ) / move_rate;

    i->target_h = h;
    i->delta_h = (i->target_h - i->h ) / move_rate;
}

// ----------------------------------------------------------------------------------
// Animates the identified menu item, moving it into or out of view, and moving the other
// menu items to make room.
#ifdef NOT_YET
static void
animate_item( menu_t *m, int idx, int move_rate )
{
    update_item_coords ( m, 0 ); // re-shuffle item coords so that the appearing/disappearing item is taken into account.

    menu_item_t *i = &(m->items[ idx ]);

    if( m->items[idx].enabled ) {
        i->x = i->target_x;
        i->w = 0;
        set_item_size( m, idx, i->dim.width, i->dim.height, move_rate );
    } else {
        i->target_x = i->x;
        set_item_size( m, idx, 0, i->dim.height, move_rate );
    }

    int id;
    for( id = 0; id < m->count; id++ ) {
        i = &(m->items[ id ]);
        i->delta_x = (i->target_x - i->x ) / move_rate;
    }
    if( m->focus ) {
        i = &(m->items[m->current]);
        float yoff = m->target_y - (m->dheight / 2);
        mainmenu_selector_set_target_position( i->x, i->y + yoff, i->dim.selector_width, i->dim.selector_height, m->selector_move_rate ); // 5 = move rate, lower the faster
    }
}
#endif

// ----------------------------------------------------------------------------------
    
static MoveType
test_for_selection_move( menu_t *m, int id )
{
    int move = Move_None;

    menu_item_t *i = &(m->items[ id ]);

    if( i->target_x != i->x && i->delta_x != 0.0 ) move |= Move_Dir_X;
    if( i->target_y != i->x && i->delta_y != 0.0 ) move |= Move_Dir_Y;
    if( i->target_w != i->w && i->delta_w != 0.0 ) move |= Move_Scale_W;
    if( i->target_h != i->h && i->delta_h != 0.0 ) move |= Move_Scale_H;

    return move;
}
// ----------------------------------------------------------------------------------
//
static MoveType
test_for_menu_move( menu_t *m )
{
    int move = Move_None;
    
    if( m->target_x != m->x && m->delta_x != 0.0 ) move |= Move_Dir_X;
    if( m->target_y != m->y && m->delta_y != 0.0 ) move |= Move_Dir_Y;

    return move;
}

// ----------------------------------------------------------------------------------
//
static int
test_for_pointer_animate( menu_t *m )
{
    int op = (int)(m->pointer_frame);

    if( m->pointer_target_time_ms < frame_time_ms + FRAME_PERIOD ) {
        m->pointer_frame = 0;
        m->pointer_target_time_ms = frame_time_ms + (POINTER_ANIMATE_TIME_FU * FRAME_PERIOD);
    }
    else {
        m->pointer_frame += (FRAME_PERIOD * ( POINTER_FRAMES - m->pointer_frame)) / (m->pointer_target_time_ms - frame_time_ms);
        if( (int)(m->pointer_frame) >= POINTER_FRAMES) m->pointer_frame = POINTER_FRAMES - 1;
    }

    return op != (int)(m->pointer_frame);
}

// ----------------------------------------------------------------------------------
//
static void
_set_menu_position( menu_t *m, int x, int y)
{
    float dest_offset_x = X_TO_GL(m->dwidth,  x);    // Offset from origin to target position (scaled center)
    float dest_offset_y = Y_TO_GL(m->dheight, y);    // Offset from origin to target position (scaled center)
    m->translate_x = dest_offset_x;
    m->translate_y = dest_offset_y;
}

// ----------------------------------------------------------------------------------
// 
static int
menu_process( menu_t *m )
{
    int id;
    int move = Move_None; 

    // TODO Do nothing if menu is not visible

    // First deal with the overal menu position etc
    move = test_for_menu_move( m );

    if( move != Move_None ) {
        if( move & Move_Dir_X ) {
            m->x += m->delta_x;
            if( m->delta_x < 0 && m->x < m->target_x ) m->x = m->target_x;
            if( m->delta_x > 0 && m->x > m->target_x ) m->x = m->target_x;
        }
        if( move & Move_Dir_Y ) {
            m->y += m->delta_y;
            if( m->delta_y < 0 && m->y < m->target_y ) m->y = m->target_y;
            if( m->delta_y > 0 && m->y > m->target_y ) m->y = m->target_y;
        }

        _set_menu_position( m, m->x, m->y );
    }

    m->moving = move;
   
    // Second deal with the individual menu item positions / scale
    //
    // FIXME THIS IS REALLY INEFFICIENT FIXME
    // WE'RE ALWAYS LOOPING AND CHECKING FOR MOVE, EVEN IF NO RECENT MENU LEFT/RIGHT/UP/DOWN
    for( id = 0; id < m->count; id++ ) {

        menu_item_t *i = &(m->items[id]);

        move = test_for_selection_move( m, id );

        if( move & Move_Scale_W ) {
            i->w += i->delta_w;
            if( i->delta_w < 0 && i->w < i->target_w ) i->w = i->target_w;
            if( i->delta_w > 0 && i->w > i->target_w ) i->w = i->target_w;
        }
        if( move & Move_Scale_H ) {
            i->h += i->delta_h;
            if( i->delta_h < 0 && i->h < i->target_h ) i->h = i->target_h;
            if( i->delta_h > 0 && i->h > i->target_h ) i->h = i->target_h;
        }
        if( move & Move_Dir_X ) {
            i->x += i->delta_x;
            if( i->delta_x < 0 && i->x < i->target_x ) i->h = i->target_x;
            if( i->delta_x > 0 && i->x > i->target_x ) i->h = i->target_x;
        }
        if( move & Move_Dir_Y ) {
            i->x += i->delta_y;
            if( i->delta_y < 0 && i->y < i->target_y ) i->y = i->target_y;
            if( i->delta_y > 0 && i->y > i->target_y ) i->y = i->target_y;
        }

        if( move != Move_None ) {
            // invoke callback to set menu item's position
            //
            m->set_item_position( m, m->private, id, i->x, i->y, i->w, i->h );
        }
    }

    // Pointer animation
    //
    if( m->marked_item > -1 && test_for_pointer_animate( m ) ) {
        set_pointer_position_frame( m );
    }

    if( m->wait_for_focus_loss && !m->moving ) {
        m->wait_for_focus_loss = 0;
        TSFUserEvent e = { {UserEvent, (void*)m}, MenuClosed, m->private  }; // Private is menu owner
        flashlight_event_publish( m->f,(FLEvent*)&e );
    }

    return move;
}

// ----------------------------------------------------------------------------------
// Moves the selector and expands/shrinks items
// If last    > -1, then that item is set to it's normal unexpanded size.
// if current > -1, then that item is expanded to its selected size, and the selector
//                  moved to cover it.
static void
move_selector( menu_t *m, int last, int current )
{
    if( current >= 0 ) {
        menu_item_t *i = &(m->items[current]);
        float yoff = m->target_y - (m->dheight / 2);

        mainmenu_selector_set_target_position( i->x, i->y + yoff, i->dim.selector_w, i->dim.selector_h, m->selector_move_rate );
        m->selector_moving = 1;

        set_item_size( m, current, i->dim.selected_w, i->dim.selected_h, 5 );
    }

    if( last >= 0 ) {
        menu_item_t *i = &(m->items[last]);
        set_item_size( m, last, i->dim.w, i->dim.h, 5 );
    }
}

// ----------------------------------------------------------------------------------
// Returns > 0 if the navigation moved left
static int
_move_left( menu_t *m, int *x, int *y )
{
    int moved = 0;
    if( m->count_w <= 1 ) return 0; // There are no horizontal items

    if( *x <= 0 ) {
        // If there are more than 2 items across the screen, then allow wrap back to the right
        if( m->count_w > 2 ) { *x = m->count_w - 1; moved = 1; }
    } else {
        (*x)--;
        moved = 1;
    }
    return moved;
}

// ----------------------------------------------------------------------------------
// Returns > 0 if the navigation moved right
static int
_move_right( menu_t *m, int *x, int *y )
{
    int moved = 0;
    if( m->count_w <= 1 ) return 0; // There are no horizontal items

    if( *x >= (m->count_w - 1) ) {
        // If there are more than 2 items across the screen, then allow wrap back to the right
        if( m->count_w > 2 ) { *x = 0; moved = 1; }
    } else {
        (*x)++;
        moved = 1;
    }
    return moved;
}

// ----------------------------------------------------------------------------------
// Returns > 0 if the navigation moved up
static int
_move_up( menu_t *m, int *x, int *y )
{
    int moved = 0;
    if( m->count_h <= 1 ) return 0; // There are no vertical items
    if( *y <= 0 ) {
        // If there are more than 2 items down the screen, then allow wrap back to the bottom
        if( m->count_h > 2 ) { *y = m->count_h - 1; moved = 1; }
    } else {
        (*y)--;
        moved = 1;
    }
    return moved;
}

// ----------------------------------------------------------------------------------
// Returns > 0 if the navigation moved down
static int
_move_down( menu_t *m, int *x, int *y )
{
    int moved = 0;
    if( m->count_h <= 1 ) return 0; // There are no vertical items

    if( *y >= (m->count_h - 1) ) {
        // If there are more than 2 items down the screen, then allow wrap back to the top
        if( m->count_h > 2 ) { *y = 0; moved = 1; }
    } else {
        (*y)++;
        moved = 1;
    }
    return moved;
}

// ----------------------------------------------------------------------------------
//
static void
menu_move_sel( menu_t *m, MoveType direction )
{
    // Find active menu to the left
    int oi, i, moved;
    i = oi = m->current;

    // Split into x and y menu grid location
    int x = i % m->count_w;
    int y = i / m->count_w;
    do {
        switch( direction ) {
            case Move_Left:  moved = _move_left ( m, &x, &y ); break;
            case Move_Right: moved = _move_right( m, &x, &y ); break;
            case Move_Up:    moved = _move_up   ( m, &x, &y ); break;
            case Move_Down:  moved = _move_down ( m, &x, &y ); break;
            default: break;
        }
        if( !moved ) return; // Navigation did/could not move, so nothing more to do

        i = y * m->count_w + x; // Convert back to an id

        // If we've landed on an enabled item, select it, else continue moving
        // XXX This will loop-forever if all items on a menu line are disabled! XXX
        if( m->items[ i ].enabled ) {
            m->current = i;
            break;
        }
    } while(1);
    if( m->current == oi ) return;
    
    move_selector( m, oi, m->current );
    if( !m->silent) ui_sound_play_flip();
}

// ----------------------------------------------------------------------------------
//
static void
menu_stop( menu_t *m )
{
}

// ----------------------------------------------------------------------------------
//
void
menu_sound_on( menu_t *m )
{
    m->silent = 0;
}

// ----------------------------------------------------------------------------------
//
void
menu_sound_off( menu_t *m )
{
    m->silent = 1;
}

// ----------------------------------------------------------------------------------
//
static void
menu_select( menu_t *m )
{
    menu_item_t *i = &(m->items[m->current]);

    if( m->current == m->marked_item ) return;

    i->menu_select_cb( m, m->private, m->current );
}

// ----------------------------------------------------------------------------------
//
void
menu_change_language( menu_t *m, language_t lang )
{
    if( !m->set_menu_language ) return;

    m->set_menu_language( m, m->private, lang & LANG_MASK);
    menu_build_items( m );
}

// ----------------------------------------------------------------------------------
static void
menu_event( void *self, FLEvent *event )
{
    menu_t *m = (menu_t *)self;

    if( event->type != Setting && !m->wait_for_focus_loss && !m->focus && !m->moving ) return;

    switch( event->type ) {
        case Navigate:
            {
                jstype_t type = ((TSFEventNavigate*)event)->type;
                if( type != JOY_JOYSTICK ) break;

                if( !mainmenu_selector_owned_by_and_ready( m ) ) {
                    m->had_select = 0;
                    break;
                }
                
                if( m->moving ) break; // We're coming in or out of scope, so ignore inputs

                int direction = ((TSFEventNavigate*)event)->direction;

                direction &= ~_INPUT_BUTTON;

                if( m->had_select &&  (direction & (_INPUT_TRIG | _INPUT_SHLD)) ) break;

                if( !m->had_select && (direction & (_INPUT_TRIG | _INPUT_SHLD)) ) {
                    menu_select( m ); 
                    m->had_select = 1;
                    return;
                }
                m->had_select = 0;

                if( m->selector_moving ) break; // If selector moving, don't allow direction change

                if( !(direction & (_INPUT_AXIS | _INPUT_DIG)) )  menu_stop   ( m );

                if     ( direction & INPUT_LEFT  ) menu_move_sel( m, Move_Left  );
                else if( direction & INPUT_RIGHT ) menu_move_sel( m, Move_Right );
                if     ( direction & INPUT_UP    ) menu_move_sel( m, Move_Up    );
                else if( direction & INPUT_DOWN  ) menu_move_sel( m, Move_Down  );
                break;
            }
        case Process:
            {
                menu_process( m );
            }
            break;
        case Setting:
            {
                TSFEventSetting *es = (TSFEventSetting*)event;
                switch( es->id ) {
                    case Language: 
                        menu_change_language( m, FL_EVENT_DATA_TO_TYPE(language_t, es->data) );
                        break;
                    default: break;
                }
            }
            break;
        case UserEvent:
            switch(((TSFUserEvent*)event)->user_type) {
                case SelectorStop:
                    m->selector_moving = 0;
                    break;
            }
            break;

        default:
            break;
    }
}

// ----------------------------------------------------------------------------------
//
float const * const
menu_get_translation( menu_t *m )
{
    return flashlight_translate( m->translate_x, m->translate_y, 0.0 );
}

// ----------------------------------------------------------------------------------
//
void
menu_enable( menu_t *m, int rate )
{
    m->focus = 1;

    mainmenu_selector_push( m );

    menu_set_target_position( m, m->enabled_ox, m->enabled_oy, rate );
    //menu_set_selector_move_rate( m, rate );
    //mainmenu_selector_enable();

    move_selector( m, -1, m->current );

    // TODO remember selector move speed, set it to the same as the show-menu speed and
    // restore selector move speed once menu is in view
}

// ----------------------------------------------------------------------------------
// A rate of 0 meanus just shutdown, don't animate anything
void
menu_disable( menu_t *m, int rate )
{
    menu_item_t *i = &(m->items[m->current]);
    set_item_size( m, m->current, i->dim.w, i->dim.h, 5 );

    mainmenu_selector_pop( m );

    menu_set_target_position( m, m->disabled_ox, m->disabled_oy, rate );

    if( rate > 0 ) m->moving = 1; // Forces the menu to be processed at least once

    if( m->focus == 0 ) return;

    if( rate > 0 ) {
        //move_selector( m, m->current, -1 );
        m->wait_for_focus_loss = 1;
    }

    m->focus = 0;
    
    // TODO remember selector move speed, set it to the same as the show-menu speed and
    // restore selector move speed once menu is in view
}

// ----------------------------------------------------------------------------------
// Restore the selector to the menu's current selector position. This is necessary
// when we've returned from a submenu
void
menu_restore_selector( menu_t *m )
{
    move_selector( m, -1, m->current );
}

// ----------------------------------------------------------------------------------
// This just marks the current but does not move the selector
void
menu_set_current_item( menu_t *m, int ix )
{
    if( ix < 0 || ix >= m->count ) return;

    m->current = ix;
}

// ----------------------------------------------------------------------------------
// This moves the selector as well as setting current
void
menu_initialise_current_item( menu_t *m, int ix )
{
    if( ix < 0 || ix >= m->count ) return;

    menu_set_current_item( m, ix );

    move_selector( m, -1, m->current );
    if( !m->silent) ui_sound_play_flip();
}

// ----------------------------------------------------------------------------------

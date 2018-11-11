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

#include "textpane.h"

#include "tsf_joystick.h"
#include "ui_sound.h"
#include "user_events.h"
#include "gridmenu.h"
#include "path.h"
#include "settings.h"
#include "gamelibrary.h"
#include "gamelist.h"
#include "mainmenu.h"
#include "suspend_menu.h"
#include "suspendresume.h"

#define WIDTH 1280
#define HEIGHT 720

#define FY 91
#define HW (WIDTH/2)
#define HH (HEIGHT/2)

#define MW  WIDTH
#define MH  HEIGHT
#define MHH (MH/2)
#define MHW (MW/2)

static int default_slot_tex_width;
static int default_slot_tex_height;
static int atlas_width;
static int atlas_height;

static void suspend_menu_event( void *self, FLEvent *event );

typedef void (*menuSelectCallback)     ( menu_t *m, void *private, int id );

static void _select_dummy(menu_t *m, void *private, int id );

#define SLOT_IMG_WIDTH    384
#define SLOT_IMG_HEIGHT   272

#define WDT 200
#define WHT 150
#define HWDT (WDT/2)
#define HWHT (WHT/2)
#define SPAN 1150
#define SPC ((SPAN - (WDT * 4)) / 5)
#define LHS ((WIDTH - SPAN)/2)

#define BW 1 // Border width of each slot image

// Each image is actually 384x272, including the border. Therefore we offset the s,t and adjust
// widths to keep the center 320x200. The displayed image is then scaled to WDTxWHT
static fs_item_t menu_item[] = { 
    {   32, 35, 320, 200, LHS+SPC+(SPC+WDT)*0+WDT/2, 360, MARKER_NONE, "1", _select_dummy },
    {  416, 35, 320, 200, LHS+SPC+(SPC+WDT)*1+WDT/2, 360, MARKER_NONE, "2", _select_dummy },
    {  800, 35, 320, 200, LHS+SPC+(SPC+WDT)*2+WDT/2, 360, MARKER_NONE, "3", _select_dummy },
    { 1184, 35, 320, 200, LHS+SPC+(SPC+WDT)*3+WDT/2, 360, MARKER_NONE, "4", _select_dummy },
};

static void _load_state  ( SuspendMenu *smenu );
static void _save_state  ( SuspendMenu *smenu );

static int text_background_index[4] = {0};

#define SE 30
static FLVertice navigation_verticies[] = {
    {    0 + SE + 31 + 100,   0 + SE - 3, 563,  87}, // Save Icon
    {    0 + SE + 31 + 100,  29 + SE - 3, 563,  58},
    {   77 + SE + 31 + 100,   0 + SE - 3, 640,  87},
    {   77 + SE + 31 + 100,  29 + SE - 3, 640,  58},

    {    0 + SE + 31 + 200,   0 + SE - 3, 563,  58},  // Load icon
    {    0 + SE + 31 + 200,  28 + SE - 3, 563,  30},
    {   77 + SE + 31 + 200,   0 + SE - 3, 640,  58},
    {   77 + SE + 31 + 200,  28 + SE - 3, 640,  30},
};

#define ADD_QUAD() \
    for(j = 0; j < 4; j++ ) { \
        vi[j].x = X_TO_GL(WIDTH,        *(bc[ j * 4 + 0 ]) ); \
        vi[j].y = Y_TO_GL(HEIGHT,       *(bc[ j * 4 + 1 ]) ); \
        vi[j].s = S_TO_GL(default_slot_tex_width,  *(bc[ j * 4 + 2 ]) ); \
        vi[j].t = T_TO_GL(default_slot_tex_height, *(bc[ j * 4 + 3 ]) ); \
        vi[j].u = 0.2; \
    } \
    flashlight_vertice_add_quad2D( vg, &vi[0], &vi[1], &vi[2], &vi[3] );

// ----------------------------------------------------------------------------------
//
static void
add_navigation_icons( FullScreenMenu *fs_menu )
{
    FLVertice vi[4];
    
    FLVerticeGroup *vg = fs_menu->back_vertice_group;

    int half_height = fs_menu->active_height / 2;

    int i,j;
    for(i = 0; i < sizeof(navigation_verticies)/sizeof(FLVertice); i += 4 ) {
        for(j = 0; j < 4; j++ ) {

            int y = navigation_verticies[i + j].y;
            y = HH - half_height + y;

            vi[j].x = X_TO_GL(WIDTH,        navigation_verticies[i + j].x);
            vi[j].y = Y_TO_GL(HEIGHT,                                   y);
            vi[j].s = S_TO_GL(atlas_width,  navigation_verticies[i + j].s);
            vi[j].t = T_TO_GL(atlas_height, navigation_verticies[i + j].t);
            vi[j].u = 0.2;
        }
        flashlight_vertice_add_quad2D( vg, &vi[0], &vi[1], &vi[2], &vi[3] );
    }

    return;
}

// ----------------------------------------------------------------------------------
//
static void
add_picture( FLVerticeGroup *vg, int id, fs_item_t *ic)
{
    int x0 = ic->ox - HWDT;
    int y0 = ic->oy - HWHT;
    int x1 = x0     + WDT; //ic->w;
    int y1 = ic->oy + WHT - HWHT; //ic->h - 75;

    int s0 = ic->s;
    int t0 = ic->t;
    int s1 = ic->s + ic->w;
    int t1 = ic->t + ic->h;
    int c[] = {x0,y0,s0,t1, x0,y1,s0,t0, x1,y0,s1,t1, x1,y1,s1,t0};

    FLVertice vi[4];
    int j;

    for(j = 0; j < 4; j++ ) {
        vi[j].x = X_TO_GL(WIDTH,        c[ j * 4 + 0 ]);
        vi[j].y = Y_TO_GL(HEIGHT,       c[ j * 4 + 1 ]);
        vi[j].s = S_TO_GL(default_slot_tex_width,  c[ j * 4 + 2 ]);
        vi[j].t = T_TO_GL(default_slot_tex_height, c[ j * 4 + 3 ]);
        vi[j].u = 0.5; // SuspendResume slot texture
    }
    flashlight_vertice_add_quad2D( vg, &vi[0], &vi[1], &vi[2], &vi[3] );

    // Draw a border
    //
    int bs0 =  4.1;
    int bt0 = 60.1;
    int bs1 =  4.9;
    int bt1 = 60.9; // Black palette entry in atlas

    int bx0 = x0 - BW;
    int bx1 = x0;
    int by0 = y0 - BW;
    int by1 = y1 + BW;
    int *bc[] = {&bx0,&by0,&bs0,&bt1, &bx0,&by1,&bs0,&bt0, &bx1,&by0,&bs1,&bt1, &bx1,&by1,&bs1,&bt0};
    ADD_QUAD() // Left border

    bx0 = x1;
    bx1 = x1 + BW;
    by0 = y0 - BW;
    by1 = y1 + BW;
    ADD_QUAD()  // Right border

    bx0 = x0;
    bx1 = x1;
    by0 = y0 - BW;
    by1 = y0;
    ADD_QUAD() // Bottom border

    bx0 = x0;
    bx1 = x1;
    by0 = y1;
    by1 = y1 + BW;
    ADD_QUAD()  // Top border

    text_background_index[ id ] = vg->index_len;

    // Black rectangle background for time duration
    bx0 = x1 - 73;
    bx1 = x1;
    by0 = y0 + 23;
    by1 = y0;
    ADD_QUAD()  // Time duration label background rectangle
}

// ----------------------------------------------------------------------------------
//
static void
init_menu_cb( menu_t *m, void *private, int id, int *enabled, menu_item_dim_t *dim, menuSelectCallback *ms_cb  )
{
    FullScreenMenu *fs_menu = (FullScreenMenu *)private;
    SuspendMenu     *smenu   = (SuspendMenu *)(fs_menu->owner_private);
    fs_item_t *ic = &(menu_item[id]);

    *enabled = 1;

    if( id == 0 ) {
        // This is a horrible way to do it - we need an additional hook in fsmenu to do static screen icons
        // We're painting the first menu item, that means either language has been changed or this is the
        // first time through - either way, the vertice list has been cleaned down, so needs to be rebuilt.
        add_navigation_icons( fs_menu );
    }

    add_picture( fs_menu->back_vertice_group, id, ic );

    dim->x = ic->ox;
    dim->y = ic->oy;

    dim->w = WDT; //ic->w;
    dim->h = WHT; // ic->h;

    dim->selected_w = dim->w;
    dim->selected_h = dim->h;

    dim->selector_w = dim->w;
    dim->selector_h = dim->h;

    dim->marker_pos = menu_item[id].marker_pos;
    
    smenu->slot_x[id] = dim->x;

    *ms_cb = menu_item[ id ].cb; // The select callback
}

// ----------------------------------------------------------------------------------
// Initialise in-game shot display
SuspendMenu *
suspend_menu_init( Flashlight *f, fonts_t *fonts )
{
    SuspendMenu *smenu = malloc( sizeof(SuspendMenu) );
    if( smenu == NULL ) {
        return NULL;
    }
    memset( smenu, 0, sizeof(SuspendMenu) );

    smenu->f = f;

    // Setup texture for four screen images
    //
    FLTexture *tx = flashlight_create_null_texture( f, SLOT_IMG_WIDTH * 4, SLOT_IMG_HEIGHT, GL_RGBA );
    if( tx == NULL ) { printf("Error tx\n"); exit(1); }
    flashlight_register_texture( tx, GL_LINEAR );
    flashlight_shader_attach_teture( f, tx, "u_SuspendUnit" );

    default_slot_tex_width  = tx->width;
    default_slot_tex_height = tx->height;

    FLTexture *atx = flashlight_load_texture( f, get_path("/usr/share/the64/ui/textures/menu_atlas.png") );
    if( atx == NULL ) { printf("Error tx\n"); exit(1); }

    atlas_width  = atx->width;
    atlas_height = atx->height;

    smenu->texture = tx;

    smenu->fonts = fonts;
    smenu->fs_menu = fsmenu_menu_init( f, fonts, init_menu_cb, (void *)smenu, "LS_TITLE", 4, 1, 360, 360 / 2, FS_MENU_FLAG_POINTER | FS_MENU_FLAG_NO_CLAIM_SCREEN | FS_MENU_FLAG_NO_LOGO, TRANSPARENT );
    smenu->fs_menu->name = "Suspend";

    menu_set_marked_item( smenu->fs_menu->menu, -1 ); // Disable marker

    flashlight_event_subscribe( f, (void*)smenu, suspend_menu_event, FLEventMaskAll );

    int i;
    for(i = 0; i < SAVE_LOAD_MAX_SLOTS; i++ ) {
        smenu->text[ i ] =  textpane_init( smenu->fs_menu->back_vertice_group, smenu->fs_menu->fonts->fontatlas, smenu->fs_menu->fonts->body, WDT, 60, WIDTH, HEIGHT, TEXT_RIGHT );
    }

    return smenu;
}

// ----------------------------------------------------------------------------------

void
suspend_menu_finish( SuspendMenu **m ) {
    free(*m);
    *m = NULL;
}

// ----------------------------------------------------------------------------------
//
static void
set_duration_label( SuspendMenu *smenu, int slot )
{
    static unsigned char buffer[32];

    if( slot >= SAVE_LOAD_MAX_SLOTS ) return;

    if( smenu->base_vert_count == 0 ) {
        // Remember the offset into the vertix group that variable text will start from
        smenu->base_vert_count = smenu->fs_menu->back_vertice_group->index_len;
    }

    int duration = smenu->slot_meta[ slot ].play_duration / 1000;
    int mins     = duration / 60;
    int seconds  = duration - ( mins * 60 );

    float left;

    if( duration > 0 ) {
        sprintf( (char *)buffer, "%0d.%02d", mins, seconds );
        textpane_t *tp =  smenu->text[ slot ];
        build_text( tp, buffer, 1, smenu->slot_x[ slot ] - WDT/2 - 3, 365 - WHT/2 );
        left = tp->min_x - S_TO_GL(WIDTH, 5);
    } 
    
    // Get the background rectangle quad
    FLVertice *v[4];
    flashlight_vertice_get_quad2D( smenu->fs_menu->back_vertice_group, text_background_index[slot], &v[0], &v[1], &v[2], &v[3] );
    
    if( duration == 0 ) left = v[2]->x; // This makes the rectangle zero width

    // Reposition the left hand edge of the text background rectangle to match the left edge
    // of the justified text with a 4pixel lefthand border
    v[0]->x = v[1]->x = left;

}

// ----------------------------------------------------------------------------------
static void
remove_page_text_verticies( SuspendMenu *smenu )
{
    if( smenu->base_vert_count == 0 ) return;

    int count = smenu->fs_menu->back_vertice_group->index_len - smenu->base_vert_count;
    if( count > 0 ) flashlight_vertice_remove_quad2D( smenu->fs_menu->back_vertice_group, count );
}

// ----------------------------------------------------------------------------------
//
static void
load_image_slots( SuspendMenu *smenu )
{
    suspend_load_slot_image   ( smenu->texture,      smenu->game, 0 );
    suspend_load_slot_image   ( smenu->texture,      smenu->game, 1 );
    suspend_load_slot_image   ( smenu->texture,      smenu->game, 2 );
    suspend_load_slot_image   ( smenu->texture,      smenu->game, 3 );

    suspend_load_slot_metadata( &smenu->slot_meta[0], smenu->game, 0 );
    suspend_load_slot_metadata( &smenu->slot_meta[1], smenu->game, 1 );
    suspend_load_slot_metadata( &smenu->slot_meta[2], smenu->game, 2 );
    suspend_load_slot_metadata( &smenu->slot_meta[3], smenu->game, 3 );

    remove_page_text_verticies( smenu );

    set_duration_label( smenu, 0 );
    set_duration_label( smenu, 1 );
    set_duration_label( smenu, 2 );
    set_duration_label( smenu, 3 );
}

// ----------------------------------------------------------------------------------
//
static void
suspend_menu_event( void *self, FLEvent *event )
{
    SuspendMenu *smenu = (SuspendMenu *)self;

    switch( event->type ) {
        case Navigate:
            {
                // FIXME Should be method calls into fsmenu to get these flags
                if( !smenu->fs_menu->active && !smenu->fs_menu->menu->moving ) return;
                if( !mainmenu_selector_owned_by_and_ready( smenu->fs_menu->menu ) ) break;

                int direction = ((TSFEventNavigate*)event)->direction;
                if( !smenu->fs_menu->menu->moving ) {
                    direction &= ~_INPUT_BUTTON;
                    if( direction & INPUT_A )     { _save_state  ( smenu ); break; }
                    if( direction & INPUT_GUIDE ) { _load_state  ( smenu ); break; }
                }
            }
            break;
        case UserEvent:
            {
                TSFUserEvent *ue = (TSFUserEvent*)event;
                switch(ue->user_type) {
                    case GameSelected:
                        {
                            SelectedGame *selected = (SelectedGame *)ue->data;
                            smenu->game = selected->game;
                        }
                        break;

                    case MenuClosed:
                        if( ue->data == smenu->fs_menu ) {

                            // The fs_menu is now disabled, restore selectors
                            mainmenu_selector_pop( smenu );
                            mainmenu_selector_reset_to_target();

                            TSFUserEvent ie = { {UserEvent, (void*)smenu}, InputClaim, FL_EVENT_DATA_FROM_INT(0) }; // Released
                            flashlight_event_publish( smenu->f,(FLEvent*)&ie );
                        
                            TSFUserEvent e = { {UserEvent, (void*)smenu}, Pause, FL_EVENT_DATA_FROM_INT(0)};
                            flashlight_event_publish( smenu->f,(FLEvent*)&e );
                        }
                        break;

                    default:
                        break;
                }
            }
            break;
        case Setting:
            {
                TSFEventSetting *es = (TSFEventSetting*)event;
                switch( es->id ) {
                    // If there is a language change, then reset base_vert_count
                    // so that it gets re-calculated correctly for the new language.
                    case Language: smenu->base_vert_count = 0; break;
                    default: break;
                }
            }
    
        default:
            break;
    }
}

// ----------------------------------------------------------------------------------
//
void
suspend_menu_enable( SuspendMenu *smenu )
{
    load_image_slots( smenu );

    // Set initial offscreen location for selector, as this is where it will
    // return to on close.
    mainmenu_selector_push( smenu );
    mainmenu_selector_set_position( 1280 / 2, -(720/2), 320, 200 ); 

    // Clear status of each slot
    memset( smenu->slot_action, 0, sizeof(smenu->slot_action) );
    
    // TODO need a "possible slot action" array too
 
    fsmenu_menu_enable( smenu->fs_menu, 1 ); // 1 = hires selector
}

// ----------------------------------------------------------------------------------
void
suspend_menu_disable( SuspendMenu *smenu )
{
    fsmenu_menu_disable( smenu->fs_menu );
}

// ----------------------------------------------------------------------------------
//
static void
_select_dummy(menu_t *m, void *private, int id )
{
}

// ----------------------------------------------------------------------------------
//
static void 
_load_state( SuspendMenu *smenu ) 
{
    int slot = smenu->fs_menu->menu->current;

    ui_sound_play_ding();

    // Resume game during "ding" sound
    if( resume_game( smenu->game, slot ) >= 0 ) {

        ui_sound_wait_for_ding();

        suspend_menu_disable( smenu );
    }
}

// ----------------------------------------------------------------------------------
static void 
_save_state( SuspendMenu *smenu )
{
    int slot = smenu->fs_menu->menu->current;

    if( smenu->slot_action[slot] == SLOT_SAVE ) return;

    smenu->slot_action[slot] = SLOT_SAVE;

    ui_sound_play_ding();

    suspend_game( smenu->game, slot ); // Save game while ding is being played

    ui_sound_wait_for_ding();

    // XXX Snapshot must have been saved before menu is disabled (and pause therefore released).
    suspend_menu_disable( smenu );
}

// ----------------------------------------------------------------------------------
// end

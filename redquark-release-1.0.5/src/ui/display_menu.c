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

#include "flashlight_screen.h"
#include "flashlight_matrix.h"
#include "flashlight_texture.h"
#include "flashlight_shader.h"

#include "textpane.h"

#include "display_menu.h"
#include "tsf_joystick.h"
#include "ui_sound.h"
#include "user_events.h"
#include "gridmenu.h"
#include "path.h"
#include "settings.h"
#include "locale.h"
#include "display_menu.h"

#define WIDTH 1280
#define HEIGHT 720

#define FY 91
#define HW (WIDTH/2)
#define HH (HEIGHT/2)

#define MW  WIDTH
#define MH  HEIGHT
#define MHH (MH/2)
#define MHW (MW/2)

static int atlas_width;
static int atlas_height;

typedef void (*menuSelectCallback)     ( menu_t *m, void *private, int id );

static void _select_mode(menu_t *m, void *private, int id );
static void change_mode( DisplayMenu *dmenu, int id ) ;

#define SP ((WIDTH - (320 * 3)) / 4)

static fs_item_t menu_item[] = {
    {   0,120, 320, 200, 160, 100, MARKER_LEFT, "DM_PX_SHARP", _select_mode },
    {   0,120, 290, 200, 160, 100, MARKER_LEFT, "DM_EU_SHARP", _select_mode },
    {   0,120, 240, 200, 160, 100, MARKER_LEFT, "DM_US_SHARP", _select_mode },
    { 320,120, 320, 200, 160, 100, MARKER_LEFT, "DM_PX_CRT",   _select_mode },
    { 320,120, 290, 200, 160, 100, MARKER_LEFT, "DM_EU_CRT",   _select_mode },
    { 320,120, 240, 200, 160, 100, MARKER_LEFT, "DM_US_CRT",   _select_mode },
};

static display_mode_t id_to_dmode[] = {
    PIXEL_SHARP,
    EU_SHARP,
    US_SHARP,
    PIXEL_CRT,
    EU_CRT,
    US_CRT,
};

// ----------------------------------------------------------------------------------
//
static void
add_picture( FLVerticeGroup *vg, int id, int rhs_x, int center_y )
{
    fs_item_t *ic = &(menu_item[id]);

    int x0 = rhs_x    - ic->w / 2;
    int y0 = center_y - ic->oy;
    int x1 = x0       + ic->w;
    int y1 = center_y - ic->oy + ic->h;

    int s0 = ic->s;
    int t0 = ic->t;
    int s1 = ic->s + 320; //ic->w;
    int t1 = ic->t + 200; //ic->h;
    int c[] = {x0,y0,s0,t1, x0,y1,s0,t0, x1,y0,s1,t1, x1,y1,s1,t0};

    FLVertice vi[4];
    int j;

    if( ic->w != 320 ) {
        // First place a black rectangle in place
        int bx0 = rhs_x - 320 / 2;
        int bx1 = bx0   + 320;
        int bs0 = 4.1;
        int bt0 =  60.1;
        int bs1 = 4.9;
        int bt1 =  60.9;
        int bc[] = {bx0,y0,bs0,bt1, bx0,y1,bs0,bt0, bx1,y0,bs1,bt1, bx1,y1,bs1,bt0};
        for(j = 0; j < 4; j++ ) {
            vi[j].x = X_TO_GL(WIDTH,        bc[ j * 4 + 0 ]);
            vi[j].y = Y_TO_GL(HEIGHT,       bc[ j * 4 + 1 ]);
            vi[j].s = S_TO_GL(atlas_width,  bc[ j * 4 + 2 ]);
            vi[j].t = T_TO_GL(atlas_height, bc[ j * 4 + 3 ]);
            vi[j].u = 0.2;
        }
        flashlight_vertice_add_quad2D( vg, &vi[0], &vi[1], &vi[2], &vi[3] );
    }

    for(j = 0; j < 4; j++ ) {
        vi[j].x = X_TO_GL(WIDTH,        c[ j * 4 + 0 ]);
        vi[j].y = Y_TO_GL(HEIGHT,       c[ j * 4 + 1 ]);
        vi[j].s = S_TO_GL(atlas_width,  c[ j * 4 + 2 ]);
        vi[j].t = T_TO_GL(atlas_height, c[ j * 4 + 3 ]);
        vi[j].u = 0.2;
    }
    flashlight_vertice_add_quad2D( vg, &vi[0], &vi[1], &vi[2], &vi[3] );
}

// ----------------------------------------------------------------------------------
//
static void
init_menu_cb( menu_t *m, void *private, int id, int *enabled, menu_item_dim_t *dim, menuSelectCallback *ms_cb  )
{
    FullScreenMenu *fs_menu = (FullScreenMenu *)private;
    DisplayMenu     *dmenu   = (DisplayMenu *)(fs_menu->owner_private);

    *enabled = 1;

    dim->w = 320;
    dim->h = 200;

    int x = SP + (320 + SP) * (id % 3) + (dim->w / 2);

    //int x = HW + ((id % 2) ? +200 : -200);
    int y = HH + ((id < 3) ? +130 : -130) - 0;

    //add_picture( fs_menu->back_vertice_group, id, x + dim->w / 2, y );
    add_picture( fs_menu->back_vertice_group, id, x, y );

    if( dmenu->text[ id ] == NULL ) {
        textpane_t *tp = textpane_init( fs_menu->back_vertice_group, fs_menu->fonts->fontatlas, fs_menu->fonts->body, dim->w, 30, WIDTH, HEIGHT, TEXT_CENTER );
        dmenu->text[ id ] = tp;
    }

    unsigned char *txt = locale_get_text( menu_item[ id ].text );
    build_text( dmenu->text[ id ], txt, 1, x - (dim->w / 2), y - (dim->h / 2) - 20 );

    dim->x = x;
    dim->y = y - 13;

    dim->selected_w = dim->w;
    dim->selected_h = dim->h;

    dim->selector_w = dim->w + 2;
    dim->selector_h = dim->h + 26 + 2;

    dim->marker_pos = menu_item[id].marker_pos;

    *ms_cb = menu_item[ id ].cb; // The select callback
}

// ----------------------------------------------------------------------------------
// Initialise in-game shot display
DisplayMenu *
display_menu_init( Flashlight *f, fonts_t *fonts )
{
    DisplayMenu *dmenu = malloc( sizeof(DisplayMenu) );
    if( dmenu == NULL ) {
        return NULL;
    }
    memset( dmenu, 0, sizeof(DisplayMenu) );

    // Make sure atlas is loaded - we just need to get width and height here
    FLTexture *tx = flashlight_load_texture( f, get_path("/usr/share/the64/ui/textures/menu_atlas.png") );
    atlas_width = tx->width;
    atlas_height = tx->height;

    dmenu->fonts = fonts;
    dmenu->fs_menu = fsmenu_menu_init( f, fonts, init_menu_cb, (void *)dmenu, "DM_TITLE", 3, 2, HEIGHT, HEIGHT / 2, FS_MENU_FLAG_POINTER | FS_MENU_FLAG_SETTINGS_MENU, OPAQUE );
    dmenu->fs_menu->name = "Display";

    // Default if settings have not been saved
    display_menu_set_selection( dmenu, PIXEL_SHARP );
    settings_set_display_mode( PIXEL_SHARP );

    return dmenu;
}

// ----------------------------------------------------------------------------------
//
void
display_menu_set_selection( DisplayMenu *dmenu, display_mode_t mode )
{
    int i = 0;
    int len = sizeof(id_to_dmode) / sizeof(display_mode_t);

    for( i = 0; i < len; i++ ) {
        if( id_to_dmode[i] == mode ) {
            menu_set_current_item( dmenu->fs_menu->menu, i );
            menu_set_marked_item ( dmenu->fs_menu->menu, i );
            break;
        }
    }
}

// ----------------------------------------------------------------------------------

void
display_menu_finish( DisplayMenu **m ) {
    free(*m);
    *m = NULL;
}

// ----------------------------------------------------------------------------------
//
void
display_menu_enable( DisplayMenu *dmenu )
{
    fsmenu_menu_enable( dmenu->fs_menu, 1 ); // 1 = hires selector
}

// ----------------------------------------------------------------------------------
void
display_menu_disable( DisplayMenu *dmenu )
{
    fsmenu_menu_disable( dmenu->fs_menu );
}

// ----------------------------------------------------------------------------------
//
static void
change_mode( DisplayMenu *dmenu, int id ) 
{
    display_mode_t dmode = id_to_dmode[id];

    TSFEventSetting e = { {Setting, (void*)dmenu}, DisplayMode, (void *)dmode };
    flashlight_event_publish( dmenu->fs_menu->f, (FLEvent*)&e );

    settings_set_display_mode( dmode );
}

// ----------------------------------------------------------------------------------
//
static void
_select_mode(menu_t *m, void *private, int id )
{
    FullScreenMenu *fs_menu = (FullScreenMenu *)private;
    DisplayMenu    *dmenu   = (DisplayMenu *)(fs_menu->owner_private);

    menu_set_marked_item( fs_menu->menu, id );

    change_mode( dmenu, id );

    ui_sound_play_ding();
}

// ----------------------------------------------------------------------------------
// end

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
#define _XOPEN_SOURCE 500 // Mandatory for nftw()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ftw.h>
#include <unistd.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "flashlight.h"

#include "textpane.h"

#include "display_menu.h"
#include "tsf_joystick.h"
#include "ui_sound.h"
#include "user_events.h"
#include "gridmenu.h"
#include "path.h"
#include "settings.h"
#include "locale.h"
#include "test_mode.h"
#include "firmware.h"
#include "poweroff.h"

#define WIDTH 1280
#define HEIGHT 720

#define FY 91
#define HW (WIDTH/2)
#define HH (HEIGHT/2)

#define MW  WIDTH
#define MH  HEIGHT
#define MHH (MH/2)
#define MHW (MW/2)

typedef void (*menuSelectCallback)     ( menu_t *m, void *private, int id );

static int delete_profile( TestModeMenu *frmenu );

static void _select_confirm(menu_t *m, void *private, int id );
static void _select_ping(menu_t *m, void *private, int id );

static fs_item_t menu_item[] = {
    {   0, 0, 100, 18, 400, 340, MARKER_NONE, "TM_PING",    _select_ping   },
    {   0, 0, 100, 18, 780, 340, MARKER_NONE, "TM_CONFIRM", _select_confirm },
};

// ----------------------------------------------------------------------------------
//
static void
init_menu_cb( menu_t *m, void *private, int id, int *enabled, menu_item_dim_t *dim, menuSelectCallback *ms_cb  )
{
    FullScreenMenu *fs_menu = (FullScreenMenu *)private;
    TestModeMenu     *frmenu   = (TestModeMenu *)(fs_menu->owner_private);

    if( id >= TM_MAX_TEXTS ) return;

    // If we're building the first menu item, then also build any other screen text/assets needed
    // because the language may have changed
    if( id == 0 ) {
        if( frmenu->statement == NULL ) {

            frmenu->statement = textpane_init( fs_menu->back_vertice_group, fs_menu->fonts->fontatlas, fs_menu->fonts->body, 730, 60, WIDTH, HEIGHT, TEXT_CENTER );
        }

        unsigned char *statement = locale_get_text( "TM_STATEMENT" );
        build_text( frmenu->statement, statement, 1, (WIDTH - 730) / 2, 420 );
    }

    *enabled = 1;

    int x = menu_item[id].ox;
    int y = menu_item[id].oy;
    int w = menu_item[id].w;
    int h = menu_item[id].h;
    
    if( frmenu->text[ id ] == NULL ) {
        textpane_t *tp = textpane_init( fs_menu->back_vertice_group, fs_menu->fonts->fontatlas, fs_menu->fonts->body, w, h, WIDTH, HEIGHT, TEXT_CENTER );

        frmenu->text[ id ] = tp;
    }

    unsigned char *label = locale_get_text( menu_item[ id ].text );

    textpane_t *tp = frmenu->text[ id ];
    tp->largest_width = 0;
    build_text( tp, label, 1, x , y - 20 );

    dim->w = tp->largest_width + 1;
    dim->h = h;

    dim->x = x + (w / 2 ); // Text is centred on textpane, so use tp max width, not text width
    dim->y = y - dim->h / 2 - 4;

    dim->selected_w = dim->w;
    dim->selected_h = dim->h;

    dim->selector_w = dim->w + 2;
    dim->selector_h = dim->h + 2;

    dim->marker_pos = menu_item[id].marker_pos;

    *ms_cb = menu_item[ id ].cb; // The select callback
}

// ----------------------------------------------------------------------------------
// Initialise in-game shot display
TestModeMenu *
test_mode_menu_init( Flashlight *f, fonts_t *fonts )
{
    TestModeMenu *frmenu = malloc( sizeof(TestModeMenu) );
    if( frmenu == NULL ) {
        return NULL;
    }
    memset( frmenu, 0, sizeof(TestModeMenu) );

    frmenu->f = f;
    frmenu->fonts  = fonts;

    frmenu->fs_menu = fsmenu_menu_init( f, fonts, init_menu_cb, (void *)frmenu, "TM_TITLE", 2, 1, 400, 400/2, FS_MENU_FULLSCREEN | FS_MENU_FLAG_NO_ICONS | FS_MENU_FLAG_IGNORE_INPUT | FS_MENU_FLAG_WARNING, OPAQUE );
    frmenu->fs_menu->name = "TestMode";

    return frmenu;
}

// ----------------------------------------------------------------------------------

void
test_mode_menu_finish( TestModeMenu **m ) {
    free(*m);
    *m = NULL;
}

// ----------------------------------------------------------------------------------
//
static void
_select_ping(menu_t *m, void *private, int id )
{
//    FullScreenMenu   *fs_menu = (FullScreenMenu *)private;
//    TestModeMenu *frmenu   = (TestModeMenu *)(fs_menu->owner_private);
}

// ----------------------------------------------------------------------------------
//
static void
_select_confirm(menu_t *m, void *private, int id )
{
    FullScreenMenu   *fs_menu = (FullScreenMenu *)private;
    TestModeMenu *frmenu   = (TestModeMenu *)(fs_menu->owner_private);

    ui_sound_play_ding();
    ui_sound_wait_for_ding();

    // Delete the user profile - which will clear the test flag
    delete_profile( frmenu );

    // And then signal a reboot
    TSFEventQuit e = { {Quit, NULL}, QUIT_SHUTDOWN };
    flashlight_event_publish( frmenu->f,(FLEvent*)&e );
}

// ----------------------------------------------------------------------------------
void
test_mode_menu_enable( TestModeMenu *frmenu )
{
    menu_reset_current( frmenu->fs_menu->menu ); 
    fsmenu_menu_enable( frmenu->fs_menu, 1 ); // 1 = hires selector
}

// ----------------------------------------------------------------------------------
void
test_mode_menu_disable( TestModeMenu *frmenu )
{
    //fsmenu_menu_disable( frmenu->fs_menu );
}

// ----------------------------------------------------------------------------------
//
static int
_nftw_cb( const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int rv = 0;
    // In development, we don't delete the directory tree - just in case it goes wrong!
#warning FACTORY RESET WILL DO DELETE
    // The target platform DOES remove the tree
    rv = remove(fpath);
    if (rv) perror(fpath);

    return rv;
}

// ----------------------------------------------------------------------------------
//
static int
delete_profile( TestModeMenu *frmenu )
{
    const char * path = get_profile_path( "/0" );

    if( !mount_root_rw() ) return -1;

    int r = nftw(path, _nftw_cb, 64, FTW_DEPTH | FTW_PHYS);
    sync();

    mount_root_ro();

    return r;
}

// ----------------------------------------------------------------------------------

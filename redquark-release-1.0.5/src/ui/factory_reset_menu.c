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
#include "factory_reset_menu.h"
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

static int delete_profile( FactoryResetMenu *frmenu );
static int delete_upgrade( FactoryResetMenu *frmenu );

static void _select_cancel(menu_t *m, void *private, int id );
static void _select_confirm(menu_t *m, void *private, int id );

static fs_item_t menu_item[] = {
    {   0, 0, 100, 18, 400, 340, MARKER_NONE, "FR_CANCEL",  _select_cancel },
    {   0, 0, 100, 18, 780, 340, MARKER_NONE, "FR_CONFIRM", _select_confirm },
};

// ----------------------------------------------------------------------------------
//
static void
init_menu_cb( menu_t *m, void *private, int id, int *enabled, menu_item_dim_t *dim, menuSelectCallback *ms_cb  )
{
    FullScreenMenu *fs_menu = (FullScreenMenu *)private;
    FactoryResetMenu     *frmenu   = (FactoryResetMenu *)(fs_menu->owner_private);

    if( id >= FR_MAX_TEXTS ) return;

    // If we're building the first menu item, then also build any other screen text/assets needed
    // because the language may have changed
    if( id == 0 ) {
        if( frmenu->statement == NULL ) {

            frmenu->statement = textpane_init( fs_menu->back_vertice_group, fs_menu->fonts->fontatlas, fs_menu->fonts->body, 1100, 18, WIDTH, HEIGHT, TEXT_CENTER );
        }

        unsigned char *statement = locale_get_text( "FR_STATEMENT" );
        build_text( frmenu->statement, statement, 1, (WIDTH - 1100) / 2, 400 );
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
FactoryResetMenu *
factory_reset_menu_init( Flashlight *f, fonts_t *fonts )
{
    FactoryResetMenu *frmenu = malloc( sizeof(FactoryResetMenu) );
    if( frmenu == NULL ) {
        return NULL;
    }
    memset( frmenu, 0, sizeof(FactoryResetMenu) );

    frmenu->f = f;
    frmenu->fonts  = fonts;

#define FRH (300)
    frmenu->fs_menu = fsmenu_menu_init( f, fonts, init_menu_cb, (void *)frmenu, "FR_TITLE", 2, 1, FRH, (FRH/2), FS_MENU_FLAG_NO_CLAIM_SCREEN | FS_MENU_FLAG_WARNING | FS_MENU_FLAG_NO_LOGO | FS_MENU_FLAG_NO_ICONS | FS_MENU_FLAG_STACKED, OPAQUE );
    frmenu->fs_menu->name = "FactoryReset";

    return frmenu;
}

// ----------------------------------------------------------------------------------

void
factory_reset_menu_finish( FactoryResetMenu **m ) {
    free(*m);
    *m = NULL;
}

// ----------------------------------------------------------------------------------
//
static void
_select_cancel(menu_t *m, void *private, int id )
{
    FullScreenMenu *fs_menu = (FullScreenMenu *)private;
    FactoryResetMenu     *frmenu   = (FactoryResetMenu *)(fs_menu->owner_private);

    fsmenu_menu_disable( frmenu->fs_menu );
}

// ----------------------------------------------------------------------------------
//
static void
_select_confirm(menu_t *m, void *private, int id )
{
    FullScreenMenu   *fs_menu = (FullScreenMenu *)private;
    FactoryResetMenu *frmenu   = (FactoryResetMenu *)(fs_menu->owner_private);

    ui_sound_play_ding();
    ui_sound_wait_for_ding();

    // Delete the user profile and upgrade install directory
    delete_profile( frmenu );
    delete_upgrade( frmenu );

    // And then signal a reboot
    TSFEventQuit e = { {Quit, NULL}, QUIT_REBOOT };
    flashlight_event_publish( frmenu->f,(FLEvent*)&e );
}

// ----------------------------------------------------------------------------------
void
factory_reset_menu_enable( FactoryResetMenu *frmenu )
{
    menu_reset_current( frmenu->fs_menu->menu ); 
    fsmenu_menu_enable( frmenu->fs_menu, 1 ); // 1 = hires selector
}

// ----------------------------------------------------------------------------------
void
factory_reset_menu_disable( FactoryResetMenu *frmenu )
{
    fsmenu_menu_disable( frmenu->fs_menu );
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
delete_profile( FactoryResetMenu *frmenu )
{
    const char * path = get_profile_path( "/0" );

    if( !mount_root_rw() ) return -1;

    int r = nftw(path, _nftw_cb, 64, FTW_DEPTH | FTW_PHYS);
    sync();

    mount_root_ro();

    return r;
}

// ----------------------------------------------------------------------------------
//
static int
delete_upgrade( FactoryResetMenu *frmenu )
{
    const char * path = get_upgrade_path( "/" );
    // Paranoid measure, to make sure a broken get_upgrade_path does not return '/'...
    //
    if( strlen(path) < 22 ) return 0;

    if( !mount_root_rw() ) return -1;

    int r = nftw(path, _nftw_cb, 64, FTW_DEPTH | FTW_PHYS);
    sync();

    mount_root_ro();

    return r;
}

// ----------------------------------------------------------------------------------
// end

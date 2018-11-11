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

#include "display_menu.h"
#include "tsf_joystick.h"
#include "ui_sound.h"
#include "user_events.h"
#include "gridmenu.h"
#include "path.h"
#include "settings.h"
#include "locale.h"
#include "keyboard_menu.h"

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
static void _select_keyboard(menu_t *m, void *private, int id );

static fs_item_t menu_item[] = {
    {   0, 0, 160, 18, 400, 500, MARKER_LEFT, "KM_UK", _select_keyboard },
    {   0, 0, 160, 18, 740, 500, MARKER_LEFT, "KM_US", _select_keyboard },
    {   0, 0, 160, 18, 400, 400, MARKER_LEFT, "KM_DE", _select_keyboard },
    {   0, 0, 160, 18, 740, 400, MARKER_LEFT, "KM_ES", _select_keyboard },
    {   0, 0, 160, 18, 400, 300, MARKER_LEFT, "KM_FR", _select_keyboard },
    {   0, 0, 160, 18, 740, 300, MARKER_LEFT, "KM_IT", _select_keyboard },
};

static language_t id_to_keyboard[] = {
    LANG_EN_UK,
    LANG_EN_US,
    LANG_DE,
    LANG_ES,
    LANG_FR,
    LANG_IT
};

// ----------------------------------------------------------------------------------
//
static void
init_menu_cb( menu_t *m, void *private, int id, int *enabled, menu_item_dim_t *dim, menuSelectCallback *ms_cb  )
{
    FullScreenMenu *fs_menu = (FullScreenMenu *)private;
    KeyboardMenu     *kmenu   = (KeyboardMenu *)(fs_menu->owner_private);

    if( id >= KM_MAX_TEXTS ) return;

    *enabled = 1;

    int x = menu_item[id].ox;
    int y = menu_item[id].oy;
    int w = menu_item[id].w;
    int h = menu_item[id].h;
    

    if( kmenu->text[ id ] == NULL ) {
        textpane_t *tp = textpane_init( fs_menu->back_vertice_group, fs_menu->fonts->fontatlas, fs_menu->fonts->body, w, h, WIDTH, HEIGHT, TEXT_LEFT );

        kmenu->text[ id ] = tp;
    }

    unsigned char *label = locale_get_text( menu_item[ id ].text );
    textpane_t *tp =  kmenu->text[ id ];
    tp->largest_width = 0;
    build_text( tp, label, 1, x , y - 20 );

    dim->w = tp->largest_width + 1;
    dim->h = h;

    dim->x = x + dim->w / 2;
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
KeyboardMenu *
keyboard_menu_init( Flashlight *f, fonts_t *fonts )
{
    KeyboardMenu *kmenu = malloc( sizeof(KeyboardMenu) );
    if( kmenu == NULL ) {
        return NULL;
    }
    memset( kmenu, 0, sizeof(KeyboardMenu) );

    kmenu->fonts = fonts;
    kmenu->fs_menu = fsmenu_menu_init( f, fonts, init_menu_cb, (void *)kmenu, "KM_TITLE", 2, 3, HEIGHT, HEIGHT/2, FS_MENU_FLAG_POINTER | FS_MENU_FLAG_NO_CLAIM_SCREEN | FS_MENU_FLAG_MATCH_ITEM_WIDTH | FS_MENU_FLAG_STACKED | FS_MENU_FLAG_SETTINGS_MENU | FS_MENU_FULLSCREEN, OPAQUE );
    kmenu->fs_menu->name = "Keyboard";

    // Default if settings have not been saved
    keyboard_menu_set_selection( kmenu, LANG_EN );
    settings_set_keyboard( LANG_EN );

    return kmenu;
}

// ----------------------------------------------------------------------------------

void
keyboard_menu_finish( KeyboardMenu **m ) {
    free(*m);
    *m = NULL;
}

// ----------------------------------------------------------------------------------
//
void
keyboard_menu_set_selection( KeyboardMenu *kmenu, language_t keyboard )
{
    int i;
    int len = sizeof(id_to_keyboard) / sizeof(language_t);

    for( i = 0; i < len; i++ ) {
        if( id_to_keyboard[i] == keyboard ) {
            menu_set_current_item( kmenu->fs_menu->menu, i );
            menu_set_marked_item ( kmenu->fs_menu->menu, i );
            break;
        }
    }
}

// ----------------------------------------------------------------------------------
static void
_select_keyboard(menu_t *m, void *private, int id )
{
    FullScreenMenu *fs_menu = (FullScreenMenu *)private;
    KeyboardMenu     *kmenu   = (KeyboardMenu *)(fs_menu->owner_private);

    menu_set_marked_item( fs_menu->menu, id );

    ui_sound_play_ding();

    language_t keyboard = id_to_keyboard[id];

    TSFEventSetting e = { {Setting, (void*)kmenu}, KeyboardRegion, (void *)keyboard };
    flashlight_event_publish( fs_menu->f, (FLEvent*)&e );

    settings_set_keyboard( keyboard );
}

// ----------------------------------------------------------------------------------
int
keyboard_menu_enable( KeyboardMenu *kmenu )
{
    return fsmenu_menu_enable( kmenu->fs_menu, 1 ); // 1 = hires selector
}

// ----------------------------------------------------------------------------------
int
keyboard_menu_disable( KeyboardMenu *kmenu )
{
    return fsmenu_menu_disable( kmenu->fs_menu );
}

// ----------------------------------------------------------------------------------
// end

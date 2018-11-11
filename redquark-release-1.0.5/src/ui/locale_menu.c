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
#include "locale_menu.h"

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
static void _select_language(menu_t *m, void *private, int id );

static fs_item_t menu_item[] = {
    {   0, 0, 160, 18, 400, 500, MARKER_LEFT, "English",  _select_language }, 
    {   0, 0, 160, 18, 740, 500, MARKER_LEFT, "Deutsch",  _select_language }, 
    {   0, 0, 160, 18, 400, 400, MARKER_LEFT, "Español",  _select_language }, 
    {   0, 0, 160, 18, 740, 400, MARKER_LEFT, "Français", _select_language }, 
    {   0, 0, 160, 18, 400, 300, MARKER_LEFT, "Italiano", _select_language }, 
    {   0, 0, 160, 18, 740, 300, MARKER_LEFT,  "" },
};

static language_t id_to_language[] = {
    LANG_EN,
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
    LocaleMenu     *lmenu   = (LocaleMenu *)(fs_menu->owner_private);

    if( id >= LM_MAX_TEXTS ) return;

    if( *(menu_item[ id ].text) == '\0' ) {
        *enabled = 0;
        return;
    }

    *enabled = 1;

    int x = menu_item[id].ox;
    int y = menu_item[id].oy;
    int w = menu_item[id].w;
    int h = menu_item[id].h;
    
    dim->w = w;
    dim->h = h;

    if( lmenu->text[ id ] == NULL ) {
        lmenu->text[ id ] = textpane_init( fs_menu->back_vertice_group, fs_menu->fonts->fontatlas, fs_menu->fonts->body, w, h, WIDTH, HEIGHT, TEXT_LEFT );
    }

    textpane_t *tp = lmenu->text[ id ];
    tp->largest_width = 0;
    build_text( tp, (unsigned char *)menu_item[ id ].text, 1, x , y - 20 );

    dim->w = tp->largest_width + 1;

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
LocaleMenu *
locale_menu_init( Flashlight *f, fonts_t *fonts )
{
    LocaleMenu *lmenu = malloc( sizeof(LocaleMenu) );
    if( lmenu == NULL ) {
        return NULL;
    }
    memset( lmenu, 0, sizeof(LocaleMenu) );

    lmenu->fonts = fonts;
    lmenu->fs_menu = fsmenu_menu_init( f, fonts, init_menu_cb, (void *)lmenu, "LM_TITLE", 2, 3, HEIGHT, HEIGHT/2, FS_MENU_FLAG_POINTER | FS_MENU_FLAG_MATCH_ITEM_WIDTH | FS_MENU_FLAG_SETTINGS_MENU, OPAQUE );
    lmenu->fs_menu->name = "Locale";

    // Default if settings have not been saved
    locale_menu_set_selection( lmenu, LANG_EN );
    settings_set_language( LANG_EN );

    return lmenu;
}

// ----------------------------------------------------------------------------------

void
locale_menu_finish( LocaleMenu **m ) {
    free(*m);
    *m = NULL;
}

// ----------------------------------------------------------------------------------
//
void
locale_menu_set_selection( LocaleMenu *lmenu, language_t lang )
{
    int i;
    int len = sizeof(id_to_language) / sizeof(language_t);

    for( i = 0; i < len; i++ ) {
        if( id_to_language[i] == lang ) {
            menu_set_current_item( lmenu->fs_menu->menu, i );
            menu_set_marked_item ( lmenu->fs_menu->menu, i );
            break;
        }
    }
}

// ----------------------------------------------------------------------------------
static void
_select_language(menu_t *m, void *private, int id )
{
    FullScreenMenu *fs_menu = (FullScreenMenu *)private;
    LocaleMenu     *lmenu   = (LocaleMenu *)(fs_menu->owner_private);

    menu_set_marked_item( fs_menu->menu, id );

    ui_sound_play_ding();

    language_t lang = id_to_language[id];

    TSFEventSetting e = { {Setting, (void*)lmenu}, Language, (void *)lang };
    flashlight_event_publish( fs_menu->f, (FLEvent*)&e );

    settings_set_language( lang );
}

// ----------------------------------------------------------------------------------
void
locale_menu_enable( LocaleMenu *lmenu )
{
    fsmenu_menu_enable( lmenu->fs_menu, 1 ); // 1 = hires selector
}

// ----------------------------------------------------------------------------------
void
locale_menu_disable( LocaleMenu *lmenu )
{
    fsmenu_menu_disable( lmenu->fs_menu );
}

// ----------------------------------------------------------------------------------
// end

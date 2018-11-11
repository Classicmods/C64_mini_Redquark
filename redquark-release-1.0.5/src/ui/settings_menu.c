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
#include "locale.h"
#include "path.h"
#include "settings.h"
#include "settings_menu.h"

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
static void _select_ip_notice(menu_t *m, void *private, int id );
static void _select_keyboard (menu_t *m, void *private, int id );
static void _select_factory_reset (menu_t *m, void *private, int id );
static void _select_sysinfo(menu_t *m, void *private, int id );

// s t  w h  ox oy
static fs_item_t menu_item[] = {
    {   0, 0, 400, 18, 540, 435, MARKER_NONE, "SM_KEYBOARD",    _select_keyboard },
    {   0, 0, 400, 18, 540, 385, MARKER_NONE, "SM_IPNOTICE",    _select_ip_notice },
    {   0, 0, 400, 18, 540, 335, MARKER_NONE, "SM_SYSTEM",      _select_sysinfo },
    {   0, 0, 400, 18, 540, 250, MARKER_NONE, "SM_FACTORY_RST", _select_factory_reset },
};

static void settings_menu_event( void *self, FLEvent *event );

// ----------------------------------------------------------------------------------
//
static void
init_menu_cb( menu_t *m, void *private, int id, int *enabled, menu_item_dim_t *dim, menuSelectCallback *ms_cb  )
{
    FullScreenMenu *fs_menu = (FullScreenMenu *)private;
    SettingsMenu     *smenu   = (SettingsMenu *)(fs_menu->owner_private);

    if( id >= SM_MAX_TEXTS ) return;

    *enabled = 1;

    int x = menu_item[id].ox;
    int y = menu_item[id].oy;
    int w = menu_item[id].w;
    int h = menu_item[id].h;

    // Adjust "center" based on language... Not a great solution, but need a quick one!
    switch( locale_get_language() ) {
        case LANG_EN: x -=  0; break;
        case LANG_DE: x -= 90; break;
        case LANG_FR: x -= 20; break;
        case LANG_ES: x -= 30; break;
        case LANG_IT: x -= 50; break;
        default:      x -=  0; break;
    }
    
    if( smenu->text[ id ] == NULL ) {
        smenu->text[ id ] = textpane_init( fs_menu->back_vertice_group, fs_menu->fonts->fontatlas, fs_menu->fonts->body, w, h, WIDTH, HEIGHT, TEXT_LEFT );
    }

    textpane_t *tp = smenu->text[ id ];
    tp->largest_width = 0;
    unsigned char *label = locale_get_text( menu_item[ id ].text );
    build_text( tp, label, 1, x , y - 20 );

    dim->w  = tp->largest_width + 1;
    dim->h = h;
    dim->x = x + dim->w / 2;
    dim->y = y - dim->h / 2 - 4;
    dim->selected_w = dim->w;
    dim->selected_h = dim->h;
    dim->selector_h = dim->h + 2;
    dim->selector_w = dim->w + 2;

    dim->marker_pos = menu_item[id].marker_pos;

    *ms_cb = menu_item[ id ].cb; // The select callback
}

// ----------------------------------------------------------------------------------
// Initialise in-game shot display
SettingsMenu *
settings_menu_init( Flashlight *f, fonts_t *fonts )
{
    SettingsMenu *smenu = malloc( sizeof(SettingsMenu) );
    if( smenu == NULL ) {
        return NULL;
    }
    memset( smenu, 0, sizeof(SettingsMenu) );

    smenu->fonts = fonts;
    smenu->fs_menu = fsmenu_menu_init( f, fonts, init_menu_cb, (void *)smenu, "SM_TITLE", 1, 4, HEIGHT, HEIGHT/2, FS_MENU_FLAG_NONE | FS_MENU_FLAG_MATCH_ITEM_WIDTH, OPAQUE );
    smenu->fs_menu->name = "Settings";

    flashlight_event_subscribe( f, (void*)smenu, settings_menu_event, FLEventMaskAll );

    smenu->ipnotice = ip_notice_init( f, fonts );
    smenu->kmenu    = keyboard_menu_init( f, fonts );
    smenu->frmenu   = factory_reset_menu_init( f, fonts );
    smenu->simenu   = system_menu_init( f, fonts );

    return smenu;
}

// ----------------------------------------------------------------------------------

void
settings_menu_finish( SettingsMenu **m ) {
    free(*m);
    *m = NULL;
}

// ----------------------------------------------------------------------------------

static void
settings_menu_event( void *self, FLEvent *event )
{
    SettingsMenu *smenu = (SettingsMenu *)self;

    //if( !fsmenu->active && !fsmenu->menu->moving ) return;

    switch( event->type ) {
//        case UserEvent:
//            {
//                TSFUserEvent *ue = (TSFUserEvent*)event;
//                switch(ue->user_type) {
//                    case MenuClosed:
//                        if( ue->data == smenu->ipnotice->fs_menu || 
//                            ue->data == smenu->frmenu->fs_menu   ||
//                            ue->data == smenu->simenu->fs_menu   ||
//                            ue->data == smenu->kmenu->fs_menu ) {
//                           
//printf("Menu closed: %s\n", ((FullScreenMenu *)ue->data)->name );
//printf("Restore inputs to: %s\n", smenu->fs_menu->name );
//                            smenu->fs_menu->flags &= ~FS_MENU_FLAG_IGNORE_INPUT;
//                        }
//                        break;
//                    default:
//                        break;
//                }
//            }

        case Setting:
            {
                TSFEventSetting *es = (TSFEventSetting*)event;
                switch( es->id ) {
                    case KeyboardRegion: 
                        keyboard_menu_set_selection( smenu->kmenu, FL_EVENT_DATA_TO_TYPE(language_t, es->data) ); break;

                    default: break;
                }
            }
            break;
        default: break;
    }
}

// ----------------------------------------------------------------------------------
//
static void
_select_keyboard(menu_t *m, void *private, int id )
{
    FullScreenMenu *fs_menu = (FullScreenMenu *)private;
    SettingsMenu     *smenu   = (SettingsMenu *)(fs_menu->owner_private);
    
    // This flag gets cleared when submenu has closed. Don't open another submenu
    // if this flag is still set
    //if( smenu->fs_menu->flags &= FS_MENU_FLAG_IGNORE_INPUT ) return;

    keyboard_menu_enable( smenu->kmenu );
    //smenu->fs_menu->flags |= FS_MENU_FLAG_IGNORE_INPUT;
}

// ----------------------------------------------------------------------------------
static void
_select_ip_notice(menu_t *m, void *private, int id )
{
    FullScreenMenu *fs_menu = (FullScreenMenu *)private;
    SettingsMenu     *smenu   = (SettingsMenu *)(fs_menu->owner_private);

    // This flag gets cleared when submenu has closed. Don't open another submenu
    // if this flag is still set
    //if( smenu->fs_menu->flags &= FS_MENU_FLAG_IGNORE_INPUT ) return;

    ip_notice_enable( smenu->ipnotice );
    //smenu->fs_menu->flags |= FS_MENU_FLAG_IGNORE_INPUT;
}

// ----------------------------------------------------------------------------------
static void
_select_factory_reset(menu_t *m, void *private, int id )
{
    FullScreenMenu *fs_menu = (FullScreenMenu *)private;
    SettingsMenu   *smenu   = (SettingsMenu *)(fs_menu->owner_private);
    
    // This flag gets cleared when submenu has closed. Don't open another submenu
    // if this flag is still set
    //if( smenu->fs_menu->flags &= FS_MENU_FLAG_IGNORE_INPUT ) return;

    factory_reset_menu_enable( smenu->frmenu );
    //smenu->fs_menu->flags |= FS_MENU_FLAG_IGNORE_INPUT;
}

// ----------------------------------------------------------------------------------
//
static void
_select_sysinfo(menu_t *m, void *private, int id )
{
    FullScreenMenu *fs_menu = (FullScreenMenu *)private;
    SettingsMenu     *smenu   = (SettingsMenu *)(fs_menu->owner_private);
 
    // This flag gets cleared when submenu has closed. Don't open another submenu
    // if this flag is still set
    //if( smenu->fs_menu->flags &= FS_MENU_FLAG_IGNORE_INPUT ) return;

    system_menu_enable( smenu->simenu );
    //smenu->fs_menu->flags |= FS_MENU_FLAG_IGNORE_INPUT;
}

// ----------------------------------------------------------------------------------
void
settings_menu_enable( SettingsMenu *smenu )
{
    menu_reset_current( smenu->fs_menu->menu ); 
    fsmenu_menu_enable( smenu->fs_menu, 1 ); // 1 = hires selector
}

// ----------------------------------------------------------------------------------
void
settings_menu_disable( SettingsMenu *smenu )
{
    fsmenu_menu_disable( smenu->fs_menu );
}

// ----------------------------------------------------------------------------------
// end

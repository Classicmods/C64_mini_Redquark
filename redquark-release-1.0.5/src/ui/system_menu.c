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
#include "system_menu.h"

#include "release.h"
#include "firmware.h"

#define TEXT_COLOUR  TP_COLOUR_DARK

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

static void _select_cancel(menu_t *m, void *private, int id );
static void _select_confirm(menu_t *m, void *private, int id );

static fs_item_t menu_item[] = {
    {   0, 0, 100, 18, 400,      240, MARKER_NONE, "SI_CANCEL",    _select_cancel },
    {   0, 0, 100, 18, 780,      240, MARKER_NONE, "SI_UPGRADE",   _select_confirm },
};

#define NOTFOUND (unsigned char*)"Not found"

// ----------------------------------------------------------------------------------
//
static void
add_screen_text( SystemMenu *smenu, FullScreenMenu *fs_menu )
{
    if( smenu->statement == NULL ) {
        FLVerticeGroup *vg = fs_menu->back_vertice_group;
        
        smenu->statement = textpane_init( vg, fs_menu->fonts->fontatlas, fs_menu->fonts->body, 1100, 18, WIDTH, HEIGHT, TEXT_CENTER );

        smenu->label_pane = textpane_init( vg, fs_menu->fonts->fontatlas, fs_menu->fonts->italic, 130,  60, WIDTH, HEIGHT, TEXT_RIGHT );
        smenu->info_pane  = textpane_init( vg, fs_menu->fonts->fontatlas, fs_menu->fonts->body,   459, 270, WIDTH, HEIGHT, TEXT_LEFT );

        smenu->upgrade_pane = textpane_init( vg, fs_menu->fonts->fontatlas, fs_menu->fonts->body, 1100, 18, WIDTH, HEIGHT, TEXT_CENTER );
    }

#define PANE_OX  550
#define PANE_TOP 490
#define PANE_TOP_FU 350
#define PANE_SEP 30
    unsigned char text[512];
    unsigned char c;
    unsigned char *s;
    int j = 0;
    int i = 0;

    i = j = 0;
    s = locale_get_text( "SI_BUILD" );
    if( *s == '\0' ) s = NOTFOUND;
    while( j < (sizeof(text)-1) && (c = s[i++]) != 0 ) text[j++] = c;
    text[j++] = '\n';

    i = 0;
    s = locale_get_text( "SI_DATE" );
    if( *s == '\0' ) s = NOTFOUND;
    while( j < (sizeof(text)-1) && (c = s[i++]) != 0 ) text[j++] = c;
    text[j++] = '\0';

    build_text( smenu->label_pane, text, TEXT_COLOUR, PANE_OX - (130 + PANE_SEP / 2), PANE_TOP );


    i = j = 0;
    s = (unsigned char*)SystemInfo.release;
    while( j < (sizeof(text)-1) && (c = s[i++]) != 0 ) text[j++] = c;
    text[j++] = '\n';

    i = 0;
    s = (unsigned char*)SystemInfo.build_date;
    while( j < (sizeof(text)-1) && (c = s[i++]) != 0 ) text[j++] = c;
    text[j++] = '\0';

    build_text( smenu->info_pane, text, TEXT_COLOUR, PANE_OX + (PANE_SEP / 2), PANE_TOP );

    s = (unsigned char*)smenu->latest_firmware;

    if( s[0] ) {

        //set_item_visibility( smenu->fs_menu->menu, 1, 0 );
        //set_item_visibility( smenu->fs_menu->menu, 2, 1 );

        // IF Firmware upgrade available
        i = j = 0;
        s = locale_get_text( "SI_UPGRADE_FOUND" );
        if( *s == '\0' ) s = NOTFOUND;
        while( j < (sizeof(text)-1) && (c = s[i++]) != 0 ) text[j++] = c;
        // concatinate without linefeed
        text[j++] = ':';
        text[j++] = ' ';
        i = 0;
        //s = (unsigned char*)"redquark-one-1.0.0-argent-rc2";
        s = (unsigned char*)smenu->latest_firmware;
        while( j < (sizeof(text)-1) && (c = s[i++]) != 0 ) text[j++] = c;
        text[j++] = '\0';

        build_text( smenu->upgrade_pane, text, TEXT_COLOUR, (WIDTH - 1100) / 2, PANE_TOP_FU );

        s = locale_get_text( "SI_NEW_FIRMWARE" );
        if( *s == '\0' ) s = NOTFOUND;
        build_text( smenu->statement, s, TEXT_COLOUR, (WIDTH - 1100) / 2, 300 );
    } else {
        //set_item_visibility( smenu->fs_menu->menu, 1, 1 );
        //set_item_visibility( smenu->fs_menu->menu, 2, 0 );

        s = locale_get_text( "SI_UPGRADE_NOT_FOUND" );
        if( *s == '\0' ) s = NOTFOUND;
        build_text( smenu->upgrade_pane, s, TEXT_COLOUR, (WIDTH - 1100) / 2, PANE_TOP_FU );
    }
}

// ----------------------------------------------------------------------------------
//
static void
init_menu_cb( menu_t *m, void *private, int id, int *enabled, menu_item_dim_t *dim, menuSelectCallback *ms_cb  )
{
    FullScreenMenu *fs_menu = (FullScreenMenu *)private;
    SystemMenu     *smenu   = (SystemMenu *)(fs_menu->owner_private);

    if( id >= SI_MAX_TEXTS ) return;

    // If we're building the first menu item, then also build any other screen text/assets needed
    // because the language may have changed
    if( id == 0 ) {
        add_screen_text( smenu, fs_menu );
    }

    int found = 0;
    if( smenu->latest_firmware[0] ) {
        found = 1;
    } else {
        if( id == 1 ) { *enabled = 0; return; }
    }

    *enabled = 1;

    int x = menu_item[id].ox;
    int y = menu_item[id].oy;
    int w = menu_item[id].w;
    int h = menu_item[id].h;

    if( !found ) x = 640 - w/2;
    
    if( smenu->text[ id ] == NULL ) {
        textpane_t *tp = textpane_init( fs_menu->back_vertice_group, fs_menu->fonts->fontatlas, fs_menu->fonts->body, w, h, WIDTH, HEIGHT, TEXT_CENTER );

        smenu->text[ id ] = tp;
    }

    unsigned char *label = locale_get_text( menu_item[ id ].text );
    if( *label == '\0' ) label = NOTFOUND;

    textpane_t *tp = smenu->text[ id ];
    tp->largest_width = 0;
    build_text( tp, label, TEXT_COLOUR, x , y - 20 );

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
SystemMenu *
system_menu_init( Flashlight *f, fonts_t *fonts )
{
    SystemMenu *smenu = malloc( sizeof(SystemMenu) );
    if( smenu == NULL ) {
        return NULL;
    }
    memset( smenu, 0, sizeof(SystemMenu) );

    smenu->f = f;
    smenu->fonts  = fonts;

#define SIH (500)
    smenu->fs_menu = fsmenu_menu_init( f, fonts, init_menu_cb, (void *)smenu, "SI_TITLE", 2, 1, SIH, (SIH/2), FS_MENU_FLAG_NO_CLAIM_SCREEN | FS_MENU_FLAG_SUB_MENU | FS_MENU_FLAG_NO_ICONS | FS_MENU_FLAG_STACKED, OPAQUE );
    smenu->fs_menu->name = "System";

    return smenu;
}

// ----------------------------------------------------------------------------------

void
system_menu_finish( SystemMenu **m ) {
    free(*m);
    *m = NULL;
}

// ----------------------------------------------------------------------------------
//
static int
system_check_for_upgrade( SystemMenu *smenu )
{
    static int first_time = 1;
    int found = 0;

    int last_state = smenu->latest_firmware[0] ? 1 : 0;

    // Look for latest firmware on USB device
    uint32_t version;
    char * fw = check_for_upgrade( &version );

    smenu->latest_firmware[0] = '\0';

    if( fw != NULL ) {
        int l = strlen(fw);
        if( l < (sizeof( smenu->latest_firmware ) - 1) ) {

            // Make sure this is a greater version number
            if( version > SystemInfo.version ) {
                strcpy( smenu->latest_firmware, fw );

                found = 1;
            }
        }
    }

    //if( found ) {
    //    printf( "Found firmware %s\n", fw );
    //} else {
    //    printf( "Firmware not found\n" );
    //}

    if( (found != last_state) || first_time) {
        first_time = 0;

        fsmenu_menu_reload_items( smenu->fs_menu );
    }

    return found;
}

// ----------------------------------------------------------------------------------
//
static void
_select_cancel(menu_t *m, void *private, int id )
{
    FullScreenMenu *fs_menu = (FullScreenMenu *)private;
    SystemMenu     *smenu   = (SystemMenu *)(fs_menu->owner_private);

    fsmenu_menu_disable( smenu->fs_menu );
}

// ----------------------------------------------------------------------------------
//
static void
_select_confirm(menu_t *m, void *private, int id )
{
    FullScreenMenu   *fs_menu = (FullScreenMenu *)private;
    SystemMenu *smenu   = (SystemMenu *)(fs_menu->owner_private);

    ui_sound_play_ding();

    install_upgrade( smenu->latest_firmware );

    // Normally, a sucessfull install will reboot the machine, and not get here...
    fsmenu_menu_disable( smenu->fs_menu );
}

// ----------------------------------------------------------------------------------
//
//static void
//_select_check_upgrade(menu_t *m, void *private, int id )
//{
//    FullScreenMenu   *fs_menu = (FullScreenMenu *)private;
//    SystemMenu *smenu   = (SystemMenu *)(fs_menu->owner_private);
//
//    ui_sound_play_ding();
//
//    system_check_for_upgrade( smenu );
//}


// ----------------------------------------------------------------------------------
void
system_menu_enable( SystemMenu *smenu )
{
    menu_reset_current( smenu->fs_menu->menu ); 

    system_check_for_upgrade( smenu );

    fsmenu_menu_enable( smenu->fs_menu, 1 ); // 1 = hires selector
}

// ----------------------------------------------------------------------------------
void
system_menu_disable( SystemMenu *smenu )
{
    fsmenu_menu_disable( smenu->fs_menu );
}

// ----------------------------------------------------------------------------------
// end

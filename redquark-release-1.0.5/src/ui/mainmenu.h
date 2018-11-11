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
#ifndef MAINMENU_H
#define MAINMENU_H

#include <stdio.h>
#include <stdlib.h>

#include "flashlight.h"

#include "gamelist.h"
#include "gameshot.h"
#include "decoration.h"
#include "summary.h"
#include "selector.h"
#include "toolbar.h"
#include "ig_menu.h"
#include "screen_fader.h"

#include "uniforms.h"
#include "fonts.h"

#include "c64.h"

typedef enum {
    MM_State_None = 0,
    MM_Wait_For_Start_Fade = 1,
    MM_Wait_For_Fade_Out,
    MM_Wait_For_Quit_Message,
} mainmenu_state_t;

typedef struct {
    Flashlight  *f;

    GameList    *gamelist;
    Gameshot    *gameshot;
    C64Manager  *c64;
    GameSummary *summary;
    Decoration  *decoration;
    Selector    *selector;
    ToolBar     *toolbar;
    Fader       *fader;

    mainmenu_state_t    state;

    int          emulator_running;
    int          poweroff_mode;

    int          music_mute;
    
} MainMenu;

MainMenu * mainmenu_init( Flashlight *f, fonts_t *fonts, int test_mode );

void mainmenu_selector_set_target_position( int ox, int oy, int sx, int sy, uint32_t move_rate );
void mainmenu_selector_set_position( int ox, int oy, int sx, int sy );
void mainmenu_selector_reset_to_target();
int  mainmenu_selector_enable();
int  mainmenu_selector_disable();
void mainmenu_selector_set_lowres();
void mainmenu_selector_set_hires();
int mainmenu_selector_pop( void *owner );
int mainmenu_selector_push( void *owner );
void * mainmenu_who_has_focus();
int mainmenu_selector_owned_by( void *owner );
int mainmenu_selector_owned_by_and_ready( void *owner );

#endif

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
#ifndef SETTINGS_MENU_H
#define SETTINGS_MENU_H

#include <stdio.h>
#include <stdlib.h>

#include "flashlight.h"

#include "uniforms.h"
#include "fontatlas.h"
#include "fonts.h"
#include "textpane.h"
#include "fs_menu.h"
#include "ip_notice.h"
#include "keyboard_menu.h"
#include "factory_reset_menu.h"
#include "system_menu.h"

#define SM_MAX_TEXTS 4

typedef struct {
    textpane_t   *text[SM_MAX_TEXTS];
    fonts_t      *fonts; 

    FullScreenMenu *fs_menu;

    IPNotice         *ipnotice;
    KeyboardMenu     *kmenu;
    FactoryResetMenu *frmenu;
    SystemMenu       *simenu;

} SettingsMenu;

SettingsMenu * settings_menu_init( Flashlight *f, fonts_t *fonts );
void settings_menu_finish( SettingsMenu **m );
void settings_menu_enable( SettingsMenu *lmenu );
void settings_menu_disable( SettingsMenu *lmenu );

#endif

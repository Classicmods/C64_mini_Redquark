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
#ifndef SYSTEM_MENU_H
#define SYSTEM_MENU_H

#include <stdio.h>
#include <stdlib.h>

#include "flashlight.h"

#include "uniforms.h"
#include "fontatlas.h"
#include "fonts.h"
#include "textpane.h"
#include "fs_menu.h"

#define SI_MAX_TEXTS 3

typedef struct {
    Flashlight *f;

    textpane_t   *statement;
    textpane_t   *upgrade_pane;

    textpane_t   *label_pane;
    textpane_t   *info_pane;

    textpane_t   *text[SI_MAX_TEXTS];
    fonts_t      *fonts; 

    FullScreenMenu *fs_menu;

    char          latest_firmware[256];

} SystemMenu;

SystemMenu * system_menu_init( Flashlight *f, fonts_t *fonts );
void         system_menu_finish( SystemMenu **m );
void         system_menu_enable( SystemMenu *smenu );
void         system_menu_disable( SystemMenu *smenu );

#endif

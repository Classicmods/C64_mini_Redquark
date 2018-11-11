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
#ifndef DISPLAY_MENU_H
#define DISPLAY_MENU_H

#include <stdio.h>
#include <stdlib.h>

#include "flashlight.h"

#include "uniforms.h"
#include "fontatlas.h"
#include "fonts.h"
#include "textpane.h"
#include "fs_menu.h"

typedef struct {
    textpane_t   *text[6];
    fonts_t      *fonts; 

    FullScreenMenu *fs_menu;

} DisplayMenu;

DisplayMenu * display_menu_init( Flashlight *f, fonts_t *fonts );
void display_menu_finish( DisplayMenu **m );
void display_menu_set_selection( DisplayMenu *dmenu, display_mode_t mode );
void display_menu_disable( DisplayMenu *dmenu );
void display_menu_enable( DisplayMenu *dmenu );

#endif

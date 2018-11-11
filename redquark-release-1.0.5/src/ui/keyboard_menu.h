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
#ifndef KEYBOARD_MENU_H
#define KEYBOARD_MENU_H

#include <stdio.h>
#include <stdlib.h>

#include "flashlight.h"

#include "uniforms.h"
#include "fontatlas.h"
#include "fonts.h"
#include "textpane.h"
#include "fs_menu.h"

#define KM_MAX_TEXTS 6

typedef struct {
    textpane_t   *text[KM_MAX_TEXTS];
    fonts_t      *fonts; 

    FullScreenMenu *fs_menu;

} KeyboardMenu;

KeyboardMenu * keyboard_menu_init( Flashlight *f, fonts_t *fonts );
void keyboard_menu_finish( KeyboardMenu **m );
int  keyboard_menu_enable( KeyboardMenu *lmenu );
int  keyboard_menu_disable( KeyboardMenu *lmenu );
void keyboard_menu_set_selection( KeyboardMenu *kmenu, language_t keyboard );

#endif

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
#ifndef SUSPEND_MENU_H
#define SUSPEND_MENU_H

#include <stdio.h>
#include <stdlib.h>

#include "flashlight.h"

#include "uniforms.h"
#include "fontatlas.h"
#include "fonts.h"
#include "textpane.h"
#include "fs_menu.h"
#include "gamelibrary.h"
#include "suspendresume.h"

#define SAVE_LOAD_MAX_SLOTS 4

typedef enum {
    SLOT_NONE = 0,
    SLOT_SAVE,
    SLOT_LOAD,
    SLOT_DELETE,
} SlotAction;

typedef struct {
    Flashlight *f;

    textpane_t      *text[6];
    fonts_t         *fonts; 

    game_t          *game;
    FullScreenMenu  *fs_menu;
    FLTexture         *texture;

    int base_vert_count;
    int              text_background_index[SAVE_LOAD_MAX_SLOTS]; // Index of quad for slot text background rectangle

    SlotAction       slot_action[SAVE_LOAD_MAX_SLOTS];
    slot_meta_data_t slot_meta  [SAVE_LOAD_MAX_SLOTS];
    int              slot_x     [SAVE_LOAD_MAX_SLOTS];

} SuspendMenu;

SuspendMenu * suspend_menu_init( Flashlight *f, fonts_t *fonts );
void suspend_menu_enable( SuspendMenu *smenu );
void suspend_menu_disable( SuspendMenu *smenu );
#endif

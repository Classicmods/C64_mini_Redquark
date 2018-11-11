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
#ifndef IG_MENU_H
#define IG_MENU_H

#include <stdio.h>
#include <stdlib.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "flashlight.h"

#include "uniforms.h"
#include "fontatlas.h"
#include "fonts.h"
#include "textpane.h"
#include "gridmenu.h"
#include "suspend_menu.h"
#include "vkeyboard.h"

typedef enum {
    Menu_Open = 1,
    Menu_Closed,
    Menu_Exit,
    Menu_SuspendPoint,
    Menu_VirtualKeyboard,
} IGMenuState;

typedef struct {
    Flashlight *f;
    FLVerticeGroup *back_vertice_group;

    textpane_t   *text[4];
    fonts_t      *fonts; 

    FLTexture      *texture;
    int           base_vert_count;

    Uniforms uniforms;

    int          active;

    IGMenuState  state;

    menu_t *menu;

    SuspendMenu     *smenu;
    VirtualKeyboard *vkeyboard;

} InGameMenu;

InGameMenu * ig_menu_init( Flashlight *, fonts_t * );
#endif

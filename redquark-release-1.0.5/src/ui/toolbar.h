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
#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <stdio.h>
#include <stdlib.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "flashlight.h"

#include "uniforms.h"
#include "gridmenu.h"
#include "display_menu.h"
#include "locale_menu.h"
#include "settings_menu.h"

typedef enum {
    TOOL_NONE      = -1,
    TOOL_DISPLAY   = 0,
    TOOL_SETTINGS  = 1,
    TOOL_USB       = 2,
    TOOL_JOYSTICK  = 3,
    TOOL_MAX       = 4
} tool_id_t;

typedef struct {
    Flashlight  *f;

    FLVerticeGroup   *vertice_group;
    FLTexture        *texture;

    tool_id_t       tool_id;
    int             focus;


    int             active;

    Uniforms        uniforms;

    menu_t         *menu;

    DisplayMenu    *dmenu;
    LocaleMenu     *lmenu;
    SettingsMenu   *smenu;
} ToolBar;

ToolBar * toolbar_init( Flashlight *f, fonts_t *fonts );

#endif

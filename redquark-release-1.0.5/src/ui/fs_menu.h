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
#ifndef FULLSCREEN_MENU_H
#define FULLSCREEN_MENU_H

#include <stdio.h>
#include <stdlib.h>

#include "flashlight_vertice.h"
#include "flashlight_texture.h"
#include "flashlight_event.h"
#include "flashlight_shader.h"

#include "uniforms.h"
#include "fontatlas.h"
#include "fonts.h"
#include "textpane.h"
#include "gridmenu.h"

#define OPAQUE 0
#define TRANSPARENT 1

typedef enum {
    FS_MENU_FLAG_NONE             = 0,
    FS_MENU_FLAG_POINTER          = 1<<0,
    FS_MENU_FLAG_NO_CLAIM_SCREEN  = 1<<1,
    FS_MENU_FLAG_IGNORE_INPUT     = 1<<2,
    FS_MENU_FLAG_TRANSPARENT      = 1<<3,
    FS_MENU_FLAG_WARNING          = 1<<4,
    FS_MENU_FLAG_SUB_MENU         = 1<<5,
    FS_MENU_FLAG_NO_LOGO          = 1<<6,
    FS_MENU_FLAG_NO_ICONS         = 1<<7,
    FS_MENU_FLAG_STACKED          = 1<<8,
    FS_MENU_FLAG_MATCH_ITEM_WIDTH = 1<<9,
    FS_MENU_FLAG_SETTINGS_MENU    = 1<<10,
    FS_MENU_FULLSCREEN            = 1<<11,
} fs_menu_flags_t;

typedef struct {
    int   s;
    int   t;
    int   w;
    int   h;
    int   ox;
    int   oy;

    menu_item_marker_pos_t marker_pos;

    char *text;

    menuSelectCallback cb;  // Declared in gridmenu.h
} fs_item_t;

struct FullScreenMenu_s;
typedef struct FullScreenMenu_s FullScreenMenu;

struct FullScreenMenu_s {
    Flashlight   *f;

    FLVerticeGroup *back_vertice_group;
    FLTexture    *texture;

    textpane_t   *title_pane;

    fonts_t      *fonts; 
    Uniforms      uniforms;

    menu_t        *menu;

    char         *title_text_id;
    int           active_height;
    int           active;
    int           moving;
    int           has_claimed_screen;
    int           base_vert_count;

    int           overlaid;       // Used to disable/mute menus hidden behind the current one
    int           overlaid_input; // Used to disable/mute menus hidden behind the current one

    void         *owner_private;
    char         *name;

    language_t    lang; // current language
    fs_menu_flags_t flags;
};

FullScreenMenu *fsmenu_menu_init( Flashlight *f, fonts_t *fonts, menuInitCallback mi_cb, void *private, char *title, int cols, int rows, int active_y, int active_height, fs_menu_flags_t flags, int transparent );

int fsmenu_menu_disable( FullScreenMenu *fsmenu );
int fsmenu_menu_enable( FullScreenMenu *fsmenu, int hires );
void fsmenu_menu_reload_items( FullScreenMenu *fsmenu );
#endif

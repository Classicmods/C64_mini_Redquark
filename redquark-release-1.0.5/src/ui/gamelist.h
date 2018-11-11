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
#ifndef GAMELIST_MGR_H
#define GAMELIST_MGR_H

#include <stdio.h>
#include <stdlib.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "flashlight_vertice.h"
#include "flashlight_texture.h"
#include "flashlight_event.h"
#include "flashlight_shader.h"

#include "carousel.h"
#include "gamelibrary.h"

typedef enum {
    BY_TITLE = 1,
    BY_AUTHOR,
    BY_YEAR,
    BY_MUSIC
} GameListSort;

typedef struct {
    game_t *game;
    int     primary_controller_id;
} SelectedGame;

typedef struct {
    Flashlight *f;
    Carousel   *carousel;

    gamelibrary_t *library;

    GameListSort   sort_order;

    int focus;

    SelectedGame selected_game;

} GameList;

GameList * gamelist_init( Flashlight *f );
int        gamelist_has_focus( GameList *gl );

#endif

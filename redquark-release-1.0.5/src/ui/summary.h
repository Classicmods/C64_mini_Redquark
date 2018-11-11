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
#ifndef GAMESUMMARY_H
#define GAMESUMMARY_H

#include <stdio.h>
#include <stdlib.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "flashlight.h"

#include "uniforms.h"
#include "fontatlas.h"
#include "fonts.h"
#include "gamelibrary.h"
#include "textpane.h"
#include "settings.h"

#define GAME_SLOTS 2

typedef enum {
    GAMESUMMARY_STATIC = 0,
//    GAMESUMMARY_FADE_IN,
//    GAMESUMMARY_FADE_OUT,
    GAMESUMMARY_FADE_DELAY
} GameSummaryState;

typedef struct {
    int fade_level[GAME_SLOTS];
    int active_slot;
    int inactive_slot;
    int fade_delay_frame_count;
    int redraw;

    game_t *game;

    GameSummaryState state;

    FLVerticeGroup *vertice_group_background;
    FLVerticeGroup *vertice_groups[GAME_SLOTS];

    textpane_t   *desc_pane  [GAME_SLOTS];
    textpane_t   *title_pane [GAME_SLOTS];
    textpane_t   *label_pane [GAME_SLOTS];
    textpane_t   *credit_pane[GAME_SLOTS];
    textpane_t   *yearl_pane [GAME_SLOTS];
    textpane_t   *year_pane  [GAME_SLOTS];

    language_t language;

    int        display_enabled;

    fonts_t      *fonts; 

    Flashlight    *f;
    FLTexture      *texture;

    Uniforms uniforms;
} GameSummary;

GameSummary * gamesummary_init( Flashlight *f, fonts_t *fonts );

#endif

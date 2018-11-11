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
#ifndef SUSPEND_RESUME_H
#define SUSPEND_RESUME_H

#include "flashlight_texture.h"
#include "gamelibrary.h"

typedef struct {
    unsigned int play_duration;
} slot_meta_data_t;

char * get_game_saves_dir( game_t *game );

int suspend_load_slot_image( FLTexture *tx, game_t *game, int slot );
int suspend_load_slot_metadata( slot_meta_data_t *meta, game_t *game, int slot );

int suspend_game( game_t *game, int slot );
int resume_game( game_t *game, int slot );

#endif

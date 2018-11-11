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
#ifndef UI_SOUND_H
#define UI_SOUND_H

#include "flashlight.h"

void ui_sound_init();
void ui_sound_deinit();
void ui_sound_play_ding();
void ui_sound_play_menu_open();
void ui_sound_play_menu_close();
void ui_sound_play_flip();
void ui_sound_play_flip_delayed();
void ui_sound_play_music();
void ui_sound_stop_ding();
void ui_sound_stop_flip();
void ui_sound_stop_music();

void ui_sound_wait_for_ding();

#endif

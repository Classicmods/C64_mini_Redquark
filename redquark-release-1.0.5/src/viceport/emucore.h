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
#ifndef EMU_CORE_H
#define EMU_CORE_H

#include "autostart.h"
#include "joystick.h"

#include "videoarch.h"

// core.c
unsigned char* start_c64();
void stop_c64();
void c64_exit(void);
void switch_to_model_ntsc();
void switch_to_model_pal();

// video.c
unsigned char *c64_create_screen();

// vsyncarch.c
void vsyncarch_sync_with_raster(video_canvas_t *c);

int delete_tempory_files();

void ui_set_vertical_shift( int adjust );

#endif

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
#ifndef UI_JOYSTICKS_H
#define UI_JOYSTICKS_H

#include "tsf_joystick.h"
#include "event.h"
#include "joystick.h"

void ui_init_joysticks( Flashlight *f );
void ui_scan_for_joysticks();
void ui_read_joysticks();
int  ui_get_joystick_count();
int  ui_joystick_get_port_for_id( int id );

unsigned long ui_joystick_last_event_time();
void joystick_queue_event( int port_id, uint32_t value );
void joystick_process_queue();
void keyboard_queue_key( uint32_t key, int pressed );
void joystick_clear_queues();
void ui_clear_joystick();

#endif

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
#ifndef PORTUI_H
#define PORTUI_H

int  ui_pause_emulation(int flag);
void load_snapshot( unsigned char *filename );
void save_snapshot( unsigned char *filename );

extern int pause_target_state;
extern int pause_current_state;

// ----------------------------------------------------------------------------------
//
static inline int
ui_can_change_pause_state()
{
    return (pause_target_state == pause_current_state);
}

// ----------------------------------------------------------------------------------
//
static inline int
ui_is_paused()
{
    return (pause_current_state == 1);
    //return ((pause_target_state == 1) && (pause_target_state == pause_current_state));
}

// ----------------------------------------------------------------------------------

#endif

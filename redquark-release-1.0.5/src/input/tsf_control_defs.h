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
#ifndef TSF_CONTROL_DEFS_H
#define TSF_CONTROL_DEFS_H

typedef enum {
    // XXX enum is a SIGNED INT so AVOID using bit 31 XXX
    INPUT_NONE        = 0,

    INPUT_LEFT        = 1<<9,
    INPUT_RIGHT       = 1<<10,
    INPUT_UP          = 1<<11,
    INPUT_DOWN        = 1<<12,
    _INPUT_DIR_X      = 1<<13,
    _INPUT_DIR_Y      = 1<<14,
    _INPUT_DIG        = 1<<15,
    _INPUT_SHLD       = 1<<24,
    _INPUT_TRIG       = 1<<25,
    _INPUT_BUTTON     = 1<<26,
    _INPUT_STICK      = 1<<27,
    _INPUT_AXIS       = 1<<28,
    _INPUT_LOC_LEFT   = 1<<29,
    _INPUT_LOC_RIGHT  = 1<<30,
                        
    INPUT_CTRL_MASK   = ~((uint32_t)INPUT_LEFT - 1),

    _INPUT_AXIS_LEFT  = _INPUT_AXIS | _INPUT_LOC_LEFT,
    _INPUT_AXIS_RIGHT = _INPUT_AXIS | _INPUT_LOC_RIGHT,

    INPUT_X            = _INPUT_BUTTON | 1<<0,
    INPUT_Y            = _INPUT_BUTTON | 1<<1,
    INPUT_A            = _INPUT_BUTTON | 1<<2,
    INPUT_B            = _INPUT_BUTTON | 1<<3,
    INPUT_START        = _INPUT_BUTTON | 1<<4,
    INPUT_GUIDE        = _INPUT_BUTTON | 1<<5,
    INPUT_BACK         = _INPUT_BUTTON | 1<<5,

    INPUT_DIG_UP       = _INPUT_BUTTON | _INPUT_DIG   | INPUT_UP,
    INPUT_DIG_DOWN     = _INPUT_BUTTON | _INPUT_DIG   | INPUT_DOWN,
    INPUT_DIG_LEFT     = _INPUT_BUTTON | _INPUT_DIG   | INPUT_LEFT,
    INPUT_DIG_RIGHT    = _INPUT_BUTTON | _INPUT_DIG   | INPUT_RIGHT,

    INPUT_SHLD_LEFT    = _INPUT_BUTTON | _INPUT_SHLD  | _INPUT_LOC_LEFT,
    INPUT_SHLD_RIGHT   = _INPUT_BUTTON | _INPUT_SHLD  | _INPUT_LOC_RIGHT,
    INPUT_TRIG_LEFT    = _INPUT_BUTTON | _INPUT_TRIG  | _INPUT_LOC_LEFT,
    INPUT_TRIG_RIGHT   = _INPUT_BUTTON | _INPUT_TRIG  | _INPUT_LOC_RIGHT,

    INPUT_STICK_LEFT   = _INPUT_BUTTON | _INPUT_STICK | _INPUT_LOC_LEFT,
    INPUT_STICK_RIGHT  = _INPUT_BUTTON | _INPUT_STICK | _INPUT_LOC_RIGHT,

    INPUT_AXIS_LEFT_X        = _INPUT_AXIS_LEFT  | _INPUT_DIR_X,  
    INPUT_AXIS_LEFT_X_LEFT   = _INPUT_AXIS_LEFT  | _INPUT_DIR_X | INPUT_LEFT,  
    INPUT_AXIS_LEFT_X_RIGHT  = _INPUT_AXIS_LEFT  | _INPUT_DIR_X | INPUT_RIGHT,  

    INPUT_AXIS_LEFT_Y        = _INPUT_AXIS_LEFT  | _INPUT_DIR_Y,  
    INPUT_AXIS_LEFT_Y_UP     = _INPUT_AXIS_LEFT  | _INPUT_DIR_Y | INPUT_UP,  
    INPUT_AXIS_LEFT_Y_DOWN   = _INPUT_AXIS_LEFT  | _INPUT_DIR_Y | INPUT_DOWN,  

    INPUT_AXIS_RIGHT_X       = _INPUT_AXIS_RIGHT | _INPUT_DIR_X,  
    INPUT_AXIS_RIGHT_X_LEFT  = _INPUT_AXIS_RIGHT | _INPUT_DIR_X | INPUT_LEFT,  
    INPUT_AXIS_RIGHT_X_RIGHT = _INPUT_AXIS_RIGHT | _INPUT_DIR_X | INPUT_RIGHT,  

    INPUT_AXIS_RIGHT_Y       = _INPUT_AXIS_RIGHT | _INPUT_DIR_Y,  
    INPUT_AXIS_RIGHT_Y_UP    = _INPUT_AXIS_RIGHT | _INPUT_DIR_Y | INPUT_UP,  
    INPUT_AXIS_RIGHT_Y_DOWN  = _INPUT_AXIS_RIGHT | _INPUT_DIR_Y | INPUT_DOWN,  

} tsf_input_type_t;

#endif

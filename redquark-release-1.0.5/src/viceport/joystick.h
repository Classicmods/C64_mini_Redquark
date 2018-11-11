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
#ifndef THE64_JOYSTICK_MAP
#define THE64_JOYSTICK_MAP

#include <stdint.h>
#include <tsf_control_defs.h>

// From vice/src/joystick.h
void joystick_set_value_absolute(unsigned int joyport, unsigned char value);

#define JOY_MAX_PORT 4

typedef enum {
    JoyPortUp    = 1<<0,
    JoyPortDown  = 1<<1,
    JoyPortLeft  = 1<<2,
    JoyPortRight = 1<<3,
    JoyPortFire  = 1<<4,
} joy_port_value_t;

typedef enum {
    JE_NONE = -1,
    JE_UP = 0,
    JE_DOWN,
    JE_LEFT,
    JE_RIGHT,
    JE_TRIGGER_LEFT,
    JE_TRIGGER_RIGHT,
    JE_Y,
    JE_X,
    JE_SHOULDER_LEFT, // Not a THE64 Joystick button
    JE_A,
    JE_B,
    JE_BACK,
    JE_SHOULDER_RIGHT, // Not a THE64 Joystick button
    JE_MAX_MAP
} joy_input_id_t;

typedef enum {
    JoyPort  = 0,
    JoyKeyboard = 1,
    Map_Type_Max = JoyKeyboard,
} joy_map_type_t;

typedef struct {
    joy_map_type_t type;
    uint32_t       value;   // Cast to joy_port_value_t if type == JoyPort
} joystick_map_t;


// --------------------------------------------------------------------------------
// portid values between 0..3
void joystick_process( int portid, uint32_t event, int set );
void joystick_set_port_map( int portid, joy_input_id_t input, joy_map_type_t type, uint32_t val );
void joystick_set_primary_port( int portid );
void joystick_clone_port_map( int port_id, joystick_map_t *src );

// --------------------------------------------------------------------------------
#endif

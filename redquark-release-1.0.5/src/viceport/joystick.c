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
#include <string.h>
#include <stdio.h>
#include "joystick.h"
#include "keyboard.h"

//#define JSDEBUG
#ifdef JSDEBUG
#   define printd(...) printf(__VA_ARGS__)
#else
#   define printd(...) {;} //(0)
#endif

#define MAX_KEY_VALUE 256
static joystick_map_t port_map [ JOY_MAX_PORT ][ JE_MAX_MAP ] = {0};
static int            key_map  [ JOY_MAX_PORT ][ JE_MAX_MAP ] = {0};
static int            key_ix   [ JOY_MAX_PORT ] = {0};
static char key_track_old[ MAX_KEY_VALUE ] = {0};
static char key_track_new[ MAX_KEY_VALUE ] = {0};

// --------------------------------------------------------------------------------
int
joy_arch_init(void) 
{ 
    int i;
    for( i = 0; i < JOY_MAX_PORT; i++ ) {
        memset( port_map, i, sizeof(port_map) );

        joystick_set_port_map( i + 1, JE_UP,    JoyPort, JoyPortUp    );
        joystick_set_port_map( i + 1, JE_DOWN,  JoyPort, JoyPortDown  );
        joystick_set_port_map( i + 1, JE_LEFT,  JoyPort, JoyPortLeft  );
        joystick_set_port_map( i + 1, JE_RIGHT, JoyPort, JoyPortRight );
        joystick_set_port_map( i + 1, JE_X,     JoyPort, JoyPortFire  );
        joystick_set_port_map( i + 1, JE_Y,     JoyPort, JoyPortFire  );

        joystick_set_port_map( i + 1, JE_TRIGGER_RIGHT,  JoyPort, JoyPortFire  );
        joystick_set_port_map( i + 1, JE_TRIGGER_LEFT,   JoyPort, JoyPortFire  );
        joystick_set_port_map( i + 1, JE_SHOULDER_RIGHT, JoyPort, JoyPortFire  );
        joystick_set_port_map( i + 1, JE_SHOULDER_LEFT,  JoyPort, JoyPortFire  );
    }

    return 0; 
}

// --------------------------------------------------------------------------------
void joystick_close(void) { }
int joystick_arch_init_resources(void)  { return 0; }
int joystick_init_cmdline_options(void) { return 0; }

// --------------------------------------------------------------------------------
// port_id values between 1..4
void
joystick_process( int port_id, uint32_t event, int set )
{
    printd("joystick_process port_id %d  event %d\n", port_id, event );
    if( port_id < 1 || port_id > JOY_MAX_PORT ) return;

    joystick_map_t *pm = port_map[ port_id - 1 ];

    joystick_map_t *active[ JE_MAX_MAP ] = {0};
    int aix = 0;

    // Collect up an array of all the active controls
    event &= ~_INPUT_BUTTON;
    if( event & _INPUT_SHLD ) active[ aix++ ] = pm + ((event & _INPUT_LOC_LEFT) ? JE_SHOULDER_LEFT : JE_SHOULDER_RIGHT);
    if( event & _INPUT_TRIG ) active[ aix++ ] = pm + ((event & _INPUT_LOC_LEFT) ? JE_TRIGGER_LEFT  : JE_TRIGGER_RIGHT);
    if( event &  INPUT_X    ) active[ aix++ ] = pm + JE_X;
    if( event &  INPUT_Y    ) active[ aix++ ] = pm + JE_Y;
    if( event &  INPUT_A    ) active[ aix++ ] = pm + JE_A;
    if( event &  INPUT_B    ) active[ aix++ ] = pm + JE_B;
    if( event &  INPUT_BACK ) active[ aix++ ] = pm + JE_BACK;
    if( event &  INPUT_LEFT ) active[ aix++ ] = pm + JE_LEFT;
    if( event &  INPUT_RIGHT) active[ aix++ ] = pm + JE_RIGHT;
    if( event &  INPUT_UP   ) active[ aix++ ] = pm + JE_UP;
    if( event &  INPUT_DOWN ) active[ aix++ ] = pm + JE_DOWN;

    // get ready to release existing keys, unless they are still held
    // Loop through key_ix. setting release elements 

    int i;
    uint32_t pv = 0;
    // Set appropriate bits for each Joystick control, or press the required key
    for( i = 0; i < aix; i++ ) {
        if( active[i]->type == JoyPort ) pv |= active[i]->value;

        // Set flag in key array, so we know that it should be pressed (or continue to be pressed).
        // Any flag we do not set that is indexed by key_map (see clone_port) will have its
        // key released.
        if( active[i]->type == JoyKeyboard && active[i]->value < MAX_KEY_VALUE ) {
            key_track_new[ active[i]->value ] = 1;
        }
    }

    // 
    int kx  = key_ix [ port_id - 1 ];
    int *km = key_map[ port_id - 1];

    for( i = 0; i < kx; i++ ) {
        if( key_track_new[ km[i] ] == 0 && key_track_old[ km[i] ] == 1 ) {
            // Key was set and is no longer, so release
            printd("Releasing Keyboard key 0x%x  (id %d)\n", km[i], i );
            key_track_old[ km[i] ] = key_track_new[ km[i] ];

            keyboard_key_released( km[i] );
        } else
        if( key_track_new[ km[i] ] == 1 &&  key_track_old[ km[i] ] == 0 ) {
            // Key was not set but should be, so press.
            printd("Sending Keyboard key 0x%x  (id %d)\n", km[i], i );
            key_track_old[ km[i] ] = key_track_new[ km[i] ];

            keyboard_key_pressed( km[i] );
        }
    }
    for( i = 0; i < kx; i++ ) {
        key_track_new[ km[i] ] = 0;
    }

    printd("Sending JS %d value 0x%x\n", port_id, pv & 0xff );
    joystick_set_value_absolute( port_id, pv & 0xff );
}

// --------------------------------------------------------------------------------
// Configures C64 port, mapping controller inputs to C64 joystick and keyboard functions.
void
joystick_set_port_map( int port_id, joy_input_id_t input, joy_map_type_t type, uint32_t val )
{
    if( port_id < 1 || port_id > JOY_MAX_PORT  || type > Map_Type_Max || input >= JE_MAX_MAP ) return;

    printd( "Mapping input %d to type %d value %d\n", input, type, val );

    port_map[ port_id - 1 ][ input ].type  = type;
    port_map[ port_id - 1 ][ input ].value = val;
}

// --------------------------------------------------------------------------------
//
void
joystick_clone_port_map( int port_id, joystick_map_t *src_map )
{
    if( port_id < 1 || port_id > JOY_MAX_PORT ) return;
    memcpy( port_map[ port_id - 1 ], src_map, sizeof( joystick_map_t ) * JE_MAX_MAP );

    // For every "button" that may be a key, create an index array so we can track
    // when the "key" should be released.
    int i;
    key_ix[port_id -1] = 0;
    for( i == 0; i < JE_MAX_MAP; i++ ) {
        if( src_map[i].type == JoyKeyboard && key_ix[port_id -1] < JE_MAX_MAP ) {
            key_map[ port_id - 1 ][ key_ix[port_id - 1]++ ] = src_map[i].value;
        }
    }

    joystick_set_value_absolute( port_id, 0x0 ); // initialise port
}

// --------------------------------------------------------------------------------

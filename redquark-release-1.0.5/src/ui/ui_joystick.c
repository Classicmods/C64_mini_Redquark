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

#include <stdio.h>
#include "path.h"
#include "ui_joystick.h"
#include "keyboard.h"
#include "flashlight.h"
//#define MAPPING_DEBUG

static tsf_joystick_t *joysticks;
static unsigned long joy_value;
static unsigned long last_event_time;
static Flashlight *flash;

extern unsigned long frame_time_ms;

// ----------------------------------------------------------------------------------
//
int
ui_get_joystick_count()
{
    return tsf_active_joystick_count( joysticks );
}

// ----------------------------------------------------------------------------------
//
unsigned long
ui_joystick_last_event_time()
{
    return last_event_time;
}

// ----------------------------------------------------------------------------------
// Returns the port (1..MAX_JOYSTICK) mapped to the given josytick id
// (This mapping is on a first come, first served basis. If we loose a JS, then
//  the port numbers are re-assigned to leave no holes).
//
//  Returns 0 if requested port is out of range.
int
ui_joystick_get_port_for_id( int id )
{
    //printf("Get virtual port for JS id %d (js count: %d\n", id, joysticks->js_count );
    if( id >= MAX_JOYSTICKS ) return 0; 

    return joysticks->joystick_map[ id ];
}

// ----------------------------------------------------------------------------------
//
void
ui_scan_for_joysticks()
{
    last_event_time = frame_time_ms;

    int found_new = tsf_scan_for_joystick( joysticks );

    if( found_new < 0 ) {
        printf("Error scanning joysticks\n");
        return;
    }

    if( found_new ) {
        tsf_enumerate_joysticks( joysticks );
        int r = tsf_reconfigure_from_sdl_map( joysticks, get_path( "/usr/share/the64/ui/data/gamecontrollerdb.txt" ) );
        if( r < 0 ) {
            // What to do here?
        }
    }
}

// ----------------------------------------------------------------------------------
//
void
ui_init_joysticks( Flashlight *f )
{
    joysticks = tsf_initialise_joysticks();

    ui_scan_for_joysticks();

    flash = f;
}

// ----------------------------------------------------------------------------------
//
static void 
receive_joystick_event( tsf_joystick_device_t *js, tsf_input_type_t ident, int val )
{
#ifdef MAPPING_DEBUG
    printf("callback got event %x = %x\n", ident, val );
    if(val > 0) printf( "PRESS\n" ); else printf( "RELEASE\n" ); 
    printf("Name: %s\n", tsf_joystick_id_to_string( ident ) );
    printf("\n");
#endif

    unsigned ojv = joy_value;

    if( js->type == JOY_JOYSTICK ) {
        if( val > 0 ) { // Pressed
            joy_value |= ident;
        } else { // Released

            if( ident & _INPUT_DIR_X ) ident |= (INPUT_LEFT | INPUT_RIGHT); // | We get told which axis was
            if( ident & _INPUT_DIR_Y ) ident |= (INPUT_UP   | INPUT_DOWN ); // | released, not what "button"

            joy_value &= ~ident;
        }
        ojv &= ~joy_value;
        if( ojv != joy_value ) {
            TSFEventNavigate e = {{ Navigate, NULL }};
            e.direction = joy_value;
            e.type = js->type;
            e.id   = js->id;
            e.vport= joysticks->joystick_map[ js->id ]; // Map JS ID to Virtual port
            flashlight_event_publish( flash,(FLEvent*)&e );
        }

        // If joystick is pressed, cancel last_event_time.
        // If released, set last_event_time.
        if( joy_value ) last_event_time = 0;
        else if( last_event_time == 0 ) last_event_time = frame_time_ms;
    }
    if( js->type == JOY_KEYBOARD || js->type == JOY_SWITCHES ) {
        TSFEventKeyboard e = {{ Keyboard, NULL }};
        e.key = ident;
        e.is_pressed = val ? 1 : 0;
        flashlight_event_publish( flash,(FLEvent*)&e );

        if( e.is_pressed ) last_event_time = 0;
        else last_event_time = frame_time_ms;
    }
}

// ----------------------------------------------------------------------------------
//
void
ui_read_joysticks()
{
    tsf_read_joystick( joysticks, receive_joystick_event, 0 );
}

// ----------------------------------------------------------------------------------
//
static void 
_dump_joystick_event( tsf_joystick_device_t *js, tsf_input_type_t ident, int val )
{
    js = js;
    ident = ident;
    val = val;
}

// ----------------------------------------------------------------------------------
// Dumps any pending joystick input
void
ui_clear_joystick()
{
    tsf_read_joystick( joysticks, _dump_joystick_event, 0 );
    joy_value = 0; // Clear cached state
}

// ----------------------------------------------------------------------------------
#define MAX_KEYBOARD_QUEUE      64
static uint32_t port_value_queue[JOY_MAX_PORT] = {0};
static uint32_t port_value_set  [JOY_MAX_PORT] = {0};
static uint32_t keyboard_queue  [MAX_KEYBOARD_QUEUE] = {0};
static int      keyboard_queue_len             = 0;

void
joystick_queue_event( int port_id, uint32_t value )
{
    if( port_id < 1 || port_id > JOY_MAX_PORT ) return;
    port_value_queue[port_id - 1] = value;
    port_value_set  [port_id - 1] = 1;
}

// ----------------------------------------------------------------------------------
//
void
keyboard_queue_key( uint32_t key, int pressed )
{
    if( !key || keyboard_queue_len >= MAX_KEYBOARD_QUEUE ) return;
    keyboard_queue[ keyboard_queue_len++ ] = key | ( pressed ? (uint32_t)(1<<31) : 0 );
}

// ----------------------------------------------------------------------------------
//
void
joystick_process_queue()
{
    if( port_value_set[0] ) { port_value_set[0] = 0; joystick_process( 1, port_value_queue[0], 0 ); }
    if( port_value_set[1] ) { port_value_set[1] = 0; joystick_process( 2, port_value_queue[1], 0 ); }
    if( port_value_set[2] ) { port_value_set[2] = 0; joystick_process( 3, port_value_queue[2], 0 ); }
    if( port_value_set[3] ) { port_value_set[3] = 0; joystick_process( 4, port_value_queue[3], 0 ); }

    if(  keyboard_queue_len ) {
        int i;
        uint32_t v;
        for(i = 0; i < keyboard_queue_len; i++ ) {
            v = keyboard_queue[ i ];

            if( v & 1<<31 ) keyboard_key_pressed((unsigned long)(v & ((uint32_t)(1<<31) - 1)) );
            else            keyboard_key_released((unsigned long)v);
        }
        keyboard_queue_len = 0;
    }
}

// ----------------------------------------------------------------------------------
//
void
joystick_clear_queues()
{
    int port_id;
    for( port_id = 1; port_id <= JOY_MAX_PORT; port_id++ ) {
        port_value_queue[port_id - 1] = 0;
        port_value_set  [port_id - 1] = 0;
    }
    keyboard_queue_len = 0;
}

// ----------------------------------------------------------------------------------

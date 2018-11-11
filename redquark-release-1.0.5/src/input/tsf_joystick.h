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
#ifndef TSF_JOYSTICK_H
#define TSF_JOYSTICK_H

#include <stdint.h>
#include <poll.h>
#include <libevdev/libevdev.h>
#include "tsf_control_defs.h"

#define MAX_JOYSTICKS 4
#define MAX_KEYBOARDS 1
#define MAX_SWITCHES  2
#define MAX_DEVICES   (MAX_JOYSTICKS + MAX_KEYBOARDS + MAX_SWITCHES)

#define DEREGISTER_QUEUE_MAX 32

typedef enum {
    JOY_UNKNOWN = 0,
    JOY_JOYSTICK,
    JOY_KEYBOARD,
    JOY_SWITCHES,   // Things like power buttons, etc
} jstype_t;

typedef struct {
    struct libevdev *evdev;
    jstype_t         type;
    int              id;
    char             device_path[256];
    uint16_t         guid[8];
    char             guidhex[33];

    uint32_t         button[ KEY_MAX ];         // Map between ev button code and local button number
    uint32_t         button_index[ KEY_MAX ];   // Index of button number 0...n-1 into button[]
    int              button_count;

    uint32_t         axis[ ABS_MAX ];           // Map between ev axis code and local axis number
    uint32_t         axis_index[ KEY_MAX ];     // Index of axis number 0...n-1 into axis[]
    int              axis_count;
    int              axis_min[ ABS_MAX ];
    int              axis_max[ ABS_MAX ];

    uint32_t         hat[ ABS_MAX ];           // Map between ev axis code and local axis number
    uint32_t         hat_index[ KEY_MAX ];     // Index of axis number 0...n-1 into axis[]
    int              hat_count;
    int              hat_min[ ABS_MAX ];
    int              hat_max[ ABS_MAX ];
    
    int              remapped;
    int              enumerated;
} tsf_joystick_device_t;

typedef struct {
    tsf_joystick_device_t joystick[ MAX_DEVICES ];
    int                   count;
    int                   js_count;
    int                   sw_count;
    int                   key_count;
    struct pollfd         pollfds[ MAX_DEVICES ];

    int                   joystick_map[ MAX_JOYSTICKS ];
    int                   deregister_queue[ DEREGISTER_QUEUE_MAX ];
    int                   deregister_queue_count;
} tsf_joystick_t;

// jsEventHandler is the callback prototype for a joystick event
typedef void (*jsEventHandler)(tsf_joystick_device_t *js, tsf_input_type_t etype, int value);

// ------------------------------------------------------------------------------------------

tsf_joystick_t * tsf_initialise_joysticks();
int              tsf_scan_for_joystick       ( tsf_joystick_t * j );
int              tsf_enumerate_joysticks     ( tsf_joystick_t *j );
int              tsf_reconfigure_from_sdl_map( tsf_joystick_t *j, const char *fname );
int              tsf_read_joystick           ( tsf_joystick_t *j, jsEventHandler cb, int timeout );
int              tsf_active_joystick_count   ( tsf_joystick_t *j );
char           * tsf_joystick_id_to_string   ( tsf_input_type_t type );

#endif

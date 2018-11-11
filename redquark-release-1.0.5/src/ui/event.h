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
#ifndef TSF_EVENT_H
#define TSF_EVENT_H

#include "flashlight_event.h"

typedef enum {
    // Events 0 and 1 are reserved by Flashlight
    Navigate     = 1<<2,
    Process      = 1<<3,
    Redraw       = 1<<4,
    WaitForVSync = 1<<5,
    VSyncDone    = 1<<6,
    KeyUp        = 1<<7,
    KeyDown      = 1<<8,
    UserEvent    = 1<<9,
    FocusChange  = 1<<10,
    Quit         = 1<<11,
    Keyboard     = 1<<12,
    Setting      = 1<<13,
    PowerOff     = 1<<30,
} TSFEventType;

typedef struct {
    FLEvent e;
    int      user_type;
    void        *data;
} TSFUserEvent;

// ----------------------------------------------------------------------------
//
typedef struct {
    FLEvent e;
    void    *who;       // Who now has the input focus
} TSFFocusChange;

// ----------------------------------------------------------------------------
//
typedef struct {
    FLEvent e;
    int      direction;
    int      type;
    int      id;
    int      vport;
} TSFEventNavigate;

// ----------------------------------------------------------------------------
//
typedef struct {
    FLEvent e;
    int      key;
    int      is_pressed;
} TSFEventKeyboard;

// ----------------------------------------------------------------------------
//
typedef struct {
    FLEvent e;
    int      key;
} TSFEventKey;

// ----------------------------------------------------------------------------
//
typedef struct {
    FLEvent e;
} TSFEventRedraw;

// ----------------------------------------------------------------------------
//
typedef struct {
    FLEvent e;
} TSFEventProcess;

// ----------------------------------------------------------------------------
//
typedef struct {
    FLEvent e;
} TSFEventWaitForVSync;

// ----------------------------------------------------------------------------
//
typedef struct {
    FLEvent e;
} TSFEventVSyncDone;
            
// ----------------------------------------------------------------------------
//
typedef struct {
    FLEvent e;
    int type;
} TSFEventQuit;

// ----------------------------------------------------------------------------
//
typedef struct {
    FLEvent e;
    int type;
} TSFEventPowerOff;

// ----------------------------------------------------------------------------
//
typedef struct {
    FLEvent e;
    int      id;
    void    *data;
} TSFEventSetting;

#endif

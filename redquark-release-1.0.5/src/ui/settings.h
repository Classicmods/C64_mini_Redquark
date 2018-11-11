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
#ifndef SETTINGS_H
#define SETTINGS_H

#include "event.h"

#include "flashlight.h"

typedef enum {
    DisplayMode = 1,
    Language,
    KeyboardRegion,
} setting_id_t;

typedef enum {
    _PIXEL     = 1<<1,
    _EU        = 1<<2,
    _US        = 1<<3,
    _SHARP     = 1<<4,
    _CRT       = 1<<5,

    PIXEL_SHARP = _PIXEL | _SHARP,
    EU_SHARP    = _EU    | _SHARP,
    US_SHARP    = _US    | _SHARP,

    PIXEL_CRT   = _PIXEL | _CRT,
    EU_CRT      = _EU    | _CRT,
    US_CRT      = _US    | _CRT,
} display_mode_t;

typedef enum {
    //_LANG_CURRENT = 0, //  Internal reference to current selected language slot

    LANG_NONE  = 0,
    LANG_EN    = 1,
    LANG_DE    = 2,
    LANG_FR    = 3,
    LANG_ES    = 4,
    LANG_IT    = 5,
    LANG_MASK  = (1<<6) - 1,
    LANG_EN_UK = LANG_EN | 1<<6,
    LANG_EN_US = LANG_EN | 1<<7,
    LANG_MIN   = LANG_EN,
    LANG_MAX   = LANG_IT
} language_t;

typedef struct {
    display_mode_t display_mode;
    language_t     language;
    language_t     keyboard;
} settings_t;

int  settings_save();
int  settings_load( Flashlight *f );

language_t     settings_get_language();
void           settings_set_language( language_t lang );

language_t     settings_get_keyboard();
void           settings_set_keyboard( language_t keyboard );

display_mode_t settings_get_display_mode();
void           settings_set_display_mode( display_mode_t mode );

#endif

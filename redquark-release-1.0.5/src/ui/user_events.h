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
#ifndef USER_EVENTS_H
#define EVENTS_H

#include "flashlight_event.h"
#include "event.h"

typedef enum {
    OpenTestMode = 1<<3,
    FSMenuOpened          = 1<<4, // Only sent if FS_MENU_FLAG_STACKED set
    FSMenuClosing         = 1<<5, // Only sent if FS_MENU_FLAG_STACKED set
    SaveGameStateRequest  = 1<<6,
    LoadGameStateRequest  = 1<<7,
    OpenLanguageSelection = 1<<8,
    InputClaim            = 1<<9,
    VirtualKeyboardChange = 1<<10,
    MenuClosed            = 1<<11,
    GameshotLoaded        = 1<<12,
    Pause                 = 1<<13,
    FullScreenClaimed     = 1<<14,
    FullScreenReleased    = 1<<15,
    CarouselMove          = 1<<16,
    CarouselStop          = 1<<17,
    CarouselSelected      = 1<<18,
    GameSelectionChange   = 1<<19,
    GameSelected          = 1<<20,
    SelectorStop          = 1<<21,
    EmulatorStarting      = 1<<22,
    EmulatorFinishing     = 1<<23,
    FadeComplete          = 1<<24,
    EmulatorExit          = 1<<25,
    AutoloadComplete      = 1<<26,
    SummaryFadeComplete   = 1<<27,
    SelectorPop           = 1<<28,
} UserEvents;

#endif

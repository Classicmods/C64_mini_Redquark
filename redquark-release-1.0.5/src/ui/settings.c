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
#define _GNU_SOURCE // For syncfs
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "settings.h"
#include "firmware.h"
#include "path.h"

static settings_t settings = {0};

// -----------------------------------------------------------------------------
language_t 
settings_get_language() 
{
    return settings.language;
}

// -----------------------------------------------------------------------------
//
void
settings_set_language( language_t lang )
{
    settings.language = lang;
}

// -----------------------------------------------------------------------------
language_t 
settings_get_keyboard() 
{
    return settings.keyboard;
}

// -----------------------------------------------------------------------------
//
void
settings_set_keyboard( language_t keyboard )
{
    settings.keyboard = keyboard;
}

// -----------------------------------------------------------------------------
// 
void
settings_set_display_mode( display_mode_t mode )
{
    settings.display_mode = mode;
    //settings_save();
}

// -----------------------------------------------------------------------------
int
settings_save()
{
    int ret = -1;
    int len;

    // MOUNT RW
    //
    if( !mount_root_rw() ) return -1;

    do {
        char *path = get_profile_path( "/0" );
        if( create_direcory_tree( path, &len ) < 0 ) break; // If this happens, what to do??

        char *filename = get_profile_path( "/0/settings.dat" );

        int fd = -1;
        if( (fd = open( (char*)filename, O_RDWR|O_CREAT|O_TRUNC, 0644 ) ) < 0 ) break;

        if( write( fd, &settings, sizeof( settings_t ) ) < 0 ) {
            close(fd);
            break;
        }

        syncfs(fd);
        close(fd);

        ret = 0;

    } while(0);

    // MOUNT RO
    mount_root_ro();

    return ret;
}

// -----------------------------------------------------------------------------
int
settings_load( Flashlight *f )
{
    int ret = -1;

    // Restore settings struct
    //
    char *filename = get_profile_path( "/0/settings.dat" );

    // Set some defaults
    settings.display_mode = PIXEL_SHARP;
    settings.language     = LANG_EN;
    settings.keyboard     = LANG_EN;

    // Override with saved settings, if there are any.
    do {
        int fd;
        if( (fd = open( (char*)filename, O_RDONLY, 0644 ) ) < 0 ) break;

        if( read( fd, &settings, sizeof( settings_t ) ) < 0 ) {
            close(fd);
            break;
        }
        close(fd);

        ret = 0; // Settings were loaded

    } while(0);

    // For each item, send event
    //
    TSFEventSetting de = { {Setting, (void*)NULL}, DisplayMode,    FL_EVENT_DATA_FROM_INT(settings.display_mode) };
    flashlight_event_publish( f, (FLEvent*)&de );

    TSFEventSetting le = { {Setting, (void*)NULL}, Language,       FL_EVENT_DATA_FROM_INT(settings.language) };
    flashlight_event_publish( f, (FLEvent*)&le );

    TSFEventSetting ke = { {Setting, (void*)NULL}, KeyboardRegion, FL_EVENT_DATA_FROM_INT(settings.keyboard) };
    flashlight_event_publish( f, (FLEvent*)&ke );

    return ret;
}

// -----------------------------------------------------------------------------
// end

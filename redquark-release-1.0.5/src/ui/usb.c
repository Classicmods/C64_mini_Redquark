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
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/mount.h>

#include <unistd.h>

#include "gamelibrary.h"
#include "flashlight_mapfile.h"

#include "path.h"
#include "usb.h"

// -----------------------------------------------------------------------------
int
check_for_usb_stick()
{
    struct  stat sb;

    // Look for USB STICK device
    if( stat( USB_STICK_DEV, &sb ) < 0 ) {
//printf("USB stick not found at %s\n", USB_STICK_DEV );
        return 0;
    }

    if( !S_ISBLK(sb.st_mode) ) {
        printf("USB stick device is not a block device\n" );
        return 0;
    }

    return 1;
}

// -----------------------------------------------------------------------------
int
mount_usb_stick( int rw )
{
    if( check_for_usb_stick() == 0 ) return 0;

    unsigned long flags = rw ? MS_NOATIME : MS_NOATIME | MS_RDONLY; 

    if( mount( USB_STICK_DEV, USB_MOUNT_POINT, "vfat", flags, NULL) ) {
        printf("Failed to mount USB stick\n" );
        return 0;
    }

    return 1;
}

// -----------------------------------------------------------------------------
int
unmount_usb_stick()
{
    if( umount2( USB_MOUNT_POINT, MNT_FORCE) ) { // Can force as it's mounted RO
        return 0;
    }

    return 1;
}

// -----------------------------------------------------------------------------
//
int
mount_root_rw()
{
    // When remounting /dev/nandb is ignored
    if( mount("/dev/nandb", "/", "ext4", MS_REMOUNT, NULL ) < 0 ) {
        printf( "Failed to re-mount RW\n" );
        return 0;
    }
    return 1;
}

// -----------------------------------------------------------------------------
//
int
mount_root_ro()
{
    // When remounting /dev/nandb is ignored
    if( mount("/dev/nandb", "/", "ext4", MS_RDONLY | MS_REMOUNT, NULL ) < 0 ) {
        printf( "Failed to re-mount RO\n" );
        return 0;
    }
    return 1;
}

// -----------------------------------------------------------------------------
// Returns 1 if file exists, 0 otherwise
int
filesystem_test_for_file( char *filepath )
{
    struct  stat sb;
    if( stat( filepath, &sb ) < 0 ) {
        //printf("Failed to find file %s\n", filepath );
        return 0;
    }

    if( !S_ISREG(sb.st_mode) ) {
        printf("File %s is not a regular file\n", filepath );
        return 0;
    }

    return 1;
}

// -----------------------------------------------------------------------------

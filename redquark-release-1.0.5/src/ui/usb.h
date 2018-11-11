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
#ifndef TSF_USB_H
#define TSF_USB_H

#define UPGRADED_FIRWMARE_PATH "/bin/the64"

// usbdrive is actually symlinked to /tmp/pendrive due to /dev being on a RO filesystem
#  define USB_STICK_DEV     "/dev/usbdrive/sda1" 
#  define USB_MOUNT_POINT   "/mnt"

int check_for_usb_stick();
int mount_usb_stick( int rw );
int unmount_usb_stick();
int mount_root_ro();
int mount_root_rw();
int filesystem_test_for_file( char *filepath );

#endif

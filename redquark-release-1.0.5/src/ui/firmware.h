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
#ifndef TSF_FIRMWARE_H
#define TSF_FIRMWARE_H

#define COMPILE_VERSION(ma,mi,pa) (((ma) * 1000000) + ((mi) * 1000) + (pa))

char * check_for_upgrade( uint32_t *version );
int install_upgrade( const char *firmware );

int check_for_and_run_latest_version( int argc, char **argv );
int check_for_initial_test_mode();


#endif

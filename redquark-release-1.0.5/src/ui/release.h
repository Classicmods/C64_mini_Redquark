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
#ifndef TSF_RELEASE_H
#define TSF_RELEASE_H

#include "system_info.h"
#include "firmware.h"

#define STR(v)  #v
#define VER_STRING(M,m,p)  STR(M)"."STR(m)"."STR(p)

#ifndef BUILD_DATE
#  define BUILD_DATE __DATE__
#endif

#ifndef BUILD_COMMIT
#  define BUILD_COMMIT "unknown commit"
#endif

system_info_t SystemInfo = {
    "theC64-"VER_STRING( MAJOR, MINOR, PATCH )"-argent",
    BUILD_DATE" "__TIME__,
    BUILD_COMMIT,
    COMPILE_VERSION( MAJOR, MINOR, PATCH)
};

#endif

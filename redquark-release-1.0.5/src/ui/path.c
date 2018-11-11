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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include "path.h"

static char root_basepath   [PATH_MAX] = {0};
static char game_basepath   [PATH_MAX] = {0};
static char profile_basepath[PATH_MAX] = {0};
static char upgrade_basepath[PATH_MAX] = {0};

// ----------------------------------------------------------------------------
// XXX WARNING THIS IS NOT RE_ENTRANT XXX
char *
get_path( char *path )
{
    static int baseend = 0;

    if( root_basepath[0] == '\0' ) {

        char *the64_root = getenv("THE64");
        baseend = (the64_root != NULL) ? strlen(the64_root) : 0;
        if( baseend > 0 ) {
            if( the64_root[baseend - 1] == '/' ) baseend--;

            if( baseend > sizeof(root_basepath) / 2 ) {
                printf("THE64 env variable too long\n");
                exit(1);
            }
            strncpy( root_basepath, the64_root, baseend );
        }
    }

    int pl = strlen(path);
    strncpy( root_basepath + baseend, path, pl );
    root_basepath[baseend + pl] = '\0';

    //*len = baseend + (pl - 1);

    return root_basepath;
}

// ----------------------------------------------------------------------------
// XXX WARNING THIS IS NOT RE_ENTRANT XXX
char *
get_game_path( char *path )
{
    static int baseend = 0;

    if( game_basepath[0] == '\0' ) {

        char *the64_game = getenv("THE64GAMES");
        baseend = (the64_game != NULL) ? strlen(the64_game) : 0;
        if( baseend == 0 ) {
            the64_game = get_path( "/usr/share/the64/games" );
            baseend = strlen(the64_game);
        }

        if( the64_game[baseend - 1] == '/' ) baseend--;

        if( baseend > sizeof(game_basepath) / 2 ) {
            fprintf(stderr, "THE64GAMES / THE64 env variable too long\n");
            exit(1);
        }
        strncpy( game_basepath, the64_game, baseend );
        game_basepath[baseend] = '\0';
    }

    int pl = strlen(path);
    strncpy( game_basepath + baseend, path, pl );
    game_basepath[baseend + pl] = '\0';

    return game_basepath;
}

// ----------------------------------------------------------------------------
// XXX WARNING THIS IS NOT RE_ENTRANT XXX
char *
get_profile_path( char *path )
{
    static int baseend = 0;

    if( profile_basepath[0] == '\0' ) {

        char *the64_profile = getenv("THE64PROFILE");
        baseend = (the64_profile != NULL) ? strlen(the64_profile) : 0;
        if( baseend == 0 ) {
            the64_profile = get_path( "/var/lib/the64/profile");
            baseend = strlen(the64_profile);
        }

        if( the64_profile[baseend - 1] == '/' ) baseend--;

        if( baseend > sizeof(profile_basepath) / 2 ) {
            fprintf(stderr, "THE64PROFILE / THE64 env variable too long\n");
            exit(1);
        }
        strncpy( profile_basepath, the64_profile, baseend );
        profile_basepath[baseend] = '\0';
    }

    int pl = strlen(path);
    strncpy( profile_basepath + baseend, path, pl );
    profile_basepath[baseend + pl] = '\0';

    return profile_basepath;
}

// ----------------------------------------------------------------------------
// XXX WARNING THIS IS NOT RE_ENTRANT XXX
char *
get_upgrade_path( char *path )
{
    static int baseend = 0;

    if( upgrade_basepath[0] == '\0' ) {

        char *the64_upgrade = getenv("THE64UPGRADE");
        baseend = (the64_upgrade != NULL) ? strlen(the64_upgrade) : 0;
        if( baseend == 0 ) {
            the64_upgrade = get_path( "/var/lib/the64/upgrade");
            baseend = strlen(the64_upgrade);
        }

        if( the64_upgrade[baseend - 1] == '/' ) baseend--;

        if( baseend > sizeof(upgrade_basepath) / 2 ) {
            fprintf(stderr, "THE64UPGRADE/ THE64 env variable too long\n");
            exit(1);
        }
        strncpy( upgrade_basepath, the64_upgrade, baseend );
        upgrade_basepath[baseend] = '\0';
    }

    int pl = strlen(path);
    strncpy( upgrade_basepath + baseend, path, pl );
    upgrade_basepath[baseend + pl] = '\0';

    return upgrade_basepath;
}

// ----------------------------------------------------------------------------
//
int
create_direcory_tree( char *path, int *rlen )
{
    char   tmp[ PATH_MAX ];
    size_t len;
    char  *p; 
    struct stat ds;

    errno = 0;

    len = strlen(path);
    if (len > sizeof(tmp) - 1) return -1; 

    strcpy(tmp, path);

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if( stat(tmp, &ds) != 0) {
                // Does not exist
                if (mkdir(tmp, S_IRWXU) != 0) {
                    if (errno != EEXIST) return -1; 
                }
            } else if( ! S_ISDIR( ds.st_mode ) ) {
                // Does exist, but is not a directory
                return -1; 
            }

            *p = '/';
        }
    }   

    if( stat(tmp, &ds) != 0) {
        // Does not exist
        if (mkdir(tmp, S_IRWXU) != 0) {
            if (errno != EEXIST) return -1; 
        }
    } else if( ! S_ISDIR( ds.st_mode ) ) {
        // Does exist, but is not a directory
        return -1; 
    }

    *rlen = len;

    return 0;
}

// ----------------------------------------------------------------------------
// end

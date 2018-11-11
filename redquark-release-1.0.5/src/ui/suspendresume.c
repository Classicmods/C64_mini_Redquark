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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "flashlight.h"
#include "tsf_compress.h"

#include "gamelibrary.h"
#include "path.h"
#include "uitraps.h"

#include "firmware.h" // has root remount functions
#include "suspendresume.h"

#include "resources.h" // Vice
#include "screenshot.h" // Vice
#include "c64/c64-snapshot.h"
#include "vicii.h"

static int write_metadata( game_t *game, char *filename );
static int load_metadata( slot_meta_data_t *meta, char *filename );

extern unsigned long frame_time_ms;

// ----------------------------------------------------------------------------------
//
#define DIRLEN 64
char *
get_game_saves_dir( game_t *game )
{
    char gpath[DIRLEN] = "/0/saves/";

    int   glen;
    char *gid  = gamelibrary_get_idstring( game, &glen ); // Calculate per-game directory

    if( glen >= DIRLEN ) return NULL;

    strncpy( gpath + 9, gid, DIRLEN - 10 );

    return get_profile_path( gpath );
}

// ----------------------------------------------------------------------------------
//
int
suspend_load_slot_image( FLTexture *tx, game_t *game, int slot )
{
    int len;
    char *filename = get_game_saves_dir( game );

    if( filename == NULL )  return -1;
    
    len = strlen( filename );
    snprintf( filename + len, PATH_MAX, "/%d.png", slot );

    void *img = flashlight_mapfile( (char *)filename, &len);
    if( img == NULL ) {
        // Fallback to default
        filename = get_path("/usr/share/the64/ui/textures/suspend_slot_default.png");
        img = flashlight_mapfile( (char *)filename, &len);
        if( img == NULL ) {
            fprintf(stderr, "Error: Could not import slot image [%s]. Invalid file.\n", (char *)filename);
            return -1;
        }
    }

    const FLImageData *raw = flashlight_png_get_raw_image( img, len );

    if( raw == NULL ) {
        fprintf(stderr, "Error: Could not import slot image [%s]. Invalid file.\n", (char *)filename);
        return -1;
    }

	int x = slot ? 384 * slot: 0;

    if( raw->width == 384 && (raw->height == 272 || raw->height == 247)) { //  272 = PAL 247 = NTSC
        flashlight_texture_update( tx, x, 0, raw->width, raw->height, raw->data );
    } else {
        fprintf(stderr, "Error: Suspend slot image [%s] has dimensions %d:%d which do not match desired %d:%d\n", (char *)filename, raw->width, raw->height, 384, 272 );
    }

    flashlight_png_release_raw_image(raw);
    flashlight_unmapfile( img, len );

    return 0;
}

// ----------------------------------------------------------------------------------
//
int
suspend_load_slot_metadata( slot_meta_data_t *meta, game_t *game, int slot )
{
    int len;
    char *filename = get_game_saves_dir( game );

    memset( meta, 0, sizeof( slot_meta_data_t ) );

    if( filename == NULL )  return -1;
    
    len = strlen( filename );
    snprintf( filename + len, PATH_MAX, "/%d.mta", slot );

    if( load_metadata( meta, filename ) < 0 ) {
        //fprintf(stderr, "Error: Could not import metadata [%s].\n", (char *)filename);
        // There will be no metadata for an empty slot. Ignore error. 
        return -1;
    }

    return 0;
}

// ----------------------------------------------------------------------------------
//
static int
write_screenshot( char *filename )
{
    int r = screenshot_save("PNG", filename, vicii_get_canvas() );
    return r;
}

// ----------------------------------------------------------------------------------
//
static int
write_metadata( game_t *game, char *filename )
{
    slot_meta_data_t meta = {0};

    meta.play_duration = game->duration;

    int fd;
    if( (fd = open( (char*)filename, O_RDWR|O_CREAT|O_TRUNC, 0644 ) ) < 0 ) return -1;

    write( fd, &meta, sizeof( slot_meta_data_t ) );
    close(fd);

    return 0;
}

// ----------------------------------------------------------------------------------
//
static int
load_metadata( slot_meta_data_t *meta, char *filename )
{
    int fd;
    if( (fd = open( (char*)filename, O_RDONLY ) ) < 0 ) return -1;

    read( fd, meta, sizeof( slot_meta_data_t ) );
    close(fd);

    return 0;
}

// ----------------------------------------------------------------------------------
//
// |-- profile mgr --|---- resource mgr ----|
//
// /var/lib/profile/0/firmware/the64-date.fw
//                   /settings.dat
//                   /saves/game_id/0.png
//                                  0.vsz
//                                  0.mta
int
suspend_game( game_t *game, int slot )
{
    if( slot < 0 || slot > 3 ) return -2;

    char *saves = get_game_saves_dir( game );
    if( saves == NULL ) return -1;

    // MOUNT RW
    if( !mount_root_rw() ) return -1;
  
    int ret = -1;
    do {
        // Make sure directory exists
        int len;
        if( create_direcory_tree( saves, &len ) < 0 ) break;

        len += snprintf( saves + len, PATH_MAX, "/%d.", slot );

        // Write metadate
        //       - execution time
        memcpy( saves + len, "mta\0", 4 );
        write_metadata( game, saves );

        // Write screenshot
        //
        memcpy( saves + len, "png\0", 4 );
        write_screenshot( saves );

        // Write Snapshot to temporary file
        //
        char * game_filename = "/tmp/save.vsf";
        save_snapshot( (unsigned char*)game_filename );

        // Now compress to target destination
        memcpy( saves + len, "vsz\0", 4 );
        tsf_compress_file( game_filename, saves );

        remove( game_filename ); // Clean up temporary file 

        sync();

        ret = 0;
    } while(0);

    // MOUNT RO
    mount_root_ro();

    return ret;
}

// ----------------------------------------------------------------------------------
int
resume_game( game_t *game, int slot )
{
    slot_meta_data_t meta = {0};

    if( slot < 0 || slot > 3 ) return -2;

    char *saves = get_game_saves_dir( game );
    if( saves == NULL ) return -1;

    int ret = -1;
    do {
        int len;
        if( create_direcory_tree( saves, &len ) < 0 ) break;

        len += snprintf( saves + len, PATH_MAX, "/%d.", slot );

        memcpy( saves + len, "vsz\0", 4 );

        // Decompress to temporary file
        char * game_filename = "/tmp/save.vsf";

        if( tsf_decompress_file( saves, game_filename ) == 0 ) {

            // Load temporary file
            resources_set_int("RAMInitStartValue", 0 );
            load_snapshot( (unsigned char*)game_filename );

            ret = 0;
        }

        // Clean up tmp file
        remove( game_filename );
        sync();

        if( ret == 0 ) {
            if( suspend_load_slot_metadata( &meta, game, slot ) == 0 ) {
                game->duration = meta.play_duration;
            }
        }

    } while(0);

    return ret;
}

// ----------------------------------------------------------------------------------
// end

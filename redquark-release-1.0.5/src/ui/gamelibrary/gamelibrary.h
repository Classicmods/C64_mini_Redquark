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
#ifndef GAMESMETA_H
#define GAMESMETA_H

#include "flashlight.h"
#include "joystick.h"
#include "settings.h"

#define GAME_META_TITLE_MAX       100
#define GAME_META_FILE_MAX        256
#define GAME_META_DESCRIPTION_MAX 1024
#define GAME_META_AUTHOR_MAX      100
#define GAME_META_PUBLISHER_MAX   100
#define GAME_META_MUSIC_MAX       100
#define GAME_META_GENRE_MAX       20 
#define GAME_META_YEAR_MAX        4

typedef enum {
    SID_6581  = 0,
    SID_8580  = 1,
    SID_8580D = 2,
} sid_type_t;

#define MAX_SCREEN_IMAGES 2
#define MAX_AUTHOR        2

typedef struct {
    unsigned char       title      [GAME_META_TITLE_MAX       + 1];
             char       filename   [GAME_META_FILE_MAX        + 1]; // room for path too
    unsigned char       description[LANG_MAX][GAME_META_DESCRIPTION_MAX + 1];
    unsigned char       author     [MAX_AUTHOR][GAME_META_AUTHOR_MAX      + 1];
    unsigned char       publisher  [GAME_META_PUBLISHER_MAX   + 1];
    unsigned char       music      [GAME_META_MUSIC_MAX       + 1];
    unsigned char       genre      [GAME_META_GENRE_MAX       + 1];
    unsigned char       year       [GAME_META_YEAR_MAX        + 1];
             char       cover_img  [GAME_META_FILE_MAX        + 1];
             char       screen_img [MAX_SCREEN_IMAGES][GAME_META_FILE_MAX        + 1];

    int                 author_count;
    int                 screen_image_count;
    int                 cartridge_title_id;
    int                 needs_truedrive;
    int                 needs_driveicon;
    int                 vertical_shift; // Screen has top/bottom boarders chopped. This allows adjust.
    int                 is_ntsc;
    int                 is_64;  
    int                 is_basic;  
    sid_type_t          sid_model;

    joystick_map_t      port_map   [JOY_MAX_PORT][JE_MAX_MAP];
    int                 port_primary;

    unsigned int        duration;

} game_t;

typedef struct {
    game_t       *games;
    unsigned int *by_author;
    unsigned int *by_music;
    unsigned int *by_year;
    unsigned int *by_title;

    int len;
    int max;

} gamelibrary_t;

gamelibrary_t * gamelibrary_init( int s );
void gamelibrary_import_games( gamelibrary_t *lib, const char *dirname );
char * gamelibrary_get_idstring( game_t *g, int *len );

#endif

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

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "flashlight.h"

#include "mainmenu.h"
#include "toolbar.h"
#include "user_events.h"
#include "path.h"

#define LIBRARY_SIZE 100

static void gamelist_event( void *self, FLEvent *event );

// ----------------------------------------------------------------------------------
//
static int
png_load( mali_texture *mt, void *b, int x, int y, void *d, int l, void *private ) 
{
    static FLImageData target_img = {0};
    target_img.width  = mali_egl_image_get_iwidth( mt );
    target_img.height = mali_egl_image_get_iheight( mt );
    target_img.depth  = 32;
    target_img.gl_color_format = FL_GL_BGRA; // So the PNG load transforms from RGBA to BGRA
    target_img.size   = target_img.width * target_img.height * target_img.depth;
    target_img.data   = b;

    return flashlight_png_insert_image( &target_img, d, l, x, y );
}

// ----------------------------------------------------------------------------------
//
void
load_card_image( Card *c, const char *filename )
{
    int len = 0;

    void *img = flashlight_mapfile( (char *)filename, &len);
    if( img == NULL ) {
        printf("Error: Could not load image file [%s]. Permissions or missing.\n", (char *)filename);
        return;
    }

    do {
        int ret;
        ret = mali_egl_image_sub_direct( c->maliFLTexture, c->tex_x, 0, img, len, png_load, NULL );

        if( ret < 1 ) {
            fprintf(stderr, "Error: Could not import carousel image [%s]\n", filename);
            break;
        }
    } while(0);

    flashlight_unmapfile( img, len );
}

// ----------------------------------------------------------------------------------
//
static game_t *
get_game( GameList *gl, int index )
{
    gamelibrary_t *lib = gl->library;

    if( index >= lib->len ) return NULL;

    int ix = 0;
    switch( gl->sort_order )
    {
        case BY_TITLE:  ix = lib->by_title [ index ]; break;
        case BY_AUTHOR: ix = lib->by_author[ index ]; break;
        case BY_MUSIC:  ix = lib->by_music [ index ]; break;
        case BY_YEAR:   ix = lib->by_year  [ index ]; break;
    }

    return lib->games + ix;
}

// ----------------------------------------------------------------------------------
//
static void
load_carousel_card( Card *card, int card_number, void *private )
{
    //printf("Load card [%d] w: %d  h: %d  s: %d\n", card_number, card->width, card->height, card->tex_x );

#ifdef COVER_TEST
    char *f = "";
    switch(card_number){
        case 0: f = "assets/covers/boulder_dash.png"; break;
        case 1: f = "assets/covers/bounder.png"; break;
        case 2: f = "assets/covers/epyx_winter_games.png"; break;
        case 3: f = "assets/covers/international_karate.png"; break;
        case 4: f = "assets/covers/monty_on_the_run.png"; break;
        case 5: f = "assets/covers/nodes_of_yesod.png"; break;
        case 6: f = "assets/covers/paradroid.png"; break;
        case 7: f = "assets/covers/robin_of_the_wood.png"; break;
        case 8: f = "assets/covers/spindizzy.png"; break;
        case 9: f = "assets/covers/uridium.png"; break;
        case 10: f = "assets/covers/laser_squad.png"; break;
        default: break;
    }
    load_card_image( card, f );
    return;
#endif

    GameList *gl = (GameList *)private;

    game_t *g = get_game( gl, card_number );
    if( g == NULL ) return; // Eek! This should never happen

    load_card_image( card, g->cover_img );
}

// ----------------------------------------------------------------------------------
// Initialise in-game shot display
GameList *
gamelist_init( Flashlight *f )
{
    GameList *gl = malloc( sizeof(GameList) );
    if( gl == NULL ) {
        return NULL;
    }
    memset( gl, 0, sizeof(GameList) );

    gl->f = f;
    flashlight_event_subscribe( f, (void*)gl, gamelist_event, FLEventMaskAll );

    gamelibrary_t *lib = gamelibrary_init( LIBRARY_SIZE ); // 100 slots -= may only need 64
    if( lib == NULL ) {
        return NULL;
    }
    gl->library = lib;

    gamelibrary_import_games( lib, get_game_path( "" ));
    gl->sort_order = BY_TITLE;

#ifdef DEBUG_GAMELIST
    int i;
    for(i = 0; i < lib->len; i++ ) {
        game_t *ng = lib->games + i;

        printf( "Title: %s\n", ng->title );
        printf( "File : %s\n", ng->filename );
        printf( "Desc : %s\n", ng->description[LANG_DE-1] );
        printf( "Cover: %s\n", ng->cover_img );
        printf( "Screen: %s\n", ng->screen_img );
        printf( "Genre: %s\n", ng->genre );
        printf( "Year : %s\n", ng->year );
        printf("Map: ");
        //int j;
        //for(j = 0; j < JE_MAX_MAP; j++ ) {
        //    printf( " %d ", ng->port_map[1][j].value );
        //}
        //printf("\n");
    }
#endif

    // Initialise the carousel AFTER we have set up the game library
    gl->carousel = carousel_init( f, load_carousel_card, lib->len, gl );

    return gl;
}

// ----------------------------------------------------------------------------------
//
int
xgamelist_has_focus( GameList *gl )
{
    return gl->carousel->focus;
}

// ----------------------------------------------------------------------------------
//
void
gamelist_finish( GameList **gl ) {
    free(*gl);
    *gl = NULL;
}

// ----------------------------------------------------------------------------------
//int
//gamelist_process( GameList *gl )
//{
//    return 1;
//}

// ----------------------------------------------------------------------------------
static void
gamelist_event( void *self, FLEvent *event )
{
    GameList *gl = (GameList *)self;

    switch( event->type ) {
        //case Process:
        //    {
        //        gamelist_process( gl );
        //    }
        //    break;
       case UserEvent:
            {
                TSFUserEvent *ue = (TSFUserEvent*)event;
                switch(ue->user_type) {
                    case CarouselStop:
                        {
                            game_t *g = get_game( gl, FL_EVENT_DATA_TO_INT(ue->data) );
                            if( g == NULL ) break; // Should never happen

                            TSFUserEvent e = { {UserEvent, (void*)gl}, GameSelectionChange, (void*)g };
                            flashlight_event_publish( gl->f,(FLEvent*)&e );
                        }
                        break;

                    case CarouselSelected:
                        {
                            CarouselSelection * select = (CarouselSelection*)(ue->data);
                            game_t *g = get_game( gl, select->card_index );
                            if( g == NULL ) break; // Should never happen

                            // Publish event referencing the game that is under the selector
                            gl->selected_game.game = g;
                            gl->selected_game.primary_controller_id = select->controller_id;

                            // Publish event referencing the game that has been selected
                            TSFUserEvent e = { {UserEvent, (void*)gl}, GameSelected, (void*)&(gl->selected_game) };
                            flashlight_event_publish( gl->f,(FLEvent*)&e );
                        }
                        break;
                    //case FullScreenClaimed:
                    //    display_enabled = 0;
                    //    break;
                    //case FullScreenReleased:
                    //    display_enabled = 1;
                    //    break;
                }
            }
            break;
       case FocusChange:
            {
                TSFFocusChange *fce = (TSFFocusChange*)event;
                if( fce->who == gl ) {
                    // We have the focus - Pass it onto the carousel
                    TSFFocusChange e = { {FocusChange, (void*)gl}, gl->carousel };
                    flashlight_event_publish( gl->f, (FLEvent*)&e );

                    gl->focus = 1;
                }
                else
                {
                    gl->focus = 0;
                }
            }
        default:
            break;
    }
}

// ----------------------------------------------------------------------------------

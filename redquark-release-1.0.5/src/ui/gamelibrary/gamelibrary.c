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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include "gamelibrary.h"
#include "flashlight_mapfile.h"

#define PATH_BUF_SIZE 1024

#define MOVE_TO_LINE_END(p) \
        while( *(p) != '\n' && *(p) > '\0' ) ++(p); \
        p = *(p) ? (p) + 1 : (p);

#define F1  20
#define F2  21
#define F3  22
#define F4  23
#define F5  24
#define F6  25
#define F7  26
#define F8  27
#define AL  10
#define AU  11
#define CM  29
#define CO  44
#define CT  15
#define CD  7
#define CL  8
#define CR  9
#define CU  6
#define DL  14
#define EN  13
#define HM  5
#define RS  1
#define RE  16
#define SL  2   // Shift Left
#define SR  3   // Shift Right
#define SS  4   // Shift Lock
#define SP  32
#define PO  168 // Pound

// Map chars in controller configuration to their equivalent evdev position code
static int ascii_to_evdev_map[] = {0,1,42,54,58,102,103,108,105,106,41,155,0,28,14,29,15,0,0,0,59,60,61,62,63,64,65,66,0,127,0,0,57,0,0,0,0,0,0,0,0,0,55,78,121,74,83,98,11,2,3,4,5,6,7,8,9,10,157,158,0,117,0,0,154,156,48,46,32,18,33,34,35,23,36,37,38,160,49,24,25,151,19,31,20,22,47,152,45,153,159,170,0,171,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,150,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


static game_t *gamelibrary_add( gamelibrary_t *l, game_t *g );


// ----------------------------------------------------------------------------
// s = Initial length of list
gamelibrary_t *
gamelibrary_init( int s ) 
{
    gamelibrary_t *l;

    l = malloc(sizeof(gamelibrary_t));
    if( l == NULL ) return l;

    l->games = malloc( s * sizeof(game_t) ); if (l->games == NULL) { 
        fprintf( stderr, "Failed to allocate space for game library\n");
        free(l);
        return NULL;
    }

    l->by_author    = malloc( s * sizeof(unsigned int) ); if (l->by_author   == NULL) { }
    l->by_music     = malloc( s * sizeof(unsigned int) ); if (l->by_music    == NULL) { }
    l->by_year      = malloc( s * sizeof(unsigned int) ); if (l->by_year     == NULL) { }
    l->by_title     = malloc( s * sizeof(unsigned int) ); if (l->by_title    == NULL) { }

    l->len = 0;
    l->max = s;

    return l;
}

// ----------------------------------------------------------------------------
// Returns pointer to game in library
static game_t *
gamelibrary_add( gamelibrary_t *l, game_t *g )
{
    int i,x,r;

    if( l->len >= l->max ) return NULL;

    memcpy( l->games + l->len, g, sizeof(game_t) );

    // Sort by title
    x = 0;
    for( i = 0; i < l->len; i++ ) {
        x = l->by_title[i];
        r = strcasecmp( g->title, l->games[x].title );
        if( r < 0 ) break; // Position found
    }
    if( i < l->len ) memmove( l->by_title + i + 1, l->by_title + i, sizeof(unsigned int) * (l->len - i) );
    l->by_title[i] = l->len;

    // Sort by Musician
    x = 0;
    for( i = 0; i < l->len; i++ ) {
        x = l->by_music   [i];
        r = strcasecmp( g->music   , l->games[x].music    );
        if( r < 0 ) break; // Position found
    }
    if( i < l->len ) memmove( l->by_music    + i + 1, l->by_music    + i, sizeof(unsigned int) * (l->len - i) );
    l->by_music   [i] = l->len;

    // Sort by author
    x = 0;
    for( i = 0; i < l->len; i++ ) {
        x = l->by_title[i];
        r = strcasecmp( g->author[0], l->games[x].author[0] );
        if( r < 0 ) break; // Position found
    }
    if( i < l->len ) memmove( l->by_author + i + 1, l->by_author + i, sizeof(unsigned int) * (l->len - i) );
    l->by_author[i] = l->len;

    // Sort by year
    x = 0;
    for( i = 0; i < l->len; i++ ) {
        x = l->by_year[i];
        r = strcasecmp( g->year, l->games[x].year );
        if( r < 0 ) break; // Position found
    }
    if( i < l->len ) memmove( l->by_year + i + 1, l->by_year + i, sizeof(unsigned int) * (l->len - i) );
    l->by_year[i] = l->len;

    l->len++;

    return 0;
}

// ----------------------------------------------------------------------------
//
int
copy_string( unsigned char * dst, const unsigned char *src, int len )
{
    int i;
    for( i = 0; (i < len - 1) && (src[i] != '\n' && src[i] > 0); i++ ) dst[i] = src[i];
    dst[i] = '\0';
    return i;
}

// ----------------------------------------------------------------------------
//
static char *
process_cart_number( char *p, game_t *g )
{
    int f;
    char *ep;
    f = strtol( p, &ep, 10 );
    if( ep == p || f < 1 || f > 10) {
        printf("Invalid definition %2.2s (Cartridge title number %d)\n", p, f);
        // Error converting cartridge title
        MOVE_TO_LINE_END( p );
        return p;
    }

    g->cartridge_title_id = f;
    return ep;
}

// ----------------------------------------------------------------------------
//
static char *
process_vertical_shift( char *p, game_t *g )
{
    int f;
    char *ep;
    f = strtol( p, &ep, 10 );
    if( ep == p || f < -16 || f > 16) {
        printf("Invalid definition %2.2s (FLVertical shift %d)\n", p, f);
        MOVE_TO_LINE_END( p );
        return p;
    }

    g->vertical_shift = f;
    return ep;
}

// ----------------------------------------------------------------------------
//
static char *
process_title( char *p, game_t *g )
{
    int l = copy_string( g->title, p, sizeof(g->title) );
    return p + l;
}

// ----------------------------------------------------------------------------
//
static char *
process_publisher( char *p, game_t *g )
{
    int l = copy_string( g->publisher, p, sizeof(g->publisher) );
    return p + l;
}

// ----------------------------------------------------------------------------
//
static char *
process_author( char *p, game_t *g )
{
    if( g->author_count == MAX_AUTHOR ) {
        MOVE_TO_LINE_END(p);
        return p;
    }
    int l = copy_string( g->author[ g->author_count ], p, sizeof(g->author[ g->author_count ]) );

    g->author_count++;
    return p + l;
}

// ----------------------------------------------------------------------------
//
static char *
process_music( char *p, game_t *g )
{
    int l = copy_string( g->music, p, sizeof(g->music) );
    return p + l;
}

// ----------------------------------------------------------------------------
//
static char *
process_genre( char *p, game_t *g )
{
    int l;
    if( strncmp(      p, "shoot",    (l = 5)) == 0 ) copy_string( g->genre, "GR_SHOOT", sizeof(g->genre) );
    else if( strncmp( p, "puzzle",   (l = 6)) == 0 ) copy_string( g->genre, "GR_PUZZLE", sizeof(g->genre) );
    else if( strncmp( p, "sport",    (l = 5)) == 0 ) copy_string( g->genre, "GR_SPORT", sizeof(g->genre) );
    else if( strncmp( p, "adventure",(l = 9)) == 0 ) copy_string( g->genre, "GR_ADVENTURE", sizeof(g->genre) );
    else if( strncmp( p, "platform", (l = 8)) == 0 ) copy_string( g->genre, "GR_PLATFORM", sizeof(g->genre) );
    else if( strncmp( p, "maze",     (l = 4)) == 0 ) copy_string( g->genre, "GR_MAZE", sizeof(g->genre) );
    else if( strncmp( p, "driving",  (l = 7)) == 0 ) copy_string( g->genre, "GR_DRIVING", sizeof(g->genre) );
    else if( strncmp( p, "programming",(l =11)) == 0 ) copy_string( g->genre, "GR_PROGRAMMING", sizeof(g->genre) );
    else l = copy_string( g->genre, p, sizeof(g->genre) );
    return p + l;
}

// ----------------------------------------------------------------------------
//
static char *
process_file( char *p, const char *path, int path_base_len, game_t *g )
{
    int l = 0;
    if( p[0] > ' ' && p[0] != '/' && (p[0] != '.' || (p[0] == '.' && p[1] == '.')) ) l = copy_string( g->filename, path, path_base_len );
    l = copy_string( g->filename + l, p, sizeof(g->filename) - l );
    return p + l;
}
            
// ----------------------------------------------------------------------------
//
static char *
process_description( char *p, game_t *g )
{
    language_t lang = LANG_NONE;
    int l;

    // First check the language
    if( p[2] != ':' ) {
        printf("Invalid language identifier %3.3s\n", p);
        MOVE_TO_LINE_END( p );
        return p;
    }
    if( p[0] == 'e' ) {
        if( p[1] == 'n' )  lang = LANG_EN;
        if( p[1] == 's' )  lang = LANG_ES;
    }
    if( p[0] == 'd' && p[1] == 'e' ) lang = LANG_DE;
    if( p[0] == 'f' && p[1] == 'r' ) lang = LANG_FR;
    if( p[0] == 'i' && p[1] == 't' ) lang = LANG_IT;

    if( lang == LANG_NONE ) {
        printf("Invalid language identifier %3.3s\n", p);
        MOVE_TO_LINE_END( p );
        return p;
    }
    p += 3;

    // Make sure language index is from 0
    int lang_ix = lang - LANG_MIN;
    l = copy_string( g->description[lang_ix], p, sizeof(g->description[lang_ix]) );
    return p + l;
}

// ----------------------------------------------------------------------------
//
static char *
process_year( char *p, game_t *g )
{
    int l = copy_string( g->year, p, sizeof(g->year) );
    return p + l;
}

// ----------------------------------------------------------------------------
//
static char *
process_cover( char *p, const char *path, int path_base_len, game_t *g )
{
    int l = 0;
    if( p[0] != '/' && (p[0] != '.' || (p[0] == '.' && p[1] == '.')) ) l = copy_string( g->cover_img, path, path_base_len );
    l = copy_string( g->cover_img + l, p, sizeof(g->cover_img) - l );
    return p + l;
}

// ----------------------------------------------------------------------------
//
static char *
process_screen( char *p, const char *path, int path_base_len, game_t *g )
{
    if( g->screen_image_count == MAX_SCREEN_IMAGES ) {
        MOVE_TO_LINE_END(p);
        return p;
    }

    int l = 0;
    if( p[0] != '/' && (p[0] != '.' || (p[0] == '.' && p[1] == '.')) ) l = copy_string( g->screen_img[g->screen_image_count], path, path_base_len );
    l = copy_string( g->screen_img[g->screen_image_count] + l, p, sizeof(g->screen_img[g->screen_image_count]) - l );

    g->screen_image_count++;

    return p + l;
}

// ----------------------------------------------------------------------------
// JU JD JL JR JF          = Joystick directions
// F1 F2 F3 F4 F5 F6 F7 F8 = Function keys  (map index 20..27)
// AL  = Arrow left         map index 10
// AU  = Arrow Up           map index 11
// CM  = Commodore Key      map index 29
// CO  = Comma              map index ,
// CT  = Control            map index 15
// CU  = Cursor Up          map index 6
// CD  = Cursor Down        map index 7
// CL  = Cursor Left        map index 8
// CR  = Cursor Right       map index 9
// DL  = Delete             map index 14
// EN  = Return             map index 13
// HM  = Home               map index 5
// RS  = Run/Stop           map index 1
// RE  = Restore            map index 16
// SL  = Shift Left         map index 2
// SR  = Shift Right        map index 3
// SS  = Shift lock         map index 4
// SP  = Space              map index 32
// PO  = Space              map index 168
// Symbols                  map index asci
// Letters                  map index asci
// Numbers                  map index asci
//
static joystick_map_t *
convert_control( char *ptr, int *len )
{
    char *p = ptr;
    static joystick_map_t map = {0};
    int *lu = ascii_to_evdev_map; // Lookup table

    // Count chars
    while( *ptr > ' ' && *ptr != ',' ) ptr++;
    *len = ptr - p;

    //printf("Processing control item %*.*s\n", ptr - p, ptr - p, p );
    
    if( ptr - p == 2 ) {
        map.type = JoyKeyboard; // The default
        // Special alias
        switch( p[0] ) {
            case 'A':
                if( p[1] == 'L' ) { map.value = lu[AL]; break; } // Arrow left
                if( p[1] == 'U' ) { map.value = lu[AU]; break; } // Arrow up
                printf("Invalid definition %2.2s\n", p);
                return NULL;
                break;

            case 'C' :
                if( p[1] == 'M' ) { map.value = lu[CM]; break; } // Commodore Key
                if( p[1] == 'O' ) { map.value = lu[CO]; break; } // Comma
                if( p[1] == 'T' ) { map.value = lu[CT]; break; } // Control
                if( p[1] == 'D' ) { map.value = lu[CD]; break; } // Cursor D
                if( p[1] == 'L' ) { map.value = lu[CL]; break; } // Cursor L
                if( p[1] == 'R' ) { map.value = lu[CR]; break; } // Cursor R
                if( p[1] == 'U' ) { map.value = lu[CU]; break; } // Cursor U
                printf("Invalid definition %2.2s\n", p);
                return NULL;
                break;

            case 'D':
                if( p[1] == 'L' ) { map.value = lu[DL]; break; } // Delete
                printf("Invalid definition %2.2s\n", p);
                return NULL;
                break;

            case 'E':
                if( p[1] == 'N' ) { map.value = lu[EN]; break; } // Enter / Return
                printf("Invalid definition %2.2s\n", p);
                return NULL;
                break;

            case 'F': // Function Keys
                {
                    int f;
                    char *ep;
                    f = strtol( p + 1, &ep, 10 );
                    if( ep == p || f < 1 || f > 8) {
                        printf("Invalid definition %2.2s (function value %d)\n", p, f);
                        // Error converting function key
                        return NULL;
                    }
                    map.value = lu[ F1 + (f - 1) ];
                }
                break;

            case 'H':
                if( p[1] == 'M' ) { map.value = HM; break; } // Home
                printf("Invalid definition %2.2s\n", p);
                return NULL;
                break;

            case 'J' :
                map.type = JoyPort;
                if( p[1] == 'U' ) { map.value = JoyPortUp;    break; } // Joystick Up
                if( p[1] == 'D' ) { map.value = JoyPortDown;  break; } // Joystick Down
                if( p[1] == 'L' ) { map.value = JoyPortLeft;  break; } // Joystick Left
                if( p[1] == 'R' ) { map.value = JoyPortRight; break; } // Joystick Right
                if( p[1] == 'F' ) { map.value = JoyPortFire;  break; } // Joystick Fire
                printf("Invalid definition %2.2s\n", p);
                return NULL;
                break;

            case 'P' :
                if( p[1] == 'O' ) { map.value = lu[PO]; break; } // Pound
                printf("Invalid definition %2.2s\n", p);
                return NULL;
                break;

            case 'R' :
                if( p[1] == 'E' ) { map.value = lu[RE]; break; } // Restore
                if( p[1] == 'S' ) { map.value = lu[RS]; break; } // Run/Stop
                printf("Invalid definition %2.2s\n", p);
                return NULL;
                break;

            case 'S' :
                if( p[1] == 'L' ) { map.value = lu[SL]; break; } // Shift (Left)
                if( p[1] == 'P' ) { map.value = lu[SP]; break; } // Space
                if( p[1] == 'R' ) { map.value = lu[SR]; break; } // Shift (Right)
                if( p[1] == 'S' ) { map.value = lu[SS]; break; } // Shift lock
                printf("Invalid definition %2.2s\n", p);
                return NULL;
                break;

            default:
                printf("Invalid definition %2.2s\n", p);
                return NULL;
        }
        //printf("Mapping %2.2s to %d\n", p, map.value );
    } else if( ptr - p == 1 ) {
        // Letters, digits and symbols
        map.type  = JoyKeyboard;
        map.value = lu[ p[0] ];
        if( map.value == 0 ) {
            printf("Invalid definition %c\n", p[0]);
            return NULL;
        }
    } else if( ptr - p == 0 ) {
        // Empty
        map.value = 0; // FIXME Is this okay?
    } else {
        printf("Invalid alias (too long) '%*.*s'\n", *len, *len, p);
        return NULL;
    }

    return &map;
}

// ----------------------------------------------------------------------------
//
// J:id*:a,b,c,d,e,f...
static char *
process_controls( char *ptr, game_t *g )
{
    int i = 0;
    int port_id;

    while( *ptr == ' ' ) ptr++;

    // First is the port number
    char *ep;
    port_id = strtol( ptr, &ep, 10 );

    if( ep == ptr ) {
        // Error - Invalid portid
        printf("Invalid joystick port map %4.4s...\n", ptr );
        MOVE_TO_LINE_END(ptr);
        return ptr;
    }

    if( port_id < 1 || port_id > JOY_MAX_PORT ) {
        printf( "Port id %d is not in the rage 1..%d\n", port_id, JOY_MAX_PORT );
        MOVE_TO_LINE_END(ptr);
        return ptr;
    }

    port_id--;

    // Clear the mapping early
    memset( g->port_map[ port_id ], sizeof(g->port_map[ port_id ] ), 0 );

    while( *ptr == ' ' ) ptr++; // Eat whitespace

    ptr = ep;
    if( *ptr == '*' ) {
        // This is the primary port
        g->port_primary = port_id + 1;
        ptr++;
    }

    while( *ptr == ' ' ) ptr++; // Eat whitespace
    if( *ptr != ':' ) {
        printf( "Missing expected :\n" );
        MOVE_TO_LINE_END(ptr);
        return ptr;
    }
    ptr++;

    int len = 0;
    i = 0;
    while( i < JE_MAX_MAP ) {
        while( *ptr == ' ' ) ptr++; // Eat whitespace

        joystick_map_t *mapped = convert_control( ptr, &len );
        if( mapped == NULL ) {
            printf("Error processing map\n" );
            MOVE_TO_LINE_END(ptr);
            return ptr;
        }

        g->port_map[ port_id ][ i ].type  = mapped->type;
        g->port_map[ port_id ][ i ].value = mapped->value;

        //printf( "Mapped to %d\n", mapped->value );

        ptr += len;
        while( *ptr == ' ' ) ptr++; // Eat whitespace

        if( i < (JE_MAX_MAP - 1) ) {
            if( *ptr != ',' ) { // All but last item should be followed by a comma
                printf("Missing expected ,\n" );
                MOVE_TO_LINE_END(ptr);
                return ptr;
            }
        } else {
            if( *ptr == ',' ) { // There should not be a comma after the last item
                printf("Last item followed by an unexpected ,\n" );
                MOVE_TO_LINE_END(ptr);
                return ptr;
            }
        }


        if( *ptr == ',' ) ptr++;

        i++;
    }
    return ptr;
}

// ----------------------------------------------------------------------------
//
static char *
process_machine( char *ptr, game_t *g )
{
    int len;
    char *p;
    // Machine_id, flag, flag, fkag

    ptr--;
    do {
        ptr++;
        while( *ptr == ' ' ) ptr++;

        // Find end of word
        p = ptr;
        while( *ptr > ' ' && *ptr != ',' ) ptr++;
        len = ptr - p;

        if( len == 0 ) break;

        if( len == 2 && strncmp( p, "64",        2) == 0 ) g->is_64           = 1;
        if( len == 4 && strncmp( p, "ntsc",      4) == 0 ) g->is_ntsc         = 1;
        if( len == 9 && strncmp( p, "truedrive", 9) == 0 ) g->needs_truedrive = 1;
        if( len == 9 && strncmp( p, "driveicon", 9) == 0 ) g->needs_driveicon = 1;
        if( len == 5 && strncmp( p, "basic",     5) == 0 ) g->is_basic        = 1;

        while( *ptr == ' ' ) ptr++;

    } while( *ptr == ',' );

    return ptr;
}


// ----------------------------------------------------------------------------
//
static char *
process_meta_line( char *ptr, const char *path, int path_base_len, game_t *g )
{
    char c;

    //printf( "Process meta line [%10s]\n", ptr );

    // Strip leading spaces
    while( *ptr == ' ' ) ptr++;
    
    // Is this an empty line or a comment?
    if( *ptr == '#' ) while( *ptr >= ' ' ) ptr++;
    if( *ptr == '\n' && *ptr > '\0' ) return ++ptr;
    
    // is the second character a colon?
    if( ptr[1] != ':' ) {
        fprintf( stderr,  "Invalid line found in game metafile: %s\n", ptr );
        return NULL;
    }
    
    c = *ptr;
    ptr += 2;

    // Skip space after the :
    while( *ptr == ' ' ) ptr++;

    switch( c ) {
        case 'T': // title
            ptr = process_title( ptr, g );
            break;
        case 'F': // file
            ptr = process_file( ptr, path, path_base_len, g );
            break;
        case 'D': // Description
            ptr = process_description( ptr, g );
            break;
        case 'C': // Cover image
            ptr = process_cover( ptr, path, path_base_len, g );
            break;
        case 'G': // Ingame screen shot
            ptr = process_screen( ptr, path, path_base_len, g );
            break;
        case 'P': // Publisher
            ptr = process_publisher( ptr, g );
            break;
        case 'A': // Author
            ptr = process_author( ptr, g );
            break;
        case 'M': // Music author
            ptr = process_music( ptr, g );
            break;
        case 'Y': // Year of release
            ptr = process_year( ptr, g );
            break;
        case 'E': // Genre
            ptr = process_genre( ptr, g );
            break;
        case 'J': // Control / Key mapping
            ptr = process_controls( ptr, g );
            break;
        case 'I': // Cartridge game title number
            ptr = process_cart_number( ptr, g );
            break;
        case 'X': // Machine configuration
            ptr = process_machine( ptr, g );
            break;
        case 'V': // FLVertical shift in screen. -16 <= v shift <= 16
            ptr = process_vertical_shift( ptr, g );
            break;
        default: 
            //printd( "Skipping unknown config entry %c\n", c );
            break;
    }

    while( *ptr >= ' ' ) ptr++; // Eat up to the linefeed

    while( *ptr < ' ' && *ptr > 0 ) ptr++; // Eat any line endings

    return ptr;
}

// ----------------------------------------------------------------------------
//
static game_t *
gamelibrary_load_meta( const char *filename, int path_base_len, game_t *g )
{

    char *m = NULL;
    char *p = 0;
    char *e = 0;
    int len;

    g->port_primary = 2;

    m = (char *)flashlight_mapfile( filename, &len );
    if( m == NULL ) return NULL;

    p = m;
    e = m + len;
    while( p < e ) {
        p = process_meta_line( p, filename, path_base_len, g );
    }

    flashlight_unmapfile( m, len );

    return g;
}

// ----------------------------------------------------------------------------
void
gamelibrary_import_games( gamelibrary_t *lib, const char *dirname )
{
    DIR *dir;
    struct dirent *entry;
    char path[PATH_BUF_SIZE];
    int pl;

    pl = copy_string( path, dirname, sizeof(path) );
    if( pl == 0 ) return;

    if( path[pl-1] != '/' ) path[pl++] = '/';

    if (!(dir = opendir(dirname))) return;
    if (!(entry = readdir(dir)))   return;


    do {
        if( lib->len >= lib->max ) break;

        if (entry->d_type == DT_DIR) {
        }
        else
        {
            //printf( "Process %s\n", entry->d_name );
            char *f = entry->d_name;
            int l = strlen( f );
            if( l > 4 && f[l-4] == '.' && f[l-3] == 't' && f[l-2] == 's' && f[l-1] == 'g' ) {

                strncpy( path + pl, f, PATH_BUF_SIZE - pl - 1 );

                game_t ng = {0}; // = lib->games + lib->len++;
                ng.sid_model = SID_6581; // Set a default;

                gamelibrary_load_meta( path , pl + 1, &ng );

                gamelibrary_add( lib, &ng );
            }

        }
    } while ( (entry = readdir(dir)));
    closedir(dir);
}


// ----------------------------------------------------------------------------
//
char *
gamelibrary_get_idstring( game_t *g, int *len )
{
    static char id[100] = {0};

    unsigned char *p = g->filename;
    if( p == NULL || *p == '\0' ) p = g->title; // Fallback to title if there is no filename

    unsigned char *s = p;
    p += strlen(p);
    while( *p != '/' && p > s ) p--; // Scan backwards to find last path sep
    if( *p == '/' ) p++;

    // Convert 
    int i = 0;
    while( *p != '.' && *p != '\0' && i < (sizeof(id) - 1) ) id[i++] = *p++;
    id[i] = '\0';

    if( g->cartridge_title_id > 0 ) {
        id[i++] = '-';
        sprintf( id + i, "%d", g->cartridge_title_id );
    }

    *len = i;

    return id;
}

// ----------------------------------------------------------------------------

// end

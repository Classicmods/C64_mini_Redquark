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
#include "flashlight_mapfile.h"

#include "path.h"
#include "settings.h"

#define MAX_MESSAGES 100

typedef struct {
    char id  [32];
    int  idlen;
    char text[128];
} locale_message_t;

static locale_message_t messages[ LANG_MAX + 1 ][ MAX_MESSAGES ] = {0};
static int              messages_count[ LANG_MAX + 1] = {0};

static language_t _language = LANG_EN;

// ----------------------------------------------------------------------------
//
void
locale_init()
{
    memset( messages, 0, sizeof(messages) );
    memset( messages_count, 0, sizeof(messages_count) );
}

// ----------------------------------------------------------------------------
//
static locale_message_t *
find_slot( language_t lang, char *id, int id_len, int create_if_missing )
{
    int i;

    locale_message_t *msg = NULL;

    if( id_len >= sizeof( msg->id ) ) {
        fprintf( stderr, "ID too long [%*.*s]\n", id_len, id_len, id );
        return NULL;
    }

    msg = messages[ lang ];
    int msg_count = messages_count[ lang ];

//printf( "find_slot: language: %d\n", lang );
//printf( "find_slot: message count: %d\n", msg_count );

    for( i = 0; i < msg_count; msg++, i++ ) {
//printf( "find_slot: matching %s against %*.*s\n", msg->id, id_len, id_len, id );
        if( msg->idlen == id_len && strncmp( msg->id, id, id_len ) == 0 ) return messages[lang] + i;
    }

    if( i >= MAX_MESSAGES ) return NULL;

    // Not found, so create a new slot
    if( create_if_missing ) {

        msg = messages[lang] + i;

        strncpy( msg->id, id, id_len);
        msg->id[id_len] = '\0';
        msg->idlen = id_len;

        msg_count++;
        messages_count[ lang ] = msg_count;

//printf("Create new slot for %s   (lang %d)\n", msg->id, lang);
    } else {
        msg = NULL; // Not found
    }

    return msg;
}

// ----------------------------------------------------------------------------
//
int
locale_store_text( language_t lang, char *id, int id_len, char *text, int text_len )
{
    locale_message_t *msg = NULL;

//printf("Store in lang %d - Label: '%*.*s' Text: '%*.*s'\n", lang, id_len, id_len, id, text_len, text_len, text );

    // Check the length of the text first, to avoid writing an ID entry when there is an error
    if( text_len >= sizeof( msg->text) ) {
        fprintf( stderr, "Message text too long [%*.*s]\n", text_len, text_len, text );
        return 0;
    }

    // Find/create slot
    msg = find_slot( lang, id, id_len, 1 );
    if( msg == NULL ) return 0;
    strncpy( msg->text, text, text_len );

    return 1;
}

// ----------------------------------------------------------------------------
language_t
locale_get_language( )
{
    return _language;
}

// ----------------------------------------------------------------------------
int
locale_select_language( language_t lang )
{
    if( lang < LANG_MIN || lang > LANG_MAX ) return 0;

    _language = lang;
    return 1;
}

// ----------------------------------------------------------------------------
// 
char *
locale_get_text( char* id )
{
    int id_len = strlen(id);
    locale_message_t *msg = find_slot( _language, id, id_len, 0 );
    if( !msg ) return "";
    
    char * t = msg->text;
//printf ( "Found %s = %s\n", id, t);
    return t;
}

// ----------------------------------------------------------------------------
//
static char *
process_message_line( language_t lang, char *ptr )
{
    //printf( "Process locale chunk [%10s]\n", ptr );

    // Strip leading spaces
    while( *ptr == ' ' ) ptr++;
    
    // Is this an empty line or a comment?
    if( *ptr == '#' ) while( *ptr >= ' ' ) ptr++;
    if( *ptr == '\n' ) return ++ptr;

    // Find the colon
    char *p = ptr;
    while( *ptr != ':' && *ptr >= ' ' ) ptr++;
    if( *ptr < ' ' ) {
        fprintf( stderr, "Invalid message line %s\n", p );
        return NULL;
    }

    int id_len = ptr - p;
    char *tp = ptr + 1;

    // Find text end
    while( *ptr != '\n' && *ptr != '\0' ) ptr++;

    if( !locale_store_text( lang, p, id_len, tp, ptr - tp ) ) {
        printf("Error storing text for %*.*s\n", id_len, id_len, p );
    }

    while( *ptr < ' ' && *ptr > 0 ) ptr++; // Eat any line endings

    return ptr;
}

// ----------------------------------------------------------------------------
//
int
locale_load_messages( language_t lang, const char *filename )
{
    char *m = NULL;
    char *p = 0;
    char *e = 0;
    int len;

    char fname[128];

    sprintf( fname, "/usr/share/the64/ui/data/%s", filename );
    char *f = get_path(fname);

    m = (char *)flashlight_mapfile( f, &len );
    if( m == NULL ) return 0;

    p = m;
    e = m + len;
    while( p < e ) {
        p = process_message_line( lang, p );
    }

    flashlight_unmapfile( m, len );

    return 1;
}

// ----------------------------------------------------------------------------


#ifdef TEST
void
main(int argc, char **argv )
{
    int r;

    locale_init();

    r = locale_load_messages( LANG_EN, "en.msg" );
    if( r == 0 ) printf( "English lang failed\n" );
    printf("Message count after English addition: %d\n", msg_count );

    r = locale_load_messages( LANG_FR, "fr.msg" );
    if( r == 0 ) printf( "French lang failed\n" );
    printf("Message count after French addition: %d\n", msg_count );

    printf("\n\n");

    char * hello = locale_get_text( "HELLO" );
    char * anoth = locale_get_text( "BROKEN" );

    if(hello == NULL ) printf("Error: HELLO not found\n");
    else printf("Initial hello: [%s]\n", hello );

    if(anoth == NULL ) printf("Error: BROKEN not found\n");
    else printf("Initial anoth: [%s]\n", anoth );

    anoth = locale_get_text( "ANOTHER" );
    printf("Selecting English\n" );
    r = locale_select_language( LANG_EN );
    if( r == 0 ) printf( "English lang failed\n" );

    printf("hello: [%s]\n", hello );
    printf("anoth: [%s]\n", anoth );

    printf("Selecting French\n" );
    r = locale_select_language( LANG_FR );
    if( r == 0 ) printf( "French lang failed\n" );

    printf("hello: [%s]\n", hello );
    printf("anoth: [%s]\n", anoth );

}
#endif

// end

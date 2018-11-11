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

#include "textpane.h"

#include "display_menu.h"
#include "tsf_joystick.h"
#include "ui_sound.h"
#include "user_events.h"
#include "gridmenu.h"
#include "path.h"
#include "settings.h"
#include "ip_notice.h"
#include "path.h"
#include "locale.h"

#define WIDTH 1280
#define HEIGHT 720

#define FY 91
#define HW (WIDTH/2)
#define HH (HEIGHT/2)

#define MW  WIDTH
#define MH  HEIGHT
#define MHH (MH/2)
#define MHW (MW/2)

typedef void (*menuSelectCallback)     ( menu_t *m, void *private, int id );
static void _select_page(menu_t *m, void *private, int id );
#define SLOT_TOP          630
#define SLOT_BOTTOM        90
#define SLIDE_POS_TOP    (SLOT_TOP - 25)
#define SLIDE_POS_BOTTOM (SLOT_BOTTOM + 25)

static FLVertice scrollbar_verticies[] = {
    { 1187 + 0, SLOT_TOP,    3, 10},
    { 1187 + 0, SLOT_BOTTOM, 3, 11},
    { 1187 + 6, SLOT_TOP,    9, 10},
    { 1187 + 6, SLOT_BOTTOM, 9, 11},

    { 1187 -  7, SLIDE_POS_TOP + 20, 507,  0},
    { 1187 -  7, SLIDE_POS_TOP - 21, 507, 41},
    { 1187 + 13, SLIDE_POS_TOP + 20, 527,  0},
    { 1187 + 13, SLIDE_POS_TOP - 21, 527, 41},
};


static fs_item_t menu_item[] = {
    {   0, 0, 160, 18, 500, 690, MARKER_LEFT, "IP_NOTICES", _select_page },
    {   0, 0, 160, 18, 750, 690, MARKER_LEFT, "IP_LEGAL",   _select_page },
};

static void _display_text( IPNotice *ipn );
static void ip_notice_event( void *self, FLEvent *event );
static void clean_up( IPNotice *ipn );

// ----------------------------------------------------------------------------------
//
static void
set_scrollbar_position( IPNotice *ipn ) 
{
    float p;

    p = ((float)ipn->next_page_offset[ ipn->current_doc ] / ipn->text_len[ ipn->current_doc ] ) * 100 ;

    if( ipn->next_page_offset[ ipn->current_doc ] >= ipn->text_len[ ipn->current_doc ] ) p = 100.0;
    if( ipn->current_page_number[ ipn->current_doc ] <= 0 ) p = 0.0;

    int y = (((float)SLIDE_POS_TOP - SLIDE_POS_BOTTOM) / 100) * p;

    ipn->scroll_position = y;
}

// ----------------------------------------------------------------------------------
//
static void
add_scrollbar_quads( IPNotice *ipn )
{
    FLVertice vi[4];
    // Build triangles
    int i,j;
    for(i = 0; i < sizeof(scrollbar_verticies)/sizeof(FLVertice); i += 4 ) {
        for(j = 0; j < 4; j++ ) {

            int p = i < 4 ? 0 : ipn->scroll_position;

            vi[j].x = X_TO_GL(WIDTH,           scrollbar_verticies[i + j].x);
            vi[j].y = Y_TO_GL(HEIGHT,          scrollbar_verticies[i + j].y - p );
            vi[j].s = S_TO_GL(ipn->tx->width,  scrollbar_verticies[i + j].s);
            vi[j].t = T_TO_GL(ipn->tx->height, scrollbar_verticies[i + j].t);
            vi[j].u = 0.2;
        }
        flashlight_vertice_add_quad2D( ipn->fs_menu->back_vertice_group, &vi[0], &vi[1], &vi[2], &vi[3] );
    }

    return;
}

// ----------------------------------------------------------------------------------
//
static void
init_menu_cb( menu_t *m, void *private, int id, int *enabled, menu_item_dim_t *dim, menuSelectCallback *ms_cb  )
{
    FullScreenMenu *fs_menu = (FullScreenMenu *)private;
    IPNotice     *ipn   = (IPNotice *)(fs_menu->owner_private);

    *enabled = 1;

    int x = menu_item[id].ox;
    int y = menu_item[id].oy;
    int w = menu_item[id].w;
    int h = menu_item[id].h;
    
    if( ipn->text[ id ] == NULL ) {
        textpane_t *tp = textpane_init( fs_menu->back_vertice_group, fs_menu->fonts->fontatlas, fs_menu->fonts->body, w, h, WIDTH, HEIGHT, TEXT_LEFT );

        ipn->text[ id ] = tp;
    }

    textpane_t *tp = ipn->text[ id ];
    unsigned char *txt = locale_get_text( menu_item[ id ].text );
    tp->largest_width = 0;
    build_text( tp, txt, 1, x , y - 20 );

    dim->w = tp->largest_width + 1; // Adjust width to match text
    dim->h = h;

    dim->x = x + dim->w / 2;
    dim->y = y - dim->h / 2 - 4;

    dim->selected_w = dim->w;
    dim->selected_h = dim->h;

    dim->selector_w = dim->w + 2;
    dim->selector_h = dim->h + 2;

    dim->marker_pos = menu_item[id].marker_pos;

    *ms_cb = menu_item[ id ].cb; // The select callback
}

// ----------------------------------------------------------------------------------
// Initialise in-game shot display
IPNotice *
ip_notice_init( Flashlight *f, fonts_t *fonts )
{
    IPNotice *ipn = malloc( sizeof(IPNotice) );
    if( ipn == NULL ) {
        return NULL;
    }
    memset( ipn, 0, sizeof(IPNotice) );

    ipn->f = f;
    ipn->fonts = fonts;
    ipn->fs_menu = fsmenu_menu_init( f, fonts, init_menu_cb, (void *)ipn, "IP_TITLE", 2, 1, HEIGHT, HEIGHT/2, FS_MENU_FLAG_POINTER | FS_MENU_FLAG_NO_CLAIM_SCREEN | FS_MENU_FLAG_STACKED | FS_MENU_FULLSCREEN, OPAQUE );
    ipn->fs_menu->name = "IPNotice";

    textpane_t *tp = textpane_init( ipn->fs_menu->back_vertice_group, ipn->fs_menu->fonts->fontatlas, ipn->fs_menu->fonts->body, 1000, 560, WIDTH, HEIGHT, TEXT_LEFT );

    tp->flags = TP_FLAG_DOCUMENT; // We've got a document, so only process a display height worth of lines.

    // Make sure the screen asset atlas texture is loaded
    ipn->tx = flashlight_load_texture( f, get_path("/usr/share/the64/ui/textures/menu_atlas.png") );
    if( ipn->tx == NULL ) { printf("Error tx\n"); exit(1); }

    flashlight_register_texture    ( ipn->tx, GL_NEAREST );
    flashlight_shader_attach_teture( f, ipn->tx, "u_DecorationUnit" );

    ipn->text[ 2 ] = tp;
    ipn->current_doc = 0;

    flashlight_event_subscribe( f, (void*)ipn, ip_notice_event, FLEventMaskAll );

    return ipn;
}

// ----------------------------------------------------------------------------------

void
ip_notice_finish( IPNotice **m ) {
    free(*m);
    *m = NULL;
}

// ----------------------------------------------------------------------------------
//
static void
remove_page_text_verticies( IPNotice *ipn )
{
    if( ipn->base_vert_count == 0 ) return;

    int count = ipn->fs_menu->back_vertice_group->index_len - ipn->base_vert_count;
    if( count > 0 ) flashlight_vertice_remove_quad2D( ipn->fs_menu->back_vertice_group, count );
}

// ----------------------------------------------------------------------------------
//
static void
_display_text( IPNotice *ipn )
{
    int rendered;
    
    remove_page_text_verticies( ipn );

    rendered = build_text( ipn->text[2], ipn->text_doc[ ipn->current_doc ] + ipn->current_page_offset[ ipn->current_doc ], 1, 90 , 615 );

    ipn->next_page_offset[ ipn->current_doc ] = ipn->current_page_offset[ ipn->current_doc ] + rendered;
}

// ----------------------------------------------------------------------------------
//
static void
page_down( IPNotice *ipn )
{
    if( ipn->next_page_offset[ ipn->current_doc ] >= ipn->text_len[ ipn->current_doc ] ) return;
    if( ipn->current_page_number[ ipn->current_doc ] >= IP_MAX_PAGES ) return;

    ipn->page_offsets[ ipn->current_doc ][ ipn->current_page_number[ ipn->current_doc ]++ ] = ipn->current_page_offset[ ipn->current_doc ];

    ui_sound_play_flip();

    ipn->current_page_offset[ ipn->current_doc ]  = ipn->next_page_offset[ ipn->current_doc ];

    _display_text( ipn );

    set_scrollbar_position( ipn );
    add_scrollbar_quads( ipn );
}

// ----------------------------------------------------------------------------------
// 
static void
page_up( IPNotice *ipn )
{
    if( ipn->current_page_number[ ipn->current_doc ] <= 0 ) return;

    ui_sound_play_flip();

    ipn->current_page_offset[ ipn->current_doc ] = ipn->page_offsets[ ipn->current_doc ][ --(ipn->current_page_number[ ipn->current_doc ]) ];


    _display_text( ipn );
    set_scrollbar_position( ipn );
    add_scrollbar_quads( ipn );
}

// ----------------------------------------------------------------------------------

static void
ip_notice_event( void *self, FLEvent *event )
{
    IPNotice *ipn = (IPNotice *)self;

    if( event->type == Setting ) {
        TSFEventSetting *es = (TSFEventSetting*)event;
        switch( es->id ) {
            // If there is a language change, then reset base_vert_count
            // so that it gets re-calculated correctly for the new language.
            case Language: ipn->base_vert_count = 0; break;
            default: break;
        }
    }

    if( !ipn->active ) return;

    switch( event->type ) {
        case Navigate:
            {
                int direction = ((TSFEventNavigate*)event)->direction;
                //if( !(direction & (INPUT_UP | INPUT_DOWN)) ) page_stop  ( c );
                if(   direction & INPUT_DOWN  ) page_down( ipn );
                if(   direction & INPUT_UP    ) page_up  ( ipn );
                break;
                break;
            }
            break;
        case UserEvent:
            {
                TSFUserEvent *ue = (TSFUserEvent*)event;
                switch(ue->user_type) {
                    case MenuClosed:
                        if( ue->data == ipn->fs_menu ) {
                            clean_up( ipn );
                            ipn->active = 0;
                        }
                        break;

                    default:
                        break;
                }
            }
            break;
        default:
            break;
    }
}

// ----------------------------------------------------------------------------------
//
static int
load_text( IPNotice *ipn, int id )
{
    char *filename;

    if( id > 1 ) return 0;
    
    if( id == 0 ) filename = get_path( "/usr/share/the64/ui/data/ipnotice.dat" );
    else          filename = get_path( "/usr/share/the64/ui/data/license.dat" );

    int len = 0;
    void *text = flashlight_mapfile( (char *)filename, &len);
    if( text == NULL ) {
        fprintf(stderr, "Error: Could not load gameshot image file [%s]. Permissions or missing?\n", filename);
        return 0;
    }

    ipn->text_doc[id] = text;
    ipn->text_len[id] = len;

    return 1;
}

// ----------------------------------------------------------------------------------
//
static void
release_text( IPNotice *ipn )
{
    flashlight_unmapfile( ipn->text_doc[0], ipn->text_len[0] );
    ipn->text_doc[0] = NULL;
    ipn->text_len[0] = 0;

    flashlight_unmapfile( ipn->text_doc[1], ipn->text_len[1] );
    ipn->text_doc[1] = NULL;
    ipn->text_len[1] = 0;
}

// ----------------------------------------------------------------------------------
//
static void
clean_up( IPNotice *ipn )
{
    remove_page_text_verticies( ipn ); // Clean up
    release_text( ipn );
}

// ----------------------------------------------------------------------------------
//
static void
_select_page(menu_t *m, void *private, int id )
{
    FullScreenMenu *fs_menu = (FullScreenMenu *)private;
    IPNotice        *ipn     = (IPNotice *)(fs_menu->owner_private);

    ui_sound_play_ding();

    if( ipn->current_doc != id ) {
        ipn->current_doc = id;
        menu_set_marked_item( fs_menu->menu, id );

        _display_text( ipn ); 
        set_scrollbar_position( ipn );
        add_scrollbar_quads( ipn );
    }
}

// ----------------------------------------------------------------------------------
void
ip_notice_enable( IPNotice *ipn )
{
    if( ipn->base_vert_count == 0 ) {
        // Remember the offset into the vertix group that page text will start from
        // so we can delete and reload as pages change.
        ipn->base_vert_count = ipn->fs_menu->back_vertice_group->index_len;
    }

    ipn->active = 1;

    load_text( ipn, 0 );
    load_text( ipn, 1 );
    fsmenu_menu_enable( ipn->fs_menu, 1 ); // 1 = hires selector

    set_scrollbar_position( ipn );

    _display_text( ipn ); 
    add_scrollbar_quads( ipn );
}

// ----------------------------------------------------------------------------------
void
ip_notice_disable( IPNotice *ipn )
{
    fsmenu_menu_disable( ipn->fs_menu );
}

// ----------------------------------------------------------------------------------
// end

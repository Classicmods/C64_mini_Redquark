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

#include "flashlight_screen.h"
#include "flashlight_matrix.h"
#include "flashlight_texture.h"
#include "flashlight_shader.h"

#include "textpane.h"

#include "ig_menu.h"
#include "tsf_joystick.h"
#include "ui_sound.h"
#include "mainmenu.h"
#include "toolbar.h"
#include "path.h"
#include "user_events.h"
#include "display_menu.h"
#include "locale_menu.h"

#define WIDTH 1280
#define HEIGHT 720

static void toolbar_menu_event( void *self, FLEvent *event );

static ToolBar *tbar = NULL;

#define SE 21

#define ICON_CENT_X (WIDTH / 2)
#define ICON_WIDTH  54
#define ICON_HEIGHT 54
#define ICON_SEP    20

#define ICON_START_X(c) (ICON_CENT_X - (ICON_WIDTH/2) * ((c) % 2) + ((ICON_SEP)/2) * (((c)+1) % 2) - ((ICON_SEP + ICON_WIDTH) * ((c) / 2)))

#define ICON_X(c,i)     (ICON_START_X(c) + (ICON_WIDTH + ICON_SEP) * (i))
#define ICON_XC(c,i)    (ICON_X(c,i) + (ICON_WIDTH / 2))
#define ICON_YC(i)      ((HEIGHT / 2) - SE)

static int atlas_width;
static int atlas_height;

#define ICON_START_IX_IN_GROUP 1

// Position menu about the origin
static FLVertice menu_verticies[] = {
    { -1,0, 1, 60 },
    { -1,0, 1, 59 },
    { -1,0, 2, 60 },
    { -1,0, 2, 59 },
//    { 640 - 350, 360 - 275,     283, 118 },  // URidium Ship
//    { 640 - 350, 360 + 275,     283, 96},
//    { 640 + 350, 360 - 275,     311, 118 },
//    { 640 + 350, 360 + 275,     311, 96},
};

struct tile_s {
    int   s;
    int   t;
    int   w;
    int   h;
    int   ox;
    int   oy;
    char *text;
    menuSelectCallback cb;  // Declared in gridmenu.h
};

typedef void (*menuSelectCallback)     ( menu_t *m, void *private, int id );

static void _select_display (menu_t *m, void *private, int id );
static void _select_settings(menu_t *m, void *private, int id );
static void _select_language(menu_t *m, void *private, int id );

static struct tile_s menu_item[] = {
    { 283, 4, 54, 54, 283,  4, "Display options",    _select_display  },
    { 220,60, 54, 54, 209, 60, "Region",             _select_language },
    { 220, 4, 54, 54, 220,  4, "Settings",           _select_settings },
    //{ 345, 4, 54, 54, 345,  4, "IP notices",         _select_ipnotice },
};

// ----------------------------------------------------------------------------------
//
static void
add_image_quads( Flashlight *f, ToolBar *tbar )
{
    FLVerticeGroup *vg = flashlight_vertice_create_group(f, 8);
	tbar->vertice_group = vg;

    // Make sure the screen asset atlas texture is loaded
    FLTexture *tx = flashlight_load_texture( f, get_path("/usr/share/the64/ui/textures/menu_atlas.png") );
    if( tx == NULL ) { printf("Error tx\n"); exit(1); }

    atlas_width = tx->width;
    atlas_height = tx->height;

    flashlight_register_texture    ( tx, GL_NEAREST );
    flashlight_shader_attach_teture( f, tx, "u_DecorationUnit" );

    FLVertice vi[4];
    vi[0].u = vi[1].u = vi[2].u = vi[3].u = 0.2; // Select the menu atlas texture

    int i,j;
    for(i = 0; i < sizeof(menu_verticies)/sizeof(FLVertice); i += 4 ) {
        for(j = 0; j < 4; j++ ) {

            vi[j].x = X_TO_GL(WIDTH,      menu_verticies[i + j].x);
            vi[j].y = Y_TO_GL(HEIGHT,     menu_verticies[i + j].y);
            vi[j].s = S_TO_GL(tx->width,  menu_verticies[i + j].s);
            vi[j].t = T_TO_GL(tx->height, menu_verticies[i + j].t);
        }
        flashlight_vertice_add_quad2D( vg, &vi[0], &vi[1], &vi[2], &vi[3] );
    }

    return;
}

// ----------------------------------------------------------------------------------
static void
add_icon( FLVerticeGroup *vg, int id, menu_item_dim_t *d )
{
    struct tile_s *ic = &(menu_item[id]);

    int x0 = d->x - (d->w / 2);
    int y0 = d->y - (d->h / 2);
    
    int x1 = x0 + d->w;
    int y1 = y0 + d->h;

    int s0 = ic->s;
    int t0 = ic->t;
    int s1 = ic->s + ic->w;
    int t1 = ic->t + ic->h;

    int c[] = {x0,y0,s0,t1, x0,y1,s0,t0, x1,y0,s1,t1, x1,y1,s1,t0};

    FLVertice vi[4];
    int j;

    for(j = 0; j < 4; j++ ) {
        vi[j].x = X_TO_GL(WIDTH,        c[ j * 4 + 0 ]);
        vi[j].y = Y_TO_GL(HEIGHT,       c[ j * 4 + 1 ]);
        vi[j].s = S_TO_GL(atlas_width,  c[ j * 4 + 2 ]);
        vi[j].t = T_TO_GL(atlas_height, c[ j * 4 + 3 ]);
        vi[j].u = 0.2;
    }
    flashlight_vertice_add_quad2D( vg, &vi[0], &vi[1], &vi[2], &vi[3] );
}

// ----------------------------------------------------------------------------------
// Innitialise each menu item
static void
init_menu_cb( menu_t *m, void *private, int id, int *enabled, menu_item_dim_t *dim, menuSelectCallback *ms_cb  )
{
    ToolBar *tbar = (ToolBar *)private;

    *enabled = 1;

    dim->w = 54;
    dim->h = 54;

    //dim->x = ICON_XC( m->enabled_count, id );
    dim->x = ICON_XC( 3, id );
    dim->y = ICON_YC(id);

    add_icon( tbar->vertice_group, id, dim );

    dim->selected_w = dim->w + 12;
    dim->selected_h = dim->h + 12;

    dim->selector_w = dim->w + 12;
    dim->selector_h = dim->h + 12;

    *ms_cb = menu_item[ id ].cb; // The select callback
}

// ----------------------------------------------------------------------------------
// This will expand/shrink menu items as they are selected / desselected
static void 
menu_item_set_position_cb( menu_t *m, void *private, int id, int x, int y, int w, int h )
{
    ToolBar *tbar = (ToolBar *)private;
    
    FLVertice *v[4];
    flashlight_vertice_get_quad2D( tbar->vertice_group, id + ICON_START_IX_IN_GROUP, &v[0], &v[1], &v[2], &v[3] );

    v[0]->x = v[1]->x = X_TO_GL(WIDTH, x - w / 2);
    v[2]->x = v[3]->x = X_TO_GL(WIDTH, x + w / 2);

    v[0]->y = v[2]->y = Y_TO_GL(HEIGHT, y - h / 2);
    v[1]->y = v[3]->y = Y_TO_GL(HEIGHT, y + h / 2);

    if( m->current == id || m->focus == 0 ) { // This is the icon being selected, not deselected. Re-map background highloght.

        if( m->focus == 0 ) x = -200; // Actually, this is the "active" icon, but the focus has moved away. Move background!

        flashlight_vertice_get_quad2D( tbar->vertice_group, 0, &v[0], &v[1], &v[2], &v[3] );

        v[0]->x = v[1]->x = X_TO_GL(WIDTH, x - w / 2 - 6);
        v[2]->x = v[3]->x = X_TO_GL(WIDTH, x + w / 2 + 6);

        v[0]->y = v[2]->y = Y_TO_GL(HEIGHT, y - h / 2 - 6);
        v[1]->y = v[3]->y = Y_TO_GL(HEIGHT, y + h / 2 + 6);
    }
}

// ----------------------------------------------------------------------------------
// Initialise in-game shot display
ToolBar *
toolbar_init( Flashlight *f, fonts_t *fonts )
{
    if( tbar != NULL ) return tbar; // There can only be one ig_menu

    tbar = malloc( sizeof(ToolBar) );
    if( tbar == NULL ) {
        return NULL;
    }
    memset( tbar, 0, sizeof(ToolBar) );

    tbar->f = f;
    add_image_quads(f, tbar);

    tbar->uniforms.texture_fade       = flashlight_shader_assign_uniform( f, "u_fade"               );
    tbar->uniforms.translation_matrix = flashlight_shader_assign_uniform( f, "u_translation_matrix" );
    tbar->active = 1;

    // Initialise the menu backend, which in-turn will call the local callback 'init_menu_cb()' to
    // initialise each menu item.
    //
    tbar->menu = menu_init( WIDTH, HEIGHT, 3, 1, f, init_menu_cb, menu_item_set_position_cb, NULL, MENU_FLAG_NONE, (void *)tbar );

    menu_set_enabled_position ( tbar->menu, WIDTH / 2, 12 + SE + ICON_HEIGHT     );
    menu_set_disabled_position( tbar->menu, WIDTH / 2,      SE + ICON_HEIGHT - 4 );

    menu_set_selector_move_rate( tbar->menu, 5 );
    menu_disable( tbar->menu, 1 ); // This will establish the menu's starting position.

    flashlight_event_subscribe( f, (void*)tbar, toolbar_menu_event, FLEventMaskAll );

    tbar->dmenu    = display_menu_init ( f, fonts );
    tbar->lmenu    = locale_menu_init  ( f, fonts );
    tbar->smenu    = settings_menu_init( f, fonts );

    return tbar;
}

// ----------------------------------------------------------------------------------

void
toolbar_menu_finish( ToolBar **m ) {
    free(*m);
    *m = tbar = NULL;
}

// ----------------------------------------------------------------------------------

static void
toolbar_menu_event( void *self, FLEvent *event )
{
    ToolBar *tbar = (ToolBar *)self;

    switch( event->type ) {
        case Redraw:
            {
                if( tbar->active || tbar->menu->moving ) {
                    glUniformMatrix4fv( tbar->uniforms.translation_matrix,1, GL_FALSE, menu_get_translation( tbar->menu ) );
                    glUniform1f( tbar->uniforms.texture_fade, 1.0 );

                    FLVerticeGroup *vg = tbar->vertice_group;
                    flashlight_shader_blend_on(tbar->f);
                    glDrawElements(GL_TRIANGLES, vg->indicies_len, GL_UNSIGNED_SHORT, flashlight_get_indicies(vg) );
                }
            }
            break;
        case FocusChange:
            {
                TSFFocusChange *fce = (TSFFocusChange*)event;
                if( fce->who == tbar ) {
                    if( tbar->focus == 0 ) {
                        // We have the focus
                        tbar->focus = 1;
                        menu_enable( tbar->menu, 5 );
                        mainmenu_selector_set_lowres();
                    }
                } else {
                    if( tbar->focus == 1 ) {
                        tbar->focus = 0;

                        menu_disable( tbar->menu, 6 );
                    }
                }
            }
            break;
        case UserEvent: 
            {
                TSFUserEvent *ue = (TSFUserEvent*)event;
                switch(ue->user_type) {
                    case FullScreenClaimed:
                        tbar->active = 0;
                        break;
                    case FullScreenReleased:
                        tbar->active = 1;
                        break;
                    case OpenLanguageSelection:
                        locale_menu_enable( tbar->lmenu );
                        break;
                    default:
                        break;
                }
            }
            break;

        case Setting:
            {
                TSFEventSetting *es = (TSFEventSetting*)event;
                switch( es->id ) {
                    case DisplayMode: 
                        if( event->self == tbar->dmenu ) break; // Don't get caught in a loop
                        display_menu_set_selection( tbar->dmenu, FL_EVENT_DATA_TO_TYPE(display_mode_t, es->data) );
                        break;

                    case Language: 
                        if( event->self == tbar->lmenu ) break; // Don't get caught in a loop
                        locale_menu_set_selection( tbar->lmenu, FL_EVENT_DATA_TO_TYPE(language_t, es->data) );
                        break;

                    default: break;
                }
            }
            break;
        default:
            break;
    }
}

// ----------------------------------------------------------------------------------
static void
_select_display(menu_t *m, void *private, int id )
{
    ToolBar *tbar = (ToolBar *)private;

    if( !tbar->active || tbar->menu->moving ) return;

    display_menu_enable( tbar->dmenu );
}

static void
_select_language(menu_t *m, void *private, int id )
{
    ToolBar *tbar = (ToolBar *)private;
    if( !tbar->active || tbar->menu->moving ) return;
    locale_menu_enable( tbar->lmenu );
}

static void
_select_settings(menu_t *m, void *private, int id )
{
    ToolBar *tbar = (ToolBar *)private;

    if( !tbar->active || tbar->menu->moving ) return;
    //if( mainmenu_selector_moving() ) return;

    settings_menu_enable( tbar->smenu );
}

// ----------------------------------------------------------------------------------
// end

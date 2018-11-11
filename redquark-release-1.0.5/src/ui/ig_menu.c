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

#include "sem.h"
#include "ig_menu.h"
#include "tsf_joystick.h"
#include "ui_sound.h"
#include "user_events.h"
#include "mainmenu.h"
#include "gridmenu.h"
#include "suspend_menu.h"
#include "selector.h"
#include "path.h"
#include "settings.h"
#include "locale.h"
#include "ui_joystick.h"
#include "uitraps.h"

#define WIDTH 1280
#define HEIGHT 720

static void ig_menu_event( void *self, FLEvent *event );

static InGameMenu *igm = NULL;

#define FY 91
#define HW (WIDTH/2)
#define HH (HEIGHT/2)

#define MW  WIDTH
#define MH  92
#define MHH (MH/2)
#define MHW (MW/2)
#define MFY (MH - 12)

static int atlas_width;
static int atlas_height;

// Position menu about the origin
static FLVertice menu_verticies[] = {
    { HW - MHW,   (HH - MHH),       40.1, 60},
    { HW - MHW,   (HH - MHH) + MFY, 40.1, 32},
    { HW + MHW-1, (HH - MHH),       40.9, 60},
    { HW + MHW-1, (HH - MHH) + MFY, 40.9, 32},

    { HW - MHW,   (HH - MHH) + MFY, 40.1, 32},
    { HW - MHW,   (HH - MHH) + MH,  40.1,  0},
    { HW + MHW-1, (HH - MHH) + MFY, 40.9, 32},
    { HW + MHW-1, (HH - MHH) + MH,  40.9,  0},
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

static void _select_saverestore(menu_t *m, void *private, int id );
static void _select_keyboard(menu_t *m, void *private, int id );
static void _select_exit(menu_t *m, void *private, int id );

#define MENU_ITEMS 3
static struct tile_s menu_item[] = {
    { 433, 0, 24, 22, 12, 11, "IG_LOADSAVE", _select_saverestore },
    { 408, 0, 24, 22, 12, 11, "IG_KEYBOARD", _select_keyboard    },
    { 456, 0, 24, 22, 12, 11, "IG_EXIT"    , _select_exit        },
};

// ----------------------------------------------------------------------------------
//
static void
add_image_quads( Flashlight *f, InGameMenu *igm )
{
    FLVertice vi[4];

    FLVerticeGroup *vg = flashlight_vertice_create_group(f, 8);
	igm->back_vertice_group = vg;

    // Make sure the screen asset atlas texture is loaded
    FLTexture *tx = flashlight_load_texture( f, get_path("/usr/share/the64/ui/textures/menu_atlas.png") );
    if( tx == NULL ) { printf("Error tx\n"); exit(1); }

    atlas_width = tx->width;
    atlas_height = tx->height;

    flashlight_register_texture    ( tx, GL_NEAREST );
    flashlight_shader_attach_teture( f, tx, "u_DecorationUnit" );

    // Build triangles
    int i,j;
    for(i = 0; i < sizeof(menu_verticies)/sizeof(FLVertice); i += 4 ) {
        for(j = 0; j < 4; j++ ) {

            vi[j].x = X_TO_GL(WIDTH,      menu_verticies[i + j].x);
            vi[j].y = Y_TO_GL(HEIGHT,     menu_verticies[i + j].y);
            vi[j].s = S_TO_GL(tx->width,  menu_verticies[i + j].s);
            vi[j].t = T_TO_GL(tx->height, menu_verticies[i + j].t);
            vi[j].u = 0.2;
        }
        flashlight_vertice_add_quad2D( vg, &vi[0], &vi[1], &vi[2], &vi[3] );
    }

    igm->base_vert_count = vg->index_len;

    return;
}

// ----------------------------------------------------------------------------------
static void
add_icon( FLVerticeGroup *vg, int id, int rhs_x, int center_y )
{
    struct tile_s *ic = &(menu_item[id]);

    int x0 = rhs_x    - ic->w;
    int y0 = center_y - ic->oy;
    int x1 = x0 + ic->w;
    int y1 = y0 + ic->h;

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
//
static void
init_menu_cb( menu_t *m, void *private, int id, int *enabled, menu_item_dim_t *dim, menuSelectCallback *ms_cb  )
{
    InGameMenu *igm = (InGameMenu *)private;

    if( id >= MENU_ITEMS ) return;

    *enabled = 1;

    dim->w = 400;
    dim->h = 20;

    if( igm->text[ id ] == NULL ) {
        igm->text[ id ] = textpane_init( igm->back_vertice_group, igm->fonts->fontatlas, igm->fonts->body, dim->w, dim->h + 10, WIDTH, HEIGHT, TEXT_CENTER );
    }

    textpane_t *tp = igm->text[ id ];

    int ox = WIDTH / (MENU_ITEMS + 1);
    int sx = ox - dim->w / 2;

    int tcx = sx + ox * id + (24/2+2); // Last part is half icon width

    unsigned char *txt = locale_get_text( menu_item[ id ].text );
    tp->largest_width = 0;
    build_text( tp, txt, 1,  tcx, HH - 10 );

    tcx = (tp->x + tp->width / 2);
    
    add_icon( igm->back_vertice_group, id, tcx - tp->largest_width / 2 - 10, tp->y + 6);

    dim->x = tcx - 18;
    dim->y = igm->text[ id ]->y + 6;

    dim->selected_w = dim->w;
    dim->selected_h = dim->h;

    dim->selector_w = tp->largest_width + 16 + 18;
    dim->selector_h = dim->h + 0;

    *ms_cb = menu_item[ id ].cb; // The select callback
}

// ----------------------------------------------------------------------------------
static void 
menu_item_set_position_cb( menu_t *m, void *private, int id, int x, int y, int w, int h )
{
    //printf( "menu set item %d position to %d, %d : %d, %d\n", id, x, y, w, h );
}

// ----------------------------------------------------------------------------------
//
static void
set_language_cb( menu_t *m, void *private, language_t lang )
{
    InGameMenu *igm = (InGameMenu *)private;

    locale_select_language( lang );

    // Remove all text verticies (and in this case the icons too) ready for rebuild
    // now that language has changed.
    int count = igm->back_vertice_group->index_len - igm->base_vert_count;
    flashlight_vertice_remove_quad2D( igm->back_vertice_group, count );

    //printf("ig_menu: Removed %d vertices from group. Indicies length is now %d from a max of %d\n", count, igm->back_vertice_group->indicies_len, igm->back_vertice_group->index_max );
}

// ----------------------------------------------------------------------------------
// Initialise in-game shot display
InGameMenu *
ig_menu_init( Flashlight *f, fonts_t *fonts )
{
    if( igm != NULL ) return igm; // There can only be one ig_menu

    igm = malloc( sizeof(InGameMenu) );
    if( igm == NULL ) {
        return NULL;
    }
    memset( igm, 0, sizeof(InGameMenu) );

    igm->f = f;


    igm->fonts = fonts;
    igm->state = Menu_Closed;

    add_image_quads( f, igm);

    igm->uniforms.texture_fade       = flashlight_shader_assign_uniform( f, "u_fade"               );
    igm->uniforms.translation_matrix = flashlight_shader_assign_uniform( f, "u_translation_matrix" );
   
    flashlight_event_subscribe( f, (void*)igm, ig_menu_event, FLEventMaskAll );

    // Initialise the menu backend, which in-turn will call the local callback 'init_menu_cb()' to
    // initialise each menu item.
    //
    igm->menu = menu_init( WIDTH, HEIGHT, MENU_ITEMS, 1, f, init_menu_cb, menu_item_set_position_cb, set_language_cb, MENU_FLAG_NONE, (void *)igm );
    menu_set_enabled_position ( igm->menu, HW, FY / 2 );
    menu_set_disabled_position( igm->menu, HW, -MHH );

    menu_set_selector_move_rate( igm->menu, 9 | S_NO_DECELERATE );
    menu_disable( igm->menu, 1 ); // This will establish the menu's starting position.

    igm->smenu = suspend_menu_init( f, fonts );
    igm->vkeyboard = virtual_keyboard_init( f, fonts );

    return igm;
}

// ----------------------------------------------------------------------------------

void
ig_menu_finish( InGameMenu **m ) {
    free(*m);
    *m = igm = NULL;
}

 
 // ----------------------------------------------------------------------------------
//
static void
ig_toggle_pause( int state )
{
    TSFUserEvent e = { {UserEvent, (void*)igm}, Pause, FL_EVENT_DATA_FROM_INT(state) };
    flashlight_event_publish( igm->f,(FLEvent*)&e );
}

// ----------------------------------------------------------------------------------

static void
ig_menu_event( void *self, FLEvent *event )
{
    static int enabled = 0;
    InGameMenu *igm = (InGameMenu *)self;

    switch( event->type ) {
        case Navigate:
            {
                if( !enabled )  break;

                int direction = ((TSFEventNavigate*)event)->direction;

                if( !igm->menu->moving ) {
                    if( direction & (INPUT_START & ~_INPUT_BUTTON) ) {
//igm->active ^= 1;
//printf("button p = %d\n", igm->active );
//ig_toggle_pause( igm->active );
//break;

                        if( igm->active ) 
                        {
                            // Mandatory check that pause is not currently changing state before we
                            // close the menu and request the chanve - it is possible to screw up the
                            // thread sync by cancelling the pause before the pause has been actioned
                            // by the trap.
                            if( !ui_can_change_pause_state() ) break;

                            ui_sound_play_menu_close();

                            igm->state  = Menu_Closed;
                            menu_disable( igm->menu, 8 );

                            TSFUserEvent e = { {UserEvent, (void*)igm}, InputClaim, FL_EVENT_DATA_FROM_INT(0) }; // Released
                            flashlight_event_publish( igm->f,(FLEvent*)&e );
                        } 
                        else
                        {
                            // Mandatory check that pause is not currently changing state before we
                            // open the menu and request the change - it is possible to screw up the
                            // thread sync by cancelling the pause before the pause has been actioned
                            // by the trap.
                            if( !ui_can_change_pause_state() ) break;

                            ui_sound_play_menu_open();
                            menu_enable( igm->menu, 8 );

                            TSFUserEvent e = { {UserEvent, (void*)igm}, InputClaim, FL_EVENT_DATA_FROM_INT(1) }; // Claimed
                            flashlight_event_publish( igm->f,(FLEvent*)&e );

                            ig_toggle_pause( 1 );
                        }
                        igm->active ^= 1;
                    }
                }
                break;
            }
        case Redraw:
            {
                if( igm->active || igm->menu->moving ) {
                    glUniformMatrix4fv( igm->uniforms.translation_matrix,1, GL_FALSE, menu_get_translation( igm->menu ) );
                    glUniform1f( igm->uniforms.texture_fade, 1.0 );

                    FLVerticeGroup *vg = igm->back_vertice_group;
                    flashlight_shader_blend_on(igm->f);
                    glDrawElements(GL_TRIANGLES, vg->indicies_len, GL_UNSIGNED_SHORT, flashlight_get_indicies(vg) );
                }
            }
            break;
        case Process:
            {
                if( !igm->active || !enabled ) break;

                switch( igm->state ) {
                    case Menu_VirtualKeyboard:
                    case Menu_SuspendPoint:
                        menu_set_selector_move_rate( igm->menu, 1 );

                    case Menu_Exit:
                        if( igm->menu->moving == 0 ) {
                            enabled     = 0;
                            igm->active = 0;
                   
                            if( igm->state == Menu_Exit ) {

                                TSFUserEvent e = { {UserEvent, (void*)igm}, EmulatorExit };
                                flashlight_event_publish( igm->f,(FLEvent*)&e );

                                ig_toggle_pause( 0 );
                            }
                            igm->state  = Menu_Closed;
                        }
                    default:
                        break;
                }
            }
            break;
        case UserEvent:
            {
                TSFUserEvent *ue = (TSFUserEvent*)event;
                switch(ue->user_type) {
                    //case EmulatorStarting:
                    case AutoloadComplete:
                        menu_reset_current( igm->vkeyboard->menu ); // TODO Fix these indirections
                        menu_reset_current( igm->smenu->fs_menu->menu ); 
                        menu_reset_current( igm->menu );
                        menu_set_selector_offscreen( igm->menu );
                        mainmenu_selector_set_hires();

                        igm->active  = 0;
                        enabled = 1;
                        break;

                    case MenuClosed:
                        if( ue->data == igm->vkeyboard  || ue->data == igm->smenu->fs_menu ) {
                            // One of our child menus has closed
                            menu_set_selector_move_rate( igm->menu, 9 | S_NO_DECELERATE );
                            enabled = 1;
                        }
                        if( ue->data == igm ) {

                            if( igm->state == Menu_Closed ) ig_toggle_pause( 0 ); 
                        }
                        break;
                    default:
                        break;
                }
            }
        default:
            break;
    }
}

// ----------------------------------------------------------------------------------

static void
_select_saverestore(menu_t *m, void *private, int id )
{
    InGameMenu *igm = (InGameMenu *)private;

    igm->state = Menu_SuspendPoint;
    menu_disable( igm->menu, 8 );

    suspend_menu_enable( igm->smenu );
}

static void
_select_keyboard(menu_t *m, void *private, int id )
{
    InGameMenu *igm = (InGameMenu *)private;

    igm->state = Menu_VirtualKeyboard;
    menu_disable( igm->menu, 8 );

    virtual_keyboard_enable( igm->vkeyboard );

    ig_toggle_pause( 0 );
}

static void
_select_exit(menu_t *m, void *private, int id )
{
    InGameMenu *igm = (InGameMenu *)private;

    ui_sound_play_menu_close();

    igm->state = Menu_Exit;
    menu_disable( igm->menu, 8 );
}

// ----------------------------------------------------------------------------------
// end

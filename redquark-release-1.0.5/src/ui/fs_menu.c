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

#include "fs_menu.h"
#include "tsf_joystick.h"
#include "ui_sound.h"
#include "user_events.h"
#include "gridmenu.h"
#include "path.h"
#include "settings.h"
#include "locale.h"
#include "fs_menu.h"
#include "mainmenu.h"
#include "palette.h"
#include "settings.h"

#define WIDTH 1280
#define HEIGHT 720

static void fs_menu_event( void *self, FLEvent *event );

#define FY 91
#define HW (WIDTH/2)
#define HH (HEIGHT/2)

#define MW  WIDTH
#define MH  HEIGHT
#define MHH (MH/2)
#define MHW (MW/2)

#define RULER_SPACE     60
#define RULER_THICKNESS 10

static int atlas_width;
static int atlas_height;

#define SE 30

// Position menu about the origin
static FLVertice menu_verticies[] = {
    { HW - MHW, (HH ), AC_BLUE + 0.1, 57.9}, // Background
    { HW - MHW, (HH ), AC_BLUE + 0.1, 57.1},
    { HW + MHW, (HH ), AC_BLUE + 0.9, 57.9},
    { HW + MHW, (HH ), AC_BLUE + 0.9, 57.1},

    {   0, RULER_SPACE + RULER_THICKNESS, 15, 10}, // Top ruler
    {   0, RULER_SPACE,                   15,  0},
    {1280, RULER_SPACE + RULER_THICKNESS, 16, 10},
    {1280, RULER_SPACE,                   16,  0},

    {   0,   0 + RULER_SPACE,                    15, 10}, // Bottom ruler
    {   0,   0 + RULER_SPACE + RULER_THICKNESS,  15,  0},
    {1280,   0 + RULER_SPACE,                    16, 10},
    {1280,   0 + RULER_SPACE + RULER_THICKNESS,  16,  0},

    { 1120 - SE - 20,   0 + SE + 17, 398, 119},  // Smaller Logo
    { 1120 - SE - 20,  35 + SE + 17, 398,  84},
    { 1280 - SE - 20,   0 + SE + 17, 558, 119},
    { 1280 - SE - 20,  35 + SE + 17, 558,  84},

    {    0 + SE + 31,   0 + SE - 0, 563,  23},  // "return" icon
    {    0 + SE + 31,  19 + SE - 0, 563,   4},
    {   77 + SE + 31,   0 + SE - 0, 640,  23},
    {   77 + SE + 31,  19 + SE - 0, 640,   4},
};

FLTexture *atlas_tx;

typedef void (*menuSelectCallback)     ( menu_t *m, void *private, int id );

#define REVEAL_RATE 4

// ----------------------------------------------------------------------------------
//
static void
build_title( FullScreenMenu *fsmenu )
{
    if( fsmenu->title_pane == NULL ) {
        fsmenu->title_pane = textpane_init( fsmenu->back_vertice_group, fsmenu->fonts->fontatlas, fsmenu->fonts->heading, 640, 60, WIDTH, HEIGHT, TEXT_LEFT );
    }

    unsigned char *title = locale_get_text( fsmenu->title_text_id );
    if( *title == '\0' ) title = (unsigned char *)"Not found";
    build_text( fsmenu->title_pane, title, fsmenu->flags & FS_MENU_FLAG_SUB_MENU ? TP_COLOUR_DARK : TP_COLOUR_LIGHT,  61, HH + (fsmenu->active_height / 2) - RULER_SPACE + 10);
}

// ----------------------------------------------------------------------------------
//
static void
add_pointer( FullScreenMenu *fsmenu )
{
    if( !(fsmenu->flags & FS_MENU_FLAG_POINTER) ) return;

    menu_add_pointer_verticies( fsmenu->menu, fsmenu->back_vertice_group, atlas_tx );
    menu_set_marked_item( fsmenu->menu, 0 );
}

// ----------------------------------------------------------------------------------
//
static FLTexture *
add_image_quads( Flashlight *f, FullScreenMenu *fsmenu )
{
    FLVertice vi[4];

    FLVerticeGroup *vg = flashlight_vertice_create_group(f, 8);
	fsmenu->back_vertice_group = vg;

    // Make sure the screen asset atlas texture is loaded
    FLTexture *tx = flashlight_load_texture( f, get_path("/usr/share/the64/ui/textures/menu_atlas.png") );
    if( tx == NULL ) { printf("Error tx\n"); exit(1); }

    atlas_width = tx->width;
    atlas_height = tx->height;

    flashlight_register_texture    ( tx, GL_NEAREST );
    flashlight_shader_attach_teture( f, tx, "u_DecorationUnit" );

    int half_height = fsmenu->active_height / 2;

    // Build triangles
    int i,j,y;
    float t,s;
    for(i = 0; i < sizeof(menu_verticies)/sizeof(FLVertice); i += 4 ) {
        for(j = 0; j < 4; j++ ) {

            // Calculate various Y coords based on active_height.
            y = menu_verticies[i + j].y;
            t = menu_verticies[i + j].t;
            s = menu_verticies[i + j].s;
            if( i == 0 ) {
                if( j == 0 || j == 2 ) y -= half_height;
                else                   y += half_height;

                // Adjust palette entry
                if(      fsmenu->flags & FS_MENU_FLAG_TRANSPARENT) t += 1;
                else if( fsmenu->flags & FS_MENU_FLAG_WARNING    ) t -= 1;

                if( fsmenu->flags & FS_MENU_FLAG_SUB_MENU   ) s  = (s - AC_BLUE) + AC_MEDIUM_GREY; //AC_LIGHT_BLUE;

            } else if( i == 4 ){ // Ajust top ruler position
                y = HH + half_height - y;
            } else if( i == 8 ) { // Ajust bottom ruler position
                y = HH - half_height + y;
            } else if ( i == 12 ) { // Logo
                if( fsmenu->flags & FS_MENU_FLAG_NO_LOGO ) y = -50;
                else y = HH - half_height + y;
            } else if ( i == 16 ) { // Return icon
                if( fsmenu->flags & FS_MENU_FLAG_NO_ICONS ) y = -50;
                else y = HH - half_height + y;
            }
            vi[j].x = X_TO_GL(WIDTH,      menu_verticies[i + j].x);
            vi[j].y = Y_TO_GL(HEIGHT,     y                      );
            vi[j].s = S_TO_GL(tx->width,  s                      );
            vi[j].t = T_TO_GL(tx->height, t                      );
            vi[j].u = 0.2;
        }
        flashlight_vertice_add_quad2D( vg, &vi[0], &vi[1], &vi[2], &vi[3] );
    }

    atlas_tx = tx;

    // Remember how many vertices form the base image so when changing language
    // we can keep these...
    fsmenu->base_vert_count = vg->index_len;

    build_title( fsmenu );

    return tx;
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
menu_set_language_cb( menu_t *m, void *private, language_t lang )
{
    FullScreenMenu *fsmenu = (FullScreenMenu *)private;

    fsmenu->lang = lang;

    locale_select_language( lang );

    int count = fsmenu->back_vertice_group->index_len - fsmenu->base_vert_count;
    flashlight_vertice_remove_quad2D( fsmenu->back_vertice_group, count );

    //printf("fs_menu: Removed %d vertices from group. Indicies length is now %d from a max of %d\n", count, fsmenu->back_vertice_group->indicies_len, fsmenu->back_vertice_group->index_max );
    
    build_title( fsmenu );
    
    if( fsmenu->flags & FS_MENU_FLAG_POINTER ) menu_add_pointer_verticies( fsmenu->menu, fsmenu->back_vertice_group, atlas_tx );
}

// ----------------------------------------------------------------------------------
// Initialise in-game shot display
FullScreenMenu *
fsmenu_menu_init( Flashlight *f, fonts_t *fonts, menuInitCallback mi_cb, void *private, char *title, int cols, int rows, int active_height, int active_y, fs_menu_flags_t flags, int transparent )
{
    FullScreenMenu *fsmenu = malloc( sizeof(FullScreenMenu) );
    if( fsmenu == NULL ) {
        return NULL;
    }
    memset( fsmenu, 0, sizeof(FullScreenMenu) );

    fsmenu->f = f;

    fsmenu->fonts = fonts;
    fsmenu->title_text_id = title;
    fsmenu->active_height = active_height;
    fsmenu->flags = flags;
    fsmenu->name = "";

    if( transparent ) fsmenu->flags |= FS_MENU_FLAG_TRANSPARENT;

    add_image_quads(f, fsmenu );

    fsmenu->uniforms.texture_fade       = flashlight_shader_assign_uniform( f, "u_fade"               );
    fsmenu->uniforms.translation_matrix = flashlight_shader_assign_uniform( f, "u_translation_matrix" );
   
    flashlight_event_subscribe( f, (void*)fsmenu, fs_menu_event, FLEventMaskAll );

    menu_flags_t mf = MENU_FLAG_NONE;
    if( flags & FS_MENU_FLAG_MATCH_ITEM_WIDTH ) mf |= MENU_FLAG_MATCH_ITEM_WIDTH;

    // Initialise the menu backend, which in-turn will call the local callback 'init_menu_cb()' to
    // initialise each menu item.
    //
    fsmenu->owner_private = private;
    fsmenu->menu = menu_init( WIDTH, HEIGHT, cols, rows, f, mi_cb, menu_item_set_position_cb, menu_set_language_cb, mf, fsmenu );

    //if( active_y < 0)  active_y = HH;
    if( active_y == HH ) fsmenu->has_claimed_screen = 1;
    if( fsmenu->flags & FS_MENU_FLAG_NO_CLAIM_SCREEN ) fsmenu->has_claimed_screen = 0;

    menu_set_enabled_position ( fsmenu->menu, HW, active_y );
    menu_set_disabled_position( fsmenu->menu, HW, -(active_height / 2) );

    menu_set_selector_move_rate( fsmenu->menu, 6 );
    menu_disable( fsmenu->menu, 1 ); // This will establish the menu's starting position.

    add_pointer( fsmenu );

    return fsmenu;
}

// ----------------------------------------------------------------------------------

void
fsmenu_menu_finish( FullScreenMenu **m ) {
    free(*m);
    //*m = fsmenu = NULL;
    *m = NULL;
}

// ----------------------------------------------------------------------------------

static void
fs_menu_event( void *self, FLEvent *event )
{
    FullScreenMenu *fsmenu = (FullScreenMenu *)self;

    if( !fsmenu->active && !fsmenu->menu->moving ) return;

    switch( event->type ) {
        case Navigate:
            {
                if( !fsmenu->active || (fsmenu->flags & FS_MENU_FLAG_IGNORE_INPUT) || fsmenu->overlaid_input != 0 )  break;

                int direction = ((TSFEventNavigate*)event)->direction;
                if( !fsmenu->menu->moving ) {
                    if( direction & (INPUT_START & ~_INPUT_BUTTON) ) {
                        if( fsmenu->active ) 
                        {
//printf("Menu button pressed for: %s\n", fsmenu->name );
                            fsmenu_menu_disable( fsmenu );
                        } 
                    }
                }
                break;
            }
        case Redraw:
            {
                if( fsmenu->overlaid == 0 && (fsmenu->active || fsmenu->menu->moving) ) {
//printf("Drawing menu %s\n", fsmenu->name );
                    glUniformMatrix4fv( fsmenu->uniforms.translation_matrix,1, GL_FALSE, menu_get_translation( fsmenu->menu ) );
                    glUniform1f( fsmenu->uniforms.texture_fade, 1.0 );

                    FLVerticeGroup *vg = fsmenu->back_vertice_group;
                    flashlight_shader_blend_on(fsmenu->f);
                    glDrawElements(GL_TRIANGLES, vg->indicies_len, GL_UNSIGNED_SHORT, flashlight_get_indicies(vg) );
                }
            }
            break;
        case Process:
            {
                // If the menu is no longer active, and is about to start moving out of view,
                // then make sure the background is being redrawn
                //
                if( !fsmenu->active && fsmenu->moving ) {
                    fsmenu->moving = 0;

                    if( fsmenu->has_claimed_screen ) {
//printf("Released fullscreen: %s\n", fsmenu->name );
                        TSFUserEvent e = { {UserEvent, (void*)fsmenu}, FullScreenReleased };
                        flashlight_event_publish( fsmenu->f,(FLEvent*)&e );
                    }

                    if( fsmenu->flags & FS_MENU_FLAG_STACKED ) {
                        // This correctly won't hit "this" menu as events are not dispatched to self.
                        TSFUserEvent e = { {UserEvent, (void*)fsmenu}, FSMenuClosing };
                        flashlight_event_publish( fsmenu->f,(FLEvent*)&e );
                    }

                    // FIXME Should not be in here really
                    if( fsmenu->flags & FS_MENU_FLAG_SETTINGS_MENU ) {
                        settings_save();
                    }


                    break;
                }

                // As soon as the menu is active and fullscreen (no longer moving) 
                // claim the fullscreen mode to stop everything behind the menu being
                // wastefully drawn!
                if( fsmenu->active && fsmenu->moving && !fsmenu->menu->moving ) {

                    if( fsmenu->has_claimed_screen ) {
//printf("Claimed fullscreen: %s\n", fsmenu->name );
                        TSFUserEvent e = { {UserEvent, (void*)fsmenu}, FullScreenClaimed };
                        flashlight_event_publish( fsmenu->f,(FLEvent*)&e );
                    }

                    if( fsmenu->flags & FS_MENU_FLAG_STACKED ) {
                        // This correctly won't hit "this" menu as events are not dispatched to self.
                        TSFUserEvent e = { {UserEvent, (void*)fsmenu}, FSMenuOpened };
                        flashlight_event_publish( fsmenu->f,(FLEvent*)&e );
                    }
                    
                    // If fullscreen claim is necessary, then we are only fully active (ie not moving)
                    // once the claim is complete.
                    fsmenu->moving = 0;
                    break;
                }
            }
            break;
        case UserEvent:
            {
                TSFUserEvent *ue = (TSFUserEvent*)event;
                switch(ue->user_type) {
                    case FSMenuClosing:
                        // If overlaying menu is fullscreen, then we need to resume displaying the menu beneath
                        if( ((FullScreenMenu *)(event->self))->flags & FS_MENU_FULLSCREEN ) fsmenu->overlaid--;
                        fsmenu->overlaid_input--;
                        break;
                    case FSMenuOpened:
                        // If overlaying menu is fullscreen, then we need to cease displaying the menu beneath
                        if( ((FullScreenMenu *)(event->self))->flags & FS_MENU_FULLSCREEN ) fsmenu->overlaid++;
                        fsmenu->overlaid_input++;
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
int
fsmenu_menu_enable( FullScreenMenu *fsmenu, int hires )
{
    if( fsmenu->moving || fsmenu->active ) return 0;

    fsmenu->active  = 1;
    fsmenu->moving  = 1;
    ui_sound_play_menu_open();
    menu_enable( fsmenu->menu, REVEAL_RATE );
    if( hires ) mainmenu_selector_set_hires();
    else mainmenu_selector_set_lowres();

    return 1;
}
    
// ----------------------------------------------------------------------------------
int
fsmenu_menu_disable( FullScreenMenu *fsmenu )
{
    if( fsmenu->moving || !fsmenu->active ) return 0;

    fsmenu->active  = 0;
    fsmenu->moving  = 1;
    ui_sound_play_menu_close();
    menu_disable( fsmenu->menu, REVEAL_RATE );

    return 1;
}

// ----------------------------------------------------------------------------------
// Forces a language change to rebuild menu - useful if menu text has changed
void
fsmenu_menu_reload_items( FullScreenMenu *fsmenu )
{
    menu_change_language( fsmenu->menu, fsmenu->lang );
}

// ----------------------------------------------------------------------------------
// end

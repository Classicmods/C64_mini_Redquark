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

#include "tsf_joystick.h"
#include "ui_sound.h"
#include "user_events.h"
#include "gridmenu.h"
#include "path.h"
#include "settings.h"
#include "selector.h"
#include "mainmenu.h"
#include "vkeyboard.h"
#include "keyboard.h" // Vice file

#define WIDTH 1280
#define HEIGHT 720

static void vk_menu_event( void *self, FLEvent *event );

#define HW (WIDTH/2)
#define HH (HEIGHT/2)

#define MW  WIDTH
#define MH  HEIGHT
#define MHH (MH/2)
#define MHW (MW/2)

#define LEFT_ADJ  15 //Left adjust to keep clear of safe area

static int atlas_width;
static int atlas_height;

// Position menu about the origin
static FLVertice menu_verticies[] = {
    { HW - 50, HH + 320,    0, 420}, 
    { HW - 50, HH - 320,  640, 420},
    { HW + 50, HH + 320,    0, 320},
    { HW + 50, HH - 320,  640, 320},
};

#define BLOCK_1_START   0
#define BLOCK_2_START   9
#define BLOCK_3_START   16

static int key_codes[] = {
    156,  48,  46,  32,
     18,  33,  34,  35,
     23,  36,  37,  38,
    160,  49,  24,  25,
    151,  19,  31,  20,
     22,  47, 152,  45,
    153, 159,  79,  80,
     81,  75,  76,  77,
     71,  72,  73,  82,
     78,  74, 150, 164,
    161, 162, 163, 155,
    165, 166, 167, 175,
    168, 169, 154,  55,
    170, 171, 157, 158,
    172, 173, 121,  83,
     98, 174, 117,  41,
      1, 103, 102,  15,
    105, 108, 106, 125,
     //42,  -1,  29,  -1,
     42,  42,  29,  29,
     59,  61,  63,  65,
     60,  62,  64,  66,
};

extern unsigned long frame_time_ms;

typedef void (*menuSelectCallback)     ( menu_t *m, void *private, int id );

static void _select_key(menu_t *m, void *private, int id );

#define REVEAL_RATE 8

static VirtualKeyboard *virt_keyboard = 0;

// ----------------------------------------------------------------------------------
//
static FLTexture *
add_image_quads( Flashlight *f, VirtualKeyboard *vkeyboard )
{
    FLVertice vi[4];

    FLVerticeGroup *vg = flashlight_vertice_create_group(f, 8);
	vkeyboard->vertice_group = vg;

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

    return tx;
}

// ----------------------------------------------------------------------------------
//
static void
init_menu_cb( menu_t *m, void *private, int id, int *enabled, menu_item_dim_t *dim, menuSelectCallback *ms_cb  )
{
    //VirtualKeyboard *vkeyboard = (VirtualKeyboard *)private;

    *enabled = 1;
    
    dim->w = 15;
    dim->h = 15;

    int item = id;

    if( item == 73 || item == 75 ) { // Skip 2nd half of double width icons
        *enabled = 0;
        return;
    }

    int r = item / 4;
    int c = item % 4;

    int x = 1280 - 100 - 21 + (26 * c) + 11  - LEFT_ADJ;
    int y = HH + 320   - 11 - (26 * r);

    if( r >= BLOCK_2_START ) y -= 6;
    if( r >= BLOCK_3_START ) y -= 6;

    if( item == 72 || item == 74 ) {
        dim->w = 40;
        x += 13;
    } 

    dim->x = x;
    dim->y = y;

    dim->selected_w = dim->w;
    dim->selected_h = dim->h;

    dim->selector_w = dim->w;
    dim->selector_h = dim->h;

    *ms_cb = _select_key; // The select callback
}

// ----------------------------------------------------------------------------------
static void 
menu_item_set_position_cb( menu_t *m, void *private, int id, int x, int y, int w, int h )
{
}

// ----------------------------------------------------------------------------------
// Initialise in-game shot display
VirtualKeyboard *
virtual_keyboard_init( Flashlight *f, fonts_t *fonts )
{
    VirtualKeyboard *vkeyboard = malloc( sizeof(VirtualKeyboard) );
    if( vkeyboard == NULL ) {
        return NULL;
    }
    memset( vkeyboard, 0, sizeof(VirtualKeyboard) );

    vkeyboard->f = f;
    vkeyboard->fonts = fonts;

    add_image_quads(f, vkeyboard);

    vkeyboard->uniforms.texture_fade       = flashlight_shader_assign_uniform( f, "u_fade"               );
    vkeyboard->uniforms.translation_matrix = flashlight_shader_assign_uniform( f, "u_translation_matrix" );
   
    flashlight_event_subscribe( f, (void*)vkeyboard, vk_menu_event, FLEventMaskAll );

    // Initialise the menu backend, which in-turn will call the local callback 'init_menu_cb()' to
    // initialise each menu item.
    //
    vkeyboard->menu = menu_init( WIDTH, HEIGHT, 4, 21, f, init_menu_cb, menu_item_set_position_cb, NULL, MENU_FLAG_NONE, (void *)vkeyboard );
    menu_sound_off( vkeyboard->menu );

    menu_set_enabled_position ( vkeyboard->menu, 1280 - 50 - LEFT_ADJ - 21, HH );
    menu_set_disabled_position( vkeyboard->menu, 1280 + 50 - LEFT_ADJ, HH );

    menu_set_selector_move_rate( vkeyboard->menu, 2 ); //| S_NO_DECELERATE );
    menu_disable( vkeyboard->menu, 1 ); // This will establish the menu's starting position.

    virt_keyboard = vkeyboard; // XXX GLOBAL

    return vkeyboard;
}

// ----------------------------------------------------------------------------------

void
virtual_keyboard_finish( VirtualKeyboard **m ) {
    free(*m);
    //*m = vkeyboard = NULL;
    *m = NULL;
}


// ----------------------------------------------------------------------------------
//
void
vk_next_block( VirtualKeyboard *vk )
{
    int r = vk->menu->current / 4;
    int id = 0;
    if( r >= BLOCK_1_START ) id = BLOCK_2_START * 4;
    if( r >= BLOCK_2_START ) id = BLOCK_3_START * 4;
    if( r >= BLOCK_3_START ) id = BLOCK_1_START * 4;

    menu_initialise_current_item( vk->menu, id ); // Moves selector
}

// ----------------------------------------------------------------------------------

static void
vk_menu_event( void *self, FLEvent *event )
{
    VirtualKeyboard *vkeyboard = (VirtualKeyboard *)self;

    if( !vkeyboard->active && !vkeyboard->menu->moving ) return;

    switch( event->type ) {
        case Navigate:
            {
                if( !vkeyboard->active )  break;
                if( !mainmenu_selector_owned_by_and_ready( vkeyboard->menu ) ) break;

                int direction = ((TSFEventNavigate*)event)->direction;
                if( !vkeyboard->menu->moving ) {

                    if( direction == INPUT_Y ) vk_next_block( vkeyboard );

                    if( direction == INPUT_X  ) vkeyboard_press_key( vkeyboard, 57 ); // Space

                    direction &= ~_INPUT_BUTTON;
                    if( direction & INPUT_A     ) vkeyboard_press_key( vkeyboard, 28 ); // Return
                    if( direction & INPUT_GUIDE ) vkeyboard_press_key( vkeyboard, 14 ); // Delete

                    if( direction & INPUT_START ) virtual_keyboard_disable( vkeyboard );
                }
                break;
            }
        case Redraw:
            {
                if( vkeyboard->active || vkeyboard->menu->moving ) {
                    glUniformMatrix4fv( vkeyboard->uniforms.translation_matrix,1, GL_FALSE, menu_get_translation( vkeyboard->menu ) );
                    glUniform1f( vkeyboard->uniforms.texture_fade, 1.0 );

                    FLVerticeGroup *vg = vkeyboard->vertice_group;
                    flashlight_shader_blend_off(vkeyboard->f);
                    glDrawElements(GL_TRIANGLES, vg->indicies_len, GL_UNSIGNED_SHORT, flashlight_get_indicies(vg) );
                }
            }
            break;
        case Process:
            {
                // TODO Clean this up
                if( !vkeyboard->active && vkeyboard->moving ) {
                    vkeyboard->moving = 0;
                    break;
                }

                if( vkeyboard->active && vkeyboard->moving && !vkeyboard->menu->moving ) {
                    vkeyboard->moving = 0;
                    break;
                }
            }
            break;
        case UserEvent:
            {
                TSFUserEvent *ue = (TSFUserEvent*)event;
                switch(ue->user_type) {
                    case MenuClosed:
                        if( ue->data == vkeyboard && vkeyboard->active ) {
                            vkeyboard->active = 0;
                            TSFUserEvent e = { {UserEvent, (void*)vkeyboard}, InputClaim, FL_EVENT_DATA_FROM_INT(0) }; // Released
                            flashlight_event_publish( vkeyboard->f,(FLEvent*)&e );
                        }
                        break;
                }
            }
            break;
        default:
            break;
    }
}

// ----------------------------------------------------------------------------------
void
virtual_keyboard_enable( VirtualKeyboard *vkeyboard )
{
    vkeyboard->active  = 1;
    vkeyboard->moving  = 1;
    ui_sound_play_menu_open();
    // Set initial offscreen location for selector, as this is where it will
    // return to on close.
    mainmenu_selector_push( vkeyboard );
    mainmenu_selector_set_position( 1280 + 300, 720/2, 21, 21 ); 

    menu_enable( vkeyboard->menu, REVEAL_RATE );
    mainmenu_selector_set_hires();

    TSFUserEvent e = { {UserEvent, (void*)vkeyboard}, InputClaim, FL_EVENT_DATA_FROM_INT(1) }; // Claimed
    flashlight_event_publish( vkeyboard->f, (FLEvent*)&e );

    TSFUserEvent ve = { {UserEvent, (void*)vkeyboard}, VirtualKeyboardChange, FL_EVENT_DATA_FROM_INT(1) }; // Open
    flashlight_event_publish( vkeyboard->f, (FLEvent*)&ve );
}
    
// ----------------------------------------------------------------------------------
void
virtual_keyboard_disable( VirtualKeyboard *vkeyboard )
{
    vkeyboard->moving  = 1;
    ui_sound_play_menu_close();
    menu_disable( vkeyboard->menu, REVEAL_RATE );
    mainmenu_selector_pop( vkeyboard );
    mainmenu_selector_reset_to_target();

    TSFUserEvent ve = { {UserEvent, (void*)vkeyboard}, VirtualKeyboardChange, FL_EVENT_DATA_FROM_INT(0) }; // Closed
    flashlight_event_publish( vkeyboard->f, (FLEvent*)&ve );
}

// ----------------------------------------------------------------------------------
//
void
virtual_keyboard_process_keys()
{
    if( virt_keyboard->keytime > frame_time_ms ) return;

    int i, key;
    for( i = 0; i < virt_keyboard->key_buffer_last_len; i++ ) {
        key = virt_keyboard->key_buffer_last[ i ];
        keyboard_key_released( key );
    }
    virt_keyboard->key_buffer_last_len = 0;

    for( i = 0; i < virt_keyboard->key_buffer_len; i++ ) {
        key = virt_keyboard->key_buffer[ i ];
        virt_keyboard->key_buffer_last[  virt_keyboard->key_buffer_last_len++ ] = key;
        keyboard_key_pressed( key );
    }
    virt_keyboard->key_buffer_len = 0;

    virt_keyboard->keytime = frame_time_ms + (20 * 5);  // Hold for 5 frames
}

// ----------------------------------------------------------------------------------
//
int
vkeyboard_press_key( VirtualKeyboard *vk, unsigned int keycode )
{
    if( vk->key_buffer_len < sizeof(vk->key_buffer_len) ) {
        vk->key_buffer[ vk->key_buffer_len++ ] = keycode;
    }
    return vk->key_buffer_len;
}

// ----------------------------------------------------------------------------------
//
static void
_select_key(menu_t *m, void *private, int id )
{
    VirtualKeyboard *vk = (VirtualKeyboard *)private;

    vkeyboard_press_key( vk, key_codes[id] );
}

// ----------------------------------------------------------------------------------
// end

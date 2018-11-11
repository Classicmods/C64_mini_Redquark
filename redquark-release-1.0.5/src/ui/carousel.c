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
#include <sys/time.h>


#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "flashlight.h"
#include "tsf_joystick.h"

#include "carousel.h"
#include "mainmenu.h"
#include "user_events.h"
#include "path.h"

#include "ui_sound.h" 
#include "ui_joystick.h" 

#define WIDTH 1280
#define HEIGHT 720
#define RATIO 140 / 200

#define CARD_HEIGHT       175
#define CARD_PIXEL_WIDTH  122         //CARD_HEIGHT * RATIO = 119
#define CARD_PIXEL_HEIGHT CARD_HEIGHT
#define CARD_COUNT        8           // The number of visible whole cards (two half cards count as one)
#define CARD_COUNT_OFFSCREEN 2
#define CARD_SPRITE_HEIGHT 175
#define CARD_SPRITE_WIDTH  122
#define CARD_PIXEL_SPACE (((WIDTH / CARD_COUNT) - CARD_PIXEL_WIDTH) + 1)  

#define TEX_CA_WIDTH  (CARD_SPRITE_WIDTH * (CARD_COUNT + CARD_COUNT_OFFSCREEN))
#define TEX_CA_HEIGHT CARD_SPRITE_HEIGHT

#define CPW CARD_PIXEL_WIDTH

#define CPB 90
#define CPT (CARD_PIXEL_HEIGHT + CPB)

#define CSH CARD_SPRITE_HEIGHT
#define CSW CARD_SPRITE_WIDTH

#define CARD_QUAD   0

static FLVertice card_verticies_init[] = {
    {  0,   CPB,   0, CSH,  0.3},    // The card texture (see CARD_QUAD)
    {  0,   CPT,   0,   0,  0.3},
    {CPW,   CPB, CSW, CSH,  0.3},
    {CPW,   CPT, CSW,   0,  0.3},
};

#define GREY_BACK 9000

static FLVertice card_verticies_frame[] = {

    {   0,  75,   4.1, 59.9,  0.2}, // Background
    {   0, 275,   4.1, 59.1,  0.2},
    {1280,  75,   4.9, 59.9,  0.2},
    {1280, 275,   4.9, 59.1,  0.2},
    
    {   0, 270,  15, 10,  0.2},  // Cover strip - Top  1 x 9
    {   0, 276,  15,  4,  0.2},
    {1280, 270,  16, 10,  0.2},
    {1280, 276,  16,  4,  0.2},

    {   0,  76,  16.1, 29,  0.2},  // Cover strip - bottom     1 x 18
    {   0,  82,  16.1, 23,  0.2},
    {1280,  76,  16.9, 29,  0.2},
    {1280,  82,  16.9, 23,  0.2},
};

#define SPIN_NORMAL 20
#define SPIN_MEDIUM 36
#define SPIN_FAST   58
#define SELECTOR_MOVE_RATE 8

#define NORMAL_TRANSLATE_RATE   5
#define FOCUS_TRANSLATE_RATE    5 // Disable carousel fly-in on first start
#define TRANSLATE_RATE(c)   ((c)->is_offscreen ? FOCUS_TRANSLATE_RATE : NORMAL_TRANSLATE_RATE)

static int atlas_width  = 0;
static int atlas_height = 0;

#define FIRST_MOVE_PAUSE 10 // in FRAME_PERIODS (ie FMP * FP)
#define FRAME_PERIOD     20 // In ms
extern unsigned long frame_time_ms;

#ifdef PV_ENABLE
static void print_vert( Carousel *c );
#endif

static void turn_carousel     ( Carousel *c, CarouselState d );
static void set_card_offscreen( Carousel *c, int card, CarouselState d );
static void carousel_event( void *self, FLEvent *event );

// ----------------------------------------------------------------------------------
static FLVerticeGroup *
add_background_quad( Flashlight *f )
{
    int len = sizeof(card_verticies_frame) / sizeof(FLVertice);

    FLVertice vi[4];
    FLVerticeGroup *vgb = flashlight_vertice_create_group(f, 1);

    // Make sure the screen asset atlas texture is loaded
    FLTexture *tx = flashlight_load_texture( f, get_path( "/usr/share/the64/ui/textures/menu_atlas.png") );
    if( tx == NULL ) { printf("Error tx\n"); exit(1); }

    flashlight_register_texture    ( tx, GL_NEAREST );
    flashlight_shader_attach_teture( f, tx, "u_DecorationUnit" );

    atlas_width  = tx->width; // Global
    atlas_height = tx->height; // Global

    int i,j;
    for(i = 0; i < len; i += 4 ) {
        for(j = 0; j < 4; j++ ) {
            vi[j].x = X_TO_GL(WIDTH,        card_verticies_frame[i + j].x);
            vi[j].y = Y_TO_GL(HEIGHT,       card_verticies_frame[i + j].y);
            vi[j].s = S_TO_GL(atlas_width,  card_verticies_frame[i + j].s);
            vi[j].t = T_TO_GL(atlas_height, card_verticies_frame[i + j].t);
            vi[j].u = card_verticies_frame[i + j].u;
        }
        flashlight_vertice_add_quad2D( vgb, &vi[0], &vi[1], &vi[2], &vi[3] );
    }
    return vgb;
}

// ----------------------------------------------------------------------------------
// Initialise carousel
// Repeateadly calls callback to fill visible card buffer. Private is passed to the callback
// usually used as a parent self reference so the callback can access it's own data.
//
Carousel *
carousel_init( Flashlight *f, LoadNextCardCallback lcb, int max_cards, void *private )
{
    if( lcb == NULL ) {
        printf("card_carousel_init: No callback provided\n" );
        exit(1);
    }

    int sz;

    Carousel *c = malloc( sizeof(Carousel) );
    if( c == NULL ) {
    }
    memset( c, 0, sizeof(Carousel) );

    c->f = f;

    c->max_cards        = max_cards;
    c->lcb              = lcb;
    c->callback_private = private;
    c->display_count    = CARD_COUNT;
    c->num_cards        = CARD_COUNT + CARD_COUNT_OFFSCREEN;
    c->state            = CAROUSEL_STOP;
    c->last             = CAROUSEL_STOP;
    c->position         = CARD_COUNT / 2;
    c->card_index       = 0;
    c->load_card        = -1;
    c->is_offscreen     = 1;

    c->offset_y = T_TO_GL(HEIGHT, -720); // Offscreen

    // Create cards
    sz = c->num_cards * sizeof(Card);
    Card *l = malloc( sz );
    if( l == NULL ) {
    }
    c->cards = l;
    memset( l, 0, sz );

    FLVerticeGroup *vg = flashlight_vertice_create_group(f, 2);
    if( vg == NULL ) {
    }
    c->vertice_group = vg;

    // Create card list
    sz = c->num_cards * sizeof(int);
    int *cix = malloc( sz );
    if( cix == NULL ) {
    }
    c->card_map = cix;
    memset( cix, 0, sz );

    c->vertice_group_background = add_background_quad( f );

    // Create texture atlas for cards
    FLTexture *tx;

    // Get the textures we neeed
    tx = flashlight_create_null_texture( f, CARD_SPRITE_WIDTH * c->num_cards, CARD_SPRITE_HEIGHT, GL_RGBA );
    if( tx == NULL ) { printf("Error tx\n"); exit(1); }
    c->texture = tx;

    flashlight_register_texture( c->texture, GL_LINEAR  );
    // Get a Mali version of bond texture
    c->maliFLTexture = mali_egl_image_get_texture( f->d->egl_display, c->texture->width, c->texture->height );

    flashlight_shader_attach_teture( f, c->texture, "u_CarouselUnit" );

    Card *card;

    // Load a display worth of cards.
    int ix = c->card_index - c->position;
    if( ix < 0 ) {
        ix += max_cards;
    }
    int i;
    for(i = 0; i < c->num_cards; i++) {
        c->card_map[i] = i;

        //printf( "Init card index %d to %d\n", i, c->card_map[i] );
       
        card = c->cards + i;

        card->maliFLTexture = c->maliFLTexture;
        card->width   = CARD_SPRITE_WIDTH;
        card->height  = CARD_SPRITE_HEIGHT;
        card->tex_x   = CARD_SPRITE_WIDTH * i;
       
        // Card loading callback
        (*lcb)(card, ix, c->callback_private);

        // Create initial quad, first map to GL coords (-1 <= c <= +1)
        FLVertice vi[4];

        int j,k;
        int len =  sizeof(card_verticies_init)/sizeof(FLVertice);

        for(k = 0; k < len; k += 4 ) {
            for(j = 0; j <  4; j++ ) {

                int tw = TEX_CA_WIDTH;
                int th = TEX_CA_HEIGHT;
                int so = i * CARD_SPRITE_WIDTH; 

                vi[j].u = card_verticies_init[j+k].u;

                if( vi[j].u == (float)0.2 ) { // Using menu atlas, choose appropriate texture dimensions
                    tw = atlas_width;
                    th = atlas_height;
                    so = 0;
                }

                vi[j].x = X_TO_GL(WIDTH,  card_verticies_init[j+k].x + i * (CARD_PIXEL_WIDTH + CARD_PIXEL_SPACE) + CARD_PIXEL_SPACE / 2);
                vi[j].y = Y_TO_GL(HEIGHT, card_verticies_init[j+k].y);
                vi[j].s = S_TO_GL(tw,     card_verticies_init[j+k].s + so);
                vi[j].t = T_TO_GL(th,     card_verticies_init[j+k].t);
            }

            flashlight_vertice_add_quad2D( vg, &vi[0], &vi[1], &vi[2], &vi[3] );
        }

        c->cards[i].ox = (CARD_PIXEL_WIDTH + CARD_PIXEL_SPACE) * i - 4; // Origin is off by 4 for some reason!
        c->cards[i].oy = card_verticies_init[4 * CARD_QUAD].y + CARD_PIXEL_HEIGHT / 2 - 0; // take Y of third quad (the texture)

        ix++;
        if( ix >= max_cards ) ix = 0;
    }

    c->uniforms.texture_fade       = flashlight_shader_assign_uniform( f, "u_fade"               );
    c->uniforms.translation_matrix = flashlight_shader_assign_uniform( f, "u_translation_matrix" );

    flashlight_event_subscribe( f, (void*)c, carousel_event, FLEventMaskAll );

    return c;
}

// ----------------------------------------------------------------------------------

void
carousel_finish( Carousel **c ) {
    if( (*c)->cards != NULL ) free( (*c)->cards );
    flashlight_vertice_release((*c)->f);
    free(*c);
    *c = NULL;
}

// ----------------------------------------------------------------------------------
static void
turn_carousel( Carousel *c, CarouselState d )
{
    int i;

    for( i = 0; i < c->num_cards; i++ ) { // last card is offscreen

        switch( d ) {
            case CAROUSEL_STOP:
                break;
            case CAROUSEL_RIGHT:
                c->card_map[i] = c->card_map[i] + 1; 
                if( c->card_map[i] >= c->num_cards ) c->card_map[i] = 0;
                break;
            case CAROUSEL_LEFT:
                c->card_map[i] = c->card_map[i] - 1; 
                if( c->card_map[i] < 0) c->card_map[i] = c->num_cards - 1;
                break;
            default:
                break;
        }

        // Re-target card in texture
        int s = c->card_map[i] * CARD_SPRITE_WIDTH;
        FLVertice *v[4];
        flashlight_vertice_get_quad2D( c->vertice_group, i * (CARD_QUAD + 1) + CARD_QUAD, &v[0], &v[1], &v[2], &v[3] );

        v[0]->s = v[1]->s = S_TO_GL(TEX_CA_WIDTH, s);
        v[2]->s = v[3]->s = S_TO_GL(TEX_CA_WIDTH, s + (CARD_SPRITE_WIDTH - 1));
    }
}

// ----------------------------------------------------------------------------------
// Keep in step with the card index in the "full library" (which is >= the number of
// cards we can display, plus one off screen) and load the next card texture about
// to come into view.
static void
move_to_next_card( Carousel *c )
{
    c->load_card = -1;
    switch( c->state ) {
        case CAROUSEL_RIGHT:
            c->card_index++;
            if( c->card_index > c->max_cards - 1 ) c->card_index = 0;

            c->load_card = c->card_index + 1;
            if( c->load_card > c->max_cards - 1 ) c->load_card -= c->max_cards;
            break;

        case CAROUSEL_LEFT:
            c->card_index--;
            if( c->card_index < 0 ) c->card_index = c->max_cards - 1;

            c->load_card = c->card_index - 1;
            if( c->load_card < 0 ) c->load_card += c->max_cards;
            break;
        default:
            break;
    }
}

// ----------------------------------------------------------------------------------
//
static void
load_next_card( Carousel *c )
{
    if( c->load_card >= 0 ) {
        int i = c->card_map[ c->num_cards - 1 ];
        Card *card = c->cards + i;

        //printf( "Loading next card into slot %d\n", i );

        card->maliFLTexture = c->maliFLTexture;
        card->width   = CARD_SPRITE_WIDTH;
        card->height  = CARD_SPRITE_HEIGHT;
        card->tex_x   = CARD_SPRITE_WIDTH * i;
        
        // Card loading callback
        (c->lcb)(card, c->load_card, c->callback_private);

        c->load_card = -1;
    }
}


// ----------------------------------------------------------------------------------
static void
set_card_offscreen( Carousel *c, int card, CarouselState d )
{
    FLVertice *v[4];
    flashlight_vertice_get_quad2D( c->vertice_group, card * (CARD_QUAD + 1) + CARD_QUAD, &v[0], &v[1], &v[2], &v[3] );

    switch( d ) {
        case CAROUSEL_STOP:
            break;
        case CAROUSEL_RIGHT:
            v[0]->x = v[1]->x = X_TO_GL(WIDTH, (c->num_cards - 1) * (CARD_PIXEL_WIDTH + CARD_PIXEL_SPACE) + CARD_PIXEL_SPACE / 2);
            v[2]->x = v[3]->x = X_TO_GL(WIDTH, (c->num_cards - 1) * (CARD_PIXEL_WIDTH + CARD_PIXEL_SPACE) + CARD_PIXEL_WIDTH + CARD_PIXEL_SPACE / 2);
            break;
        case CAROUSEL_LEFT:
            v[0]->x = v[1]->x = -1 - X_TO_GL(WIDTH, WIDTH/2 + CARD_PIXEL_WIDTH + CARD_PIXEL_SPACE / 2);
            v[2]->x = v[3]->x = -1 - X_TO_GL(WIDTH, WIDTH/2 + CARD_PIXEL_SPACE / 2);
            break;
        default:
            break;
    }
}

// ----------------------------------------------------------------------------------
#ifdef PV_ENABLE
static void print_vert( Carousel *c )
{
    int i;
    FLVertice *v[4];
    for( i = 0; i < c->num_cards; i++ ) {
        printf("Card %d\n", i);

        flashlight_vertice_get_quad2D( c->vertice_group, i, &v[0], &v[1], &v[2], &v[3] );
        int j;
        for( j = 0; j < 4; j++ ) {
            FLVertice *z = v[j];
            printf("Vert %d   x %04.6f  y %04.6f  s %04.6f  t %04.6f\n", j, z->x, z->y, z->s, z->t );
        }
    }
}
#endif

// ----------------------------------------------------------------------------------
void
carousel_stop( Carousel *c )
{
    c->keep_going = 0;
    //c->target_pause_t = 0;
    //c->target_pause_t = frame_time_ms + (FRAME_PERIOD * FIRST_MOVE_PAUSE);
    if( c->state & SELECTOR_PAUSE ) c->state = CAROUSEL_STOP;
}

// ----------------------------------------------------------------------------------
// Note the rotate direction is 
static void
carousel_rotate_selector_right( Carousel *c )
{
    if( !(c->state & _RIGHT) && c->state != CAROUSEL_STOP ) return;
    if( c->state == CAROUSEL_RIGHT ) return;

    c->keep_going++;

    set_card_offscreen( c, c->num_cards - 1, CAROUSEL_RIGHT );
    c->state = CAROUSEL_RIGHT;
    c->rotate_started_at = 0;
}

// ----------------------------------------------------------------------------------
static void
carousel_rotate_selector_left( Carousel *c ) 
{
    if( !(c->state & _LEFT) && c->state != CAROUSEL_STOP ) return;
    if( c->state == CAROUSEL_LEFT ) return;

    c->keep_going++;

    set_card_offscreen( c, c->num_cards - 1, CAROUSEL_LEFT );
    c->state = CAROUSEL_LEFT;
    c->rotate_started_at = 0;
}

// ----------------------------------------------------------------------------------
//
static void
carousel_move_selector_left( Carousel *c )
{
    // Deal with a navigation direction change while already moving/rotating
    if( !(c->state & _LEFT) && c->state != CAROUSEL_STOP ) {
        return;
    }

    c->selector_moving = 1;
    c->keep_going++;

    c->state = SELECTOR_LEFT;
    c->position--;
    c->card_index--;
    if( c->card_index < 0 ) c->card_index = c->max_cards - 1; // FIXME Refactor with code in turn_carousel

    int ox = c->cards[c->position].ox;
    int oy = c->cards[c->position].oy;

    mainmenu_selector_set_target_position( ox, oy, CARD_PIXEL_WIDTH, CARD_PIXEL_HEIGHT, 
            SELECTOR_MOVE_RATE | ((c->keep_going > 2) ? S_NO_DECELERATE : 0) );
}

// ----------------------------------------------------------------------------------
//
static void
carousel_move_selector_right( Carousel *c )
{
    // Deal with a navigation direction change while already moving/rotating
    if( !(c->state & _RIGHT) && c->state != CAROUSEL_STOP ) {
        return;
    }

    c->selector_moving = 1;
    c->keep_going++;

    c->state = SELECTOR_RIGHT;
    c->position++;
    c->card_index++;
    if( c->card_index > c->max_cards - 1 ) c->card_index = 0; // FIXME Refactor with code in turn_carousel

    int ox = c->cards[c->position].ox;
    int oy = c->cards[c->position].oy;

    mainmenu_selector_set_target_position( ox, oy, CARD_PIXEL_WIDTH, CARD_PIXEL_HEIGHT, 
            SELECTOR_MOVE_RATE | ((c->keep_going > 2) ? S_NO_DECELERATE : 0) );
}

// ----------------------------------------------------------------------------------
//
void
carousel_left( Carousel *c )
{
    if( c->selector_moving ) return;

    if( c->position > 1 ) {
        if( !c->selector_moving ) ui_sound_play_flip_delayed();
        carousel_move_selector_left( c );
    } else {
        carousel_rotate_selector_left( c ) ;
    }
}

// ----------------------------------------------------------------------------------
//
void
carousel_right( Carousel *c )
{
    if( c->selector_moving ) return;

    if( c->position < c->display_count-1 ) {
        if( !c->selector_moving ) ui_sound_play_flip_delayed();
        carousel_move_selector_right( c );
    } else {
        carousel_rotate_selector_right( c ) ;
    }
}

// ----------------------------------------------------------------------------------

void
carousel_select( Carousel *c, int js_id ) 
{
    if( c->state != CAROUSEL_STOP ) return;

    c->state = CAROUSEL_SELECTED;

    ui_sound_play_ding();

    CarouselSelection selection = {0};
    selection.card_index = c->card_index;
    selection.controller_id = js_id;

    TSFUserEvent e = { {UserEvent, (void*)c}, CarouselSelected, (void*)&selection };
    flashlight_event_publish( c->f,(FLEvent*)&e );
}

// ----------------------------------------------------------------------------------

//static int
//get_spin_rate( Carousel *c )
//{
//    int rate = SPIN_NORMAL;
//    static int rotate_watching_card = 0;
//
//    if( c->rotate_started_at == 0 ) {
//        c->rotate_started_at = 1;
//        rotate_watching_card = c->card_index;
//        return rate;
//    }
//
//    if( rotate_watching_card != c->card_index ) {
//        rotate_watching_card = c->card_index;
//        c->rotate_started_at++;
//    }
//
//    if( c->rotate_started_at > 8  ) rate = SPIN_MEDIUM;
//    if( c->rotate_started_at > 24 ) rate = SPIN_FAST;
//
//    return rate;
//}

// ----------------------------------------------------------------------------------
// Handle left-right rotateing and selection
static int
carousel_process( Carousel *c )
{
    int redraw = 0;
    switch(c->state) {
        case CAROUSEL_STOP:
            if( c->last != c->state ) redraw = 1;
            c->keep_going = 0;
            break;
        case CAROUSEL_RIGHT:
            // carousel_right means keep the selector stationary and rotate carousel left
            
            //rate = get_spin_rate(c);
            if( c->rotate_offset == 0 ) {

                c->target_rotate_offset = -(CARD_PIXEL_WIDTH + CARD_PIXEL_SPACE);
                c->target_t = frame_time_ms + (FRAME_PERIOD * SELECTOR_MOVE_RATE);

                //if( rate >= SPIN_FAST  ) ui_sound_play_flip();
                //else ui_sound_play_flip_delayed();
                //move_to_next_card( c );
                ui_sound_play_flip_delayed();
                move_to_next_card( c );
            }

            if( c->target_t < frame_time_ms + FRAME_PERIOD ) {
                turn_carousel( c, CAROUSEL_RIGHT );
                c->rotate_offset = 0;

                if( c->keep_going == 1 && c->target_pause_t < frame_time_ms ) {
                    c->target_pause_t = frame_time_ms + (FRAME_PERIOD * FIRST_MOVE_PAUSE);
                    c->state |= SELECTOR_PAUSE;
                    break;
                }
                if( !c->keep_going) c->state = CAROUSEL_STOP;
            }
            else c->rotate_offset += (FRAME_PERIOD * (c->target_rotate_offset - c->rotate_offset)) / (c->target_t - frame_time_ms);

            redraw = 1;
            break;
        case CAROUSEL_LEFT:
            // carousel_left means keep the selector stationary and rotate carousel right
           
            //rate = get_spin_rate(c);
            if( c->rotate_offset == 0 ) {

                c->target_rotate_offset = (CARD_PIXEL_WIDTH + CARD_PIXEL_SPACE);
                c->target_t = frame_time_ms + (FRAME_PERIOD * SELECTOR_MOVE_RATE);

                //if( rate >= SPIN_FAST  ) ui_sound_play_flip();
                //else ui_sound_play_flip_delayed();
                //move_to_next_card( c );
                ui_sound_play_flip_delayed();
                move_to_next_card( c );
            }
            
            if( c->target_t < frame_time_ms + FRAME_PERIOD ) {
                turn_carousel( c, CAROUSEL_LEFT );
                c->rotate_offset = 0;

                if( c->keep_going == 1 && c->target_pause_t < frame_time_ms ) {
                    c->target_pause_t = frame_time_ms + (FRAME_PERIOD * FIRST_MOVE_PAUSE);
                    c->state |= SELECTOR_PAUSE;
                    break;
                }
                if( !c->keep_going ) c->state = CAROUSEL_STOP;
            }
            else c->rotate_offset += (FRAME_PERIOD * (c->target_rotate_offset - c->rotate_offset)) / (c->target_t - frame_time_ms);
            redraw = 1;
            break;
        case SELECTOR_LEFT:
            redraw = 1;
            if( !c->selector_moving ) {
                if( c->keep_going == 1 && c->target_pause_t < frame_time_ms ) {
                    c->target_pause_t = frame_time_ms + (FRAME_PERIOD * FIRST_MOVE_PAUSE);
                    c->state |= SELECTOR_PAUSE ;
                    break;
                }
                if( !c->keep_going ) c->state = CAROUSEL_STOP;
                else carousel_left( c );
            }
            break;
        case SELECTOR_RIGHT:
            redraw = 1;
            if( !c->selector_moving ) {
                if( c->keep_going == 1 && c->target_pause_t < frame_time_ms ) {
                    c->target_pause_t = frame_time_ms + (FRAME_PERIOD * FIRST_MOVE_PAUSE);
                    c->state |= SELECTOR_PAUSE;
                    break;
                }
                if( !c->keep_going) c->state = CAROUSEL_STOP;
                else carousel_right( c );
            }
            break;
        case CAROUSEL_PAUSE_LEFT:
        case CAROUSEL_PAUSE_RIGHT:
        case SELECTOR_PAUSE_LEFT:
        case SELECTOR_PAUSE_RIGHT:
            if( c->target_pause_t < frame_time_ms ) {
                c->state &= ~SELECTOR_PAUSE;
                c->target_pause_t = 0;
            }
            c->keep_going++;
            break;
        default:
            break;
    }

    // Up/down translation processing
    if( c->target_offset_y != c->offset_y ) {
        if( c->target_offset_y_t < frame_time_ms + FRAME_PERIOD ) c->offset_y = c->target_offset_y;
        else c->offset_y += (FRAME_PERIOD * (c->target_offset_y - c->offset_y)) / (c->target_offset_y_t - frame_time_ms);
    }

    if( redraw ) {
        if( c->last != c->state ) { // Check previous state and only send event on a change
            // Send event indicating carousel select movement start/stop and which card we're on
            TSFUserEvent e = { {UserEvent, (void*)c}, c->state != CAROUSEL_STOP ? CarouselMove : CarouselStop, FL_EVENT_DATA_FROM_INT(c->card_index) };
            flashlight_event_publish( c->f,(FLEvent*)&e );

            c->last = c->state;
        }
    }

    return redraw;
}

// ----------------------------------------------------------------------------------
static void
carousel_event( void *self, FLEvent *event )
{
    Carousel *c = (Carousel*)self;
    //static int have_had_focus = 0;
    static int display_enabled = 1;
    static int stacked_selector_at_head = 0;

    switch( event->type ) {
        case Navigate:
            {
                if( !display_enabled ) break;;

                jstype_t type  = ((TSFEventNavigate*)event)->type;
                if( type != JOY_JOYSTICK ) break;

                int     js_id = ((TSFEventNavigate*)event)->id;
                int direction = ((TSFEventNavigate*)event)->direction;

                if( !c->focus) direction = 0; // We've not got the focus, so stop the carousel
                if( c->state == CAROUSEL_SELECTED || !mainmenu_selector_owned_by_and_ready( c ) ) direction = 0;

                // Discard any "it's a button" bits, so we can look specifically for INPUT_X etc
                direction &= ~_INPUT_BUTTON;

                if( !(direction & (INPUT_LEFT | INPUT_RIGHT)) ) carousel_stop  ( c );
                if(   direction & (_INPUT_SHLD | _INPUT_TRIG | INPUT_X | INPUT_Y)) carousel_select(c, js_id); 
                if(   direction & INPUT_LEFT  ) carousel_left  ( c );
                if(   direction & INPUT_RIGHT ) carousel_right ( c );
                break;
            }
        case Redraw:
            {
                if( display_enabled ) {
                    FLVerticeGroup *bg = c->vertice_group_background;
                    FLVerticeGroup *cg = c->vertice_group;

                    glUniform1f( c->uniforms.texture_fade, 1.0 );

                    float h = c->offset_y;

                    flashlight_shader_blend_off(c->f);
                    glUniformMatrix4fv( c->uniforms.translation_matrix, 1, GL_FALSE, flashlight_translate( 0.0, h, 0.0 ) );
                    glDrawElements(GL_TRIANGLES, bg->indicies_len, GL_UNSIGNED_SHORT, flashlight_get_indicies(bg) );


                    float d = (float)(c->rotate_offset - (CARD_PIXEL_WIDTH + CARD_PIXEL_SPACE + c->display_count)/2 ) / (WIDTH/2);
                    glUniformMatrix4fv( c->uniforms.translation_matrix, 1, GL_FALSE, flashlight_translate( d, h, 0.0 ) );
                    glDrawElements(GL_TRIANGLES, cg->indicies_len, GL_UNSIGNED_SHORT, flashlight_get_indicies(cg) );
                }
                break;
            }
        case Process:
            {
                carousel_process( c );
            }
            break;
        case FocusChange:
            {
                TSFFocusChange *fce = (TSFFocusChange*)event;
                if( fce->who == c ) {
                    if( c->focus ) break;

                    if( !stacked_selector_at_head ) {
                        // Under normal running, the carousel is the fist owner of the selector.
                        // This stops the selector wandering away when a game is selected from
                        // the carousel.
                        mainmenu_selector_push( c );
                        stacked_selector_at_head = 1;
                    }

                    // We have the focus
                    c->focus = 1;
                    c->state = CAROUSEL_STOP;
                    display_enabled = 1;

                    int ox = c->cards[c->position].ox;
                    int oy = c->cards[c->position].oy;
                    mainmenu_selector_set_lowres();

                    if( c->is_offscreen) mainmenu_selector_set_position( 1280/2, -720, CARD_PIXEL_WIDTH, CARD_PIXEL_HEIGHT);
                    mainmenu_selector_set_target_position( ox, oy, CARD_PIXEL_WIDTH, CARD_PIXEL_HEIGHT, TRANSLATE_RATE(c) );
                    //c->selector_moving = 1;

                    c->target_offset_y = T_TO_GL(HEIGHT, 0);
                    c->target_offset_y_t = frame_time_ms + FRAME_PERIOD * TRANSLATE_RATE(c);

                    c->is_offscreen = 0;

                    TSFUserEvent e = { {UserEvent, (void*)c}, CarouselStop, FL_EVENT_DATA_FROM_INT(c->card_index) };
                    flashlight_event_publish( c->f,(FLEvent*)&e );

                    ui_clear_joystick();

                    //if( have_had_focus ) ui_sound_play_flip(); // if() stops sound playing on startup focus
                } else {
                    if(c->focus ) {
                        // We've lost focus
                        c->focus = 0;

                        if( c->state == CAROUSEL_SELECTED ) break;

                        // toolbar probably has the focus, so move up out of the way
                        c->target_offset_y = T_TO_GL(HEIGHT, 20); //  Move to y offset 20
                        c->target_offset_y_t = frame_time_ms + FRAME_PERIOD * TRANSLATE_RATE(c);
                    }
                }
            }
            break;

		case VSyncDone:
            if( !c->focus) break;
            if( c->load_card >= 0 ) load_next_card( c );
            break;

        case UserEvent:
            switch(((TSFUserEvent*)event)->user_type) {
                case SelectorStop:
                    c->selector_moving = 0;
                    break;
                case FullScreenClaimed:
                    display_enabled = 0;
                    break;
                case FullScreenReleased:
                    display_enabled = 1;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

// ----------------------------------------------------------------------------------
// end

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
#ifndef CARDS_H
#define CARDS_H

#include <stdio.h>
#include <stdlib.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "flashlight.h"
#include <maliaw.h>

#include "uniforms.h"

typedef struct {
    mali_texture *maliFLTexture;
    int      width;
    int      height;
    int      tex_x;

    int       ox;         // } x,y of the cards origin (center)
    int       oy;         // }

    void     *private;    // Ptr to private data owned by carousel parent
} Card;

typedef enum {
    CAROUSEL_STOP  = 0,
    _LEFT          = 1,
    _RIGHT         = 2,
    CAROUSEL_SELECTED = 4,
    CAROUSEL_LEFT  = 16 + _LEFT,    // 17
    CAROUSEL_RIGHT = 16 + _RIGHT,   // 18
    SELECTOR_LEFT  = 32 + _LEFT,    // 33
    SELECTOR_RIGHT = 32 + _RIGHT,   // 34
    SELECTOR_PAUSE = 64,
    SELECTOR_PAUSE_LEFT  = SELECTOR_PAUSE + SELECTOR_LEFT,
    SELECTOR_PAUSE_RIGHT = SELECTOR_PAUSE + SELECTOR_RIGHT,
    CAROUSEL_PAUSE_LEFT  = SELECTOR_PAUSE + CAROUSEL_LEFT,
    CAROUSEL_PAUSE_RIGHT = SELECTOR_PAUSE + CAROUSEL_RIGHT,
    CAROUSEL_PAUSE_STOP  = SELECTOR_PAUSE + CAROUSEL_STOP,
} CarouselState;

typedef void (*LoadNextCardCallback)( Card *, int card_number, void *private );

typedef struct {
    int card_index;     // Which carousel card was selected
    int controller_id;  // The ID of the controller that made the selection
} CarouselSelection;

typedef struct {
    int num_cards;      // total number of allocated slots
    int display_count;  // The maximum width of the carousel, in cards

    int  *card_map;     // Mapping between each displayed card position and Card & FLVertice. Length num_cards.
                        
    int max_cards;
    int position;       // Index of the currently highlighted card.
    int card_index;     // Which card is currently selected out of the whole set
    int load_card;      // Indicates a card needs to be loaded, and which one

    LoadNextCardCallback lcb;
    void                *callback_private;

    Card *cards;        // List of loaded cards

    int selector_moving; // True when selector is moving
    int focus;

    float rotate_offset;
    float target_rotate_offset;
    unsigned long target_t;
    unsigned long target_pause_t;

    int keep_going;

    float target_offset_y;
    float offset_y;
    unsigned long target_offset_y_t;

    int   is_offscreen;

    CarouselState state;
    CarouselState next;
    CarouselState last;

    GLubyte *atlas;     // The texture atlas containing the loaded card images
    
    FLVerticeGroup *vertice_group;
    FLVerticeGroup *vertice_group_background;

    FLTexture *texture;
    Flashlight *f;

    mali_texture *maliFLTexture;

    Uniforms uniforms;

    unsigned long rotate_started_at;
} Carousel;

Carousel * carousel_init( Flashlight *f, LoadNextCardCallback lcb, int max_cards, void * callback_private );
#endif

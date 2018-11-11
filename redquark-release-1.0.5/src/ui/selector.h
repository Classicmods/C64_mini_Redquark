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
#ifndef SELECTORS_H
#define SELECTORS_H

#include <stdio.h>
#include <stdlib.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "flashlight.h"

#include "uniforms.h"
#include "event.h"

#define S_NO_DECELERATE  (1<<31)
#define SELECTOR_STACK_MAX 10

typedef struct {
    int enabled;
    int hires;

    float target_size_x;
    float target_size_y;
    float target_x;
    float target_y;
    uint32_t move_time_fu;

    int decel_count;
    unsigned long target_t;

    void *owner;

} SelectorParams;

typedef struct {
    Flashlight *f;

    FLVerticeGroup *vertice_group;
    FLTexture *texture;
    Uniforms uniforms;

    SelectorParams param;

    int is_hires;
    float size_x;
    float size_y;
    float x;
    float y;

    float set_width;
    float set_height;

    // Output values
    float scale_x;
    float scale_y;
    float translate_x;
    float translate_y;

    int   moving;

    SelectorParams stack[ SELECTOR_STACK_MAX ];
    int  stackp;
} Selector;

typedef struct {
    TSFUserEvent e;
} SelectorEvent;

Selector * selector_init( Flashlight *f );
void       selector_set_target_position( Selector *s, int ox, int oy, int sx, int sy, uint32_t move_rate );
void       selector_set_position( Selector *s, int ox, int oy, int sx, int sy );
void       selector_reset_to_target( Selector *s );
int        selector_enable ( Selector *s );
int        selector_disable( Selector *s );
void       selector_set_lowres( Selector *s );
void       selector_set_hires ( Selector *s );
int        selector_owned_by( Selector *s, void *owner );
int        selector_owned_by_and_ready( Selector *s, void *owner );
int        selector_moving( Selector *s );
int        selector_stack_push( Selector *s, void *owner );
int        selector_stack_pop( Selector *s, void * owner );

#endif

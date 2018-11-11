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
#ifndef GRIDMENU_H
#define GRIDMENU_H

#include "flashlight.h"
#include "settings.h"

typedef enum {
    MARKER_NONE = 0,
    MARKER_LEFT,
    MARKER_RIGHT
} menu_item_marker_pos_t;

typedef enum {
    MENU_FLAG_NONE        = 0,
    MENU_FLAG_MATCH_ITEM_WIDTH = 1<<0,
} menu_flags_t;

typedef struct {
    int   x;  
    int   y;
    int   w;              // Usual width
    int   h;              // Usual height

    int   selected_w;     // Selected width
    int   selected_h;     // Selected height

    int   selector_w;     // The selector width
    int   selector_h;     // The selector height

    menu_item_marker_pos_t marker_pos;

} menu_item_dim_t;

struct menu_s;
typedef struct menu_s menu_t;

typedef void (*menuSelectCallback)( menu_t *m, void *private, int id );
typedef void (*menuLanguageCallback)( menu_t *m, void *private, language_t lang );

typedef struct {
    menu_item_dim_t     dim;

    float   x;
    float   target_x;
    float   delta_x;

    float   y;
    float   target_y;
    float   delta_y;

    // w / h are used when expansion of current icon is required
    float   w;          // Current icon current width
    float   target_w;   // Current icon target width
    float   delta_w;    // Ajustment delta

    float   h;          // Current icon current height
    float   target_h;   // Current icon target height
    float   delta_h;    // Ajustment delta

    int     enabled;
    int     marked;

    menuSelectCallback      menu_select_cb;
} menu_item_t ;


typedef void (*menuInitCallback)       ( menu_t *m, void *private, int id, int *enabled, menu_item_dim_t *dim, menuSelectCallback *ms_cb );
typedef void (*menuSetPositionCallback)( menu_t *m, void *private, int id, int x, int y, int w, int h );

struct menu_s {
    menu_item_t *items;

    int     count_w;        // Number of items horizontally
    int     count_h;        // Number of columns vertically
    int     count;          // Totoal number of items
    int     enabled_count;
    int     current;
    int     focus;
    int     wait_for_focus_loss;
    int     moving;
    int     selector_moving;
    int     dwidth;
    int     dheight;
    int     awidth;         // Atlas width
    int     aheight;        // Atlas height - used for pointer
    uint32_t selector_move_rate;

    float   target_x;
    float   target_y;

    float   delta_x;
    float   delta_y;

    float   x;              // Current menu position
    float   y;

    int     enabled_ox;     // Menu position when enabled
    int     enabled_oy;
    int     disabled_ox;    // Menu position when disabled
    int     disabled_oy;

    int     marked_item;    // Which menu item is marked
    int     pointer_vertex_ix; // The index of the pointer "quad" in the owners vertice list.
    float   pointer_frame;  // Ajusted in fractional increments
    unsigned long pointer_target_time_ms;

    // Output values
    float translate_x;
    float translate_y;

    Flashlight     *f;
    FLVerticeGroup *vertice_group; // This belongs to the owner of the gridmenu

    // Callbacks
    menuInitCallback        create_menu_item;
    menuSetPositionCallback set_item_position;
    menuLanguageCallback    set_menu_language;

    int                     silent;
    menu_flags_t            flags;

    int                     had_select;     // Keep track of select presses to avoid multiple clicks


    void   *private;
};
        
menu_t * menu_init      ( int dw, int dh, int count_w, int count_h, Flashlight *f, menuInitCallback mi_cb, menuSetPositionCallback msp_cb, menuLanguageCallback mlang_cb, menu_flags_t flags, void *private );
void menu_set_enabled_position ( menu_t *m, int x, int y );
void menu_set_disabled_position( menu_t *m, int x, int y );
void menu_finish        ( menu_t **m );
void menu_focus         ( menu_t *m, int has );
void menu_initialise_selector( menu_t *m );
void menu_set_selector_move_rate( menu_t *m, uint32_t rate );
void set_item_visibility( menu_t *m, int id, int visible );
void menu_set_target_position( menu_t *s, int ox, int oy, int move_rate );
void menu_reset_current( menu_t *m );
void menu_enable( menu_t *m, int rate );
void menu_disable( menu_t *m, int rate );
float const * const menu_get_translation( menu_t *m );
void menu_restore_selector( menu_t *m );
void menu_set_marked_item( menu_t *m, int item );
void menu_set_current_item( menu_t *m, int ix );        // Does not move selector
void menu_initialise_current_item( menu_t *m, int ix ); // Moves selector
void menu_sound_off( menu_t *m );
void menu_sound_on( menu_t *m );
void menu_add_pointer_verticies( menu_t *m, FLVerticeGroup *vg, FLTexture *menu_atlas_tx );
void menu_set_selector_offscreen( menu_t *m );
void menu_change_language( menu_t *m, language_t lang ); // Can be use to reload menu items if they have changed

#endif

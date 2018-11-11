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
#ifndef IP_MENU_H
#define IP_MENU_H

#include <stdio.h>
#include <stdlib.h>

#include "flashlight.h"

#include "uniforms.h"
#include "fontatlas.h"
#include "fonts.h"
#include "textpane.h"
#include "fs_menu.h"


#define IP_MAX_TEXTS 3
#define IP_MAX_PAGES 256

typedef struct {
    Flashlight   *f;

    textpane_t   *text[IP_MAX_TEXTS];
    fonts_t      *fonts; 
    FLTexture      *tx; // Atlas

    FullScreenMenu *fs_menu;

    int             active;
    int             base_vert_count;

    unsigned char  *text_doc[2];  // | Memory mapped file. Must be released.
    int             text_len[2];  // |

    int             current_doc;

    int             current_page_offset[2];   // start of page
    int             next_page_offset[2];     // start of next page

    int             current_page_number[2];
    int             page_offsets[2][IP_MAX_PAGES];

    int             scroll_position;

} IPNotice;

IPNotice * ip_notice_init( Flashlight *f, fonts_t *fonts );
void ip_notice_finish ( IPNotice **ip );
void ip_notice_enable ( IPNotice *ip );
void ip_notice_disable( IPNotice *ip );

#endif

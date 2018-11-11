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
#ifndef VIRTUAL_KEYBOARD_H
#define VIRTUAL_KEYBOARD_H

#include <stdio.h>
#include <stdlib.h>

#include "flashlight.h"

#include "fs_menu.h"

typedef struct {
    Flashlight *f;
    FLVerticeGroup *vertice_group;

    textpane_t   *title;

    fonts_t      *fonts; 
    FLTexture      *texture;
    Uniforms      uniforms;

    menu_t        *menu;

    int           active;
    int           moving;

    unsigned long keytime;

    unsigned int  key_buffer[32];
    int           key_buffer_len;
    unsigned int  key_buffer_last[32];
    int           key_buffer_last_len;
} VirtualKeyboard;

VirtualKeyboard *virtual_keyboard_init( Flashlight *f, fonts_t *fonts );

void virtual_keyboard_disable( VirtualKeyboard *vkeyboard );
void virtual_keyboard_enable ( VirtualKeyboard *vkeyboard );
int vkeyboard_press_key( VirtualKeyboard *vk, unsigned int keycode );
void virtual_keyboard_process_keys();
#endif

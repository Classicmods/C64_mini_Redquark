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

#include "summary.h"
#include "user_events.h"

#include "harfbuzz_kerning.h"

#include "vec234.h"
#include "texture-atlas.h"
#include "texture-font.h"
#include "utf8-utils.h"
#include "font-manager.h"

#include "fontatlas.h"

#ifdef STATIC_FONT_LOADING
#include "path.h"
#include "flashlight_mapfile.h"
static void load_binary_atlas( fontatlas_t *f );
#endif

// ----------------------------------------------------------------------------------
//
fontatlas_t *
fontatlas_init( Flashlight *f, int atlas_size )
{
    fontatlas_t *fa = malloc(sizeof(fontatlas_t));
    if( f == NULL ) return NULL;

    fa->f = f;

#ifdef STATIC_FONT_LOADING
    fa->font_manager = font_manager_new( 420, 420, 1 ); // FIXME
    fa->atlas = flashlight_create_null_texture( f,
                                        fa->font_manager->atlas->width, 
                                        fa->font_manager->atlas->height, 
                                        GL_LUMINANCE );
    
    load_binary_atlas( fa );
#else
    fa->font_manager = font_manager_new( atlas_size, atlas_size, 1 );
    fa->atlas = flashlight_create_null_texture( f,
                                        fa->font_manager->atlas->width, 
                                        fa->font_manager->atlas->height, GL_LUMINANCE );
    if( fa->atlas == NULL ) { 
        printf("Error creating NULL texture for textpane\n");
        //FIXME Clean up font manager;
        return NULL;
    }

    fa->atlas->size = fa->font_manager->atlas->used; // FIXME PROBABLY WRONG, but we'll not use size anyway...
    fa->atlas->data = fa->font_manager->atlas->data;
#endif

    fa->font_max   = MAX_FONTS;
    fa->font_count = 0;

    return fa;
}

// ----------------------------------------------------------------------------------
// Returns font ID > -1 if successful
font_id_t
fontatlas_add_font( fontatlas_t *fa, const char *font_filename, int font_size )
{
    // FIXME Add font should be driven from the STATIC compiled stuct?
    //
    kerned_texture_font_t *kfont = NULL;

    if( fa->font_count == fa->font_max ) {
        printf("Too many fonts allocated: %d\n", fa->font_max );
        return -1;
    }

    do {
#ifdef STATIC_FONT_LOADING
        texture_font_index_t *font;
        font = find_precompiled_font( (char*)font_filename, font_size );

        char * path = get_path(  "/usr/share/the64/ui/fonts/" );

        kfont = kerned_texture_font_init( font, path );

        if( kfont == NULL ) break;
#else
        texture_font_t *font;
        font = font_manager_get_from_filename( fa->font_manager, font_filename, font_size );

        kfont = kerned_texture_font_init( font ); 
        if( kfont == NULL ) break;

        static char *charset = 
            "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
            "`abcdefghijlklmnopqrstuvwxyz{|}~"
            " !\"#$%&'()*+,-./0123456789:;<=>?"
            "ÄÉÖÜßäéöü" // German
            "ÀÂÆÇÈÉÊËÎÏÔŒÙÛŸàâæçèéêëîïôùûÿ" // French (Umlauts are in the German)
            "ÌÒìò" // Italian (other chars chared with French)
            "ÁÉÍÑÓÚ¿áéíñóú¡" // Spanish
            "™©®" // TM (C) (R)
            "›"
            ;

        ((texture_font_t*)kfont)->rendermode = RENDER_SIGNED_DISTANCE_FIELD;
        int remaining = texture_font_load_glyphs( (texture_font_t *)kfont, charset ); 
        if( remaining != 0 ) {
            printf("Failed to load all characters: %d remain\n", remaining );
            exit(1);
        }
#endif

        font_id_t font_id = (font_id_t)fa->font_count++;

        // Slot 0 is never used
        fa->kerned_font[ font_id ] = kfont;
        fa->font_size  [ font_id ] = font_size;

        return font_id;

    } while(0);

    // Only get here on error
    if( kfont ) kerned_texture_font_destroy( kfont );
    return -1;
}

// ----------------------------------------------------------------------------------
// Should be called once all fonts have been added to atlas, to add atlas to shader
void
register_fontatlas_with_shader( fontatlas_t *fa )
{
    // register font atlas texture with the next available texture unit
    int ret = flashlight_register_texture( fa->atlas, GL_LINEAR );
    if( ret < 0 ) {
        printf( "Error registering texture!\n");
        return;
    }
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

    // Attach texture unit of atlas to uniform variable
    flashlight_shader_attach_teture( fa->f, fa->atlas, "u_TextPaneUnit" ); // FIXME Change the name of this

    //test_write_png( "/tmp/fontatlas.png", fa->atlas->width, fa->atlas->height, fa->atlas->data );
}

// ----------------------------------------------------------------------------------
#ifdef STATIC_FONT_LOADING
static void
load_binary_atlas( fontatlas_t *fa )
{
    char *filename = get_path("/usr/share/the64/ui/textures/font_atlas.bin");
    int len = 0;
    void *img = flashlight_mapfile( filename, &len);
    if( img == NULL ) {
        fprintf(stderr, "Error: Could not load font atlas [%s]. Permissions or missing?\n", filename);
        return;
    }

    fa->atlas->size = len;
    fa->atlas->data = img;
}
#endif

// ----------------------------------------------------------------------------------

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
// Use Harfbuzz to provide glyph advance information
//
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
#include FT_LCD_FILTER_H
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "utf8-utils.h"

#define TSF_FONT_DATA
#include "harfbuzz_kerning.h"

#define HRES  64
#define HRESf 64.f
#define DPI   72

#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST     {
#define FT_ERROR_END_LIST       { 0, 0 } };

static const struct {
    int          code;
    const char*  message;
} FT_Errors[] =
#include FT_ERRORS_H

static hb_feature_t kern_on  = { HB_TAG('k','e','r','n'), 1, 0, UINT_MAX }; // Kerning on
static hb_feature_t liga_off = { HB_TAG('l','i','g','a'), 0, 0, UINT_MAX }; // Ligature off
static hb_feature_t clig_off = { HB_TAG('c','l','i','g'), 0, 0, UINT_MAX };


// ----------------------------------------------------------------------------
#ifdef STATIC_FONT_LOADING
static int
load_face(texture_font_index_t *self, float size, FT_Library *library, FT_Face *face, const char *path)
#else
static int
load_face(texture_font_t *self, float size, FT_Library *library, FT_Face *face)
#endif
{
    FT_Error error;
    FT_Matrix matrix = {
        (int)((1.0/HRES) * 0x10000L),
        (int)((0.0)      * 0x10000L),
        (int)((0.0)      * 0x10000L),
        (int)((1.0)      * 0x10000L)};

    assert(library);
    assert(size);

    /* Initialize library */
    error = FT_Init_FreeType(library);
    if(error) {
        fprintf(stderr, "FT_Error (0x%02x) : %s\n",
                FT_Errors[error].code, FT_Errors[error].message);
        goto cleanup;
    }

#ifdef STATIC_FONT_LOADING
    char fullpath[256] = {0};
    sprintf(fullpath, "%s%s", path, self->filename );

    error = FT_New_Face(*library, fullpath, 0, face);
#else
    /* Load face */
    switch (self->location) {
    case TEXTURE_FONT_FILE:
        error = FT_New_Face(*library, self->filename, 0, face);
        break;

    case TEXTURE_FONT_MEMORY:
        error = FT_New_Memory_Face(*library,
            self->memory.base, self->memory.size, 0, face);
        break;
    }
#endif

    if(error) {
        fprintf(stderr, "FT_Error (line %d, code 0x%02x) : %s\n",
                __LINE__, FT_Errors[error].code, FT_Errors[error].message);
        goto cleanup_library;
    }

    /* Select charmap */
    error = FT_Select_Charmap(*face, FT_ENCODING_UNICODE);
    if(error) {
        fprintf(stderr, "FT_Error (line %d, code 0x%02x) : %s\n",
                __LINE__, FT_Errors[error].code, FT_Errors[error].message);
        goto cleanup_face;
    }

    /* Set char size */
    error = FT_Set_Char_Size(*face, (int)(size * HRES), 0, DPI * HRES, DPI);

    if(error) {
        fprintf(stderr, "FT_Error (line %d, code 0x%02x) : %s\n",
                __LINE__, FT_Errors[error].code, FT_Errors[error].message);
        goto cleanup_face;
    }

    /* Set transform matrix */
    FT_Set_Transform(*face, &matrix, NULL);

    return 1;

cleanup_face:
    FT_Done_Face( *face );
cleanup_library:
    FT_Done_FreeType( *library );
cleanup:
    return 0;
}

// ----------------------------------------------------------------------------
#ifdef STATIC_FONT_LOADING
static void
index_font( font_table_t *ft, texture_font_index_t *font )
{
    memset( ft, 0, sizeof(font_table_t));
    ft->font = font;

	// index glyphs into buckets
	int i;
	for( i = 0; i < ft->font->glyph_count; i++ ) {

		int bucket_key = (unsigned int)(ft->font->glyphs[i].codepoint) % BUCKETS;

		//printf("Adding codepoint %d to bucket %d\n", ft->font->glyphs[i].codepoint, bucket_key );

		glyph_bucket_t *gb = &ft->glyph_bucket[ bucket_key ];

		gb->glyph_ptr[ gb->length++ ] = &ft->font->glyphs[i];
	}
}

// ----------------------------------------------------------------------------
//
#ifdef STATIC_FONT_LOADING
texture_font_index_t *
find_precompiled_font(  char *filename, int size )
{
    int font_count = sizeof(FONTS.font) / sizeof(texture_font_index_t);


    // Just keep the filename
    int l = strlen( filename );
    for( l--; l > 0; l-- ) {
        if( filename[l - 1] == '/' ) break;
    }

    filename += l;

    int i;
    for(i = 0; i < font_count; i++ ) {
        texture_font_index_t *fi = &FONTS.font[i];

        if( strcmp( filename, fi->filename ) == 0 ) {
            if( fi->font_size == size ) {
                return fi;
            }
        }
    }
    return NULL;
}

#endif

// ----------------------------------------------------------------------------
//
#ifdef STATIC_FONT_LOADING
static inline tsf_texture_glyph_t *
#else
static inline texture_glyph_t *
#endif
_get_glyph( font_table_t *ft, uint32_t codepoint )
{
    int bucket_key = (unsigned int)codepoint % BUCKETS;
    glyph_bucket_t *gb = &ft->glyph_bucket[ bucket_key ];

    int i;
#ifdef STATIC_FONT_LOADING
    tsf_texture_glyph_t *tg;
#else
    texture_glyph_t *tg;
#endif
    for( i = 0; i < gb->length; i++ ) {
        tg = gb->glyph_ptr[ i ];
        if( tg->codepoint == codepoint ) return tg;
    }
    return NULL;
}

// ----------------------------------------------------------------------------
//
#ifdef STATIC_FONT_LOADING
tsf_texture_glyph_t *
#else
texture_glyph_t *
#endif
kerned_texture_font_get_glyph( kerned_texture_font_t *ktf, const char * codepoint )
{
	font_table_t *ft = &ktf->font_table;

#ifdef STATIC_FONT_LOADING
    static tsf_texture_glyph_t special;
    tsf_texture_glyph_t *g;
#else
    texture_glyph_t *g;
#endif

    uint32_t ucodepoint = utf8_to_utf32( codepoint );
//printf("Look up codepoint %ld\n", ucodepoint );
//
  
    g = _get_glyph( ft, ucodepoint );
//if(g==NULL) printf("Glyph not found\n");
//else printf("Found. Width = %d\n", g->width );

    if( g == NULL ) {
        g = _get_glyph( ft, -1 ); // Glyph not found, fetch the special placeholder glyph
        memcpy( &special, g, sizeof(tsf_texture_glyph_t) );
        special.codepoint = ucodepoint;
        g = &special;
    }

    return g;
}
#endif

// ----------------------------------------------------------------------------
//
kerned_texture_font_t *
#ifdef STATIC_FONT_LOADING
kerned_texture_font_init( texture_font_index_t *font, const char *path )
#else
kerned_texture_font_init( texture_font_t *font )
#endif
{
    kerned_texture_font_t *ktf = malloc(sizeof(kerned_texture_font_t));
    if( ktf == NULL ) return NULL;

#ifdef STATIC_FONT_LOADING
	index_font( &ktf->font_table, font );
#else
    memcpy( (void*)ktf, (void*)font, sizeof(texture_font_t) );
#endif

    FT_Init_FreeType(&(ktf->library));
    
#ifdef STATIC_FONT_LOADING
    load_face(font, font->font_size, &(ktf->library), &(ktf->face), path );
#else
    load_face(font, font->size, &(ktf->library), &(ktf->face) );
#endif
    ktf->font = hb_ft_font_create(ktf->face, NULL);
    
    /* Create a buffer for harfbuzz to use */
    ktf->buf = hb_buffer_create();

    return ktf;
}
    
// ----------------------------------------------------------------------------

unsigned int
kerned_texture_font_process_text( kerned_texture_font_t *ktf, const char* text, int len )
{
    ktf->text = text;
    ktf->len  = len;

    hb_buffer_reset( ktf->buf );
    
    hb_buffer_set_direction(ktf->buf, HB_DIRECTION_LTR);
    hb_buffer_set_script   (ktf->buf, HB_SCRIPT_LATIN);
    hb_buffer_set_language (ktf->buf, hb_language_from_string("en",2));

    hb_feature_t feature[3] = { kern_on, liga_off, clig_off };

    hb_buffer_add_utf8( ktf->buf,  text, len, 0, len);
    hb_shape          ( ktf->font, ktf->buf, feature, 3);
    
    ktf->glyph_pos  = hb_buffer_get_glyph_positions(ktf->buf, &(ktf->glyph_count) );

    return ktf->glyph_count;
}

// ----------------------------------------------------------------------------

float
kerned_texture_font_get_advance( kerned_texture_font_t *ktf, int glyph_index )
{
    if( glyph_index >= ktf->glyph_count ) return 0;
    
    return (float)(ktf->glyph_pos[ glyph_index ].x_advance) / (64*64);
}

// ----------------------------------------------------------------------------

void
kerned_texture_font_destroy( kerned_texture_font_t *ktf )
{
    hb_buffer_destroy( ktf->buf  );
    hb_font_destroy  ( ktf->font );

    FT_Done_Face     ( ktf->face );
    FT_Done_FreeType ( ktf->library );

    free( (void*)ktf );
}

// ----------------------------------------------------------------------------

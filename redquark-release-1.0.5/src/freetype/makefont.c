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
/* Freetype GL - A C OpenGL Freetype engine
 *
 * Distributed under the OSI-approved BSD 2-Clause License.  See accompanying
 * file `LICENSE` for more details.
 */
#include "opengl.h"
#include "vec234.h"
#include "vector.h"
#include "freetype-gl.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef WIN32
#   define PRIzu "zu"
#else
#   define PRIzu "Iu"
#endif

// XXX Add new glyphs to it's own set, so that atlas and structs remain backwards XXX
// XXX compatible.                                                                XXX
static char * font_symbols[] = {
    // DO NOT change this first set, ever!
    "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
    "`abcdefghijlklmnopqrstuvwxyz{|}~"
    " !\"#$%&'()*+,-./0123456789:;<=>?"
    "ÄÉÖÜßäéöü" // German
    "ÀÂÆÇÈÉÊËÎÏÔŒÙÛŸàâæçèéêëîïôùûÿ" // French (Umlauts are in the German)
    "ÌÒìò" // Italian (other chars chared with French)
    "ÁÉÍÑÓÚ¿áéíñóú¡" // Spanish
    "™©®" // TM (C) (R)
    "›" // >
    ,

    // Glyphes added 04/03/2019
    //
    "œ°„“‚‘’”"   // oe, degree, low and left quotation (single and double)

    ,
    // Terminator
    "\0"
};

static char *copyright =
"/*                                                                       \n"  
" *  THEC64 Mini                                                          \n"
" *  Copyright (C) 2017 Chris Smith                                       \n"
" *                                                                       \n"
" *  This program is free software: you can redistribute it and/or modify \n"
" *  it under the terms of the GNU General Public License as published by \n"
" *  the Free Software Foundation, either version 3 of the License, or    \n"
" *  (at your option) any later version.                                  \n"
" *                                                                       \n"
" *  This program is distributed in the hope that it will be useful,      \n"
" *  but WITHOUT ANY WARRANTY; without even the implied warranty of       \n"
" *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        \n"
" *  GNU General Public License for more details.                         \n"
" *                                                                       \n"
" *  You should have received a copy of the GNU General Public License    \n"
" *  along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"
" */                                                                      \n";


// ------------------------------------------------------------- print help ---
void print_help()
{
    fprintf( stderr, "Usage: makefont [--help] "
             "--header <header file> "
             "--texture <texture size> "
             "--rendermode <one of 'normal', 'outline_edge', 'outline_positive', 'outline_negative' or 'sdf'>\n" );
}


// ------------------------------------------------------------------- main ---
//
void
emit_structs( FILE *file, size_t glyph_count )
{
    fprintf( file, copyright );
    fprintf( file,
        "#ifndef TSF_EMBEDDED_FONTS_H\n"
        "#define TSF_EMBEDDED_FONTS_H\n\n" );

    fprintf( file,
        "typedef struct\n"
        "{\n"
        "    int32_t codepoint;             // utf32\n"        
        "    int     width, height;\n"
        "    int     offset_x, offset_y;\n"
        "    float   s0, t0, s1, t1;\n"
        "} tsf_texture_glyph_t;\n\n" );

    fprintf( file,
        "typedef struct\n"
        "{\n"
        "    char            filename[128];\n"
        "    size_t          font_size;\n"
        "    size_t          glyph_count;\n"
        "    tsf_texture_glyph_t glyphs[%" PRIzu "];\n" // Make a ptr 
        "} texture_font_index_t;\n\n", glyph_count );

    fprintf( file,
        "typedef struct\n"
        "{\n"
        "    size_t               tex_width;\n"
        "    size_t               tex_height;\n"
        "    size_t               tex_depth;\n"
        "    texture_font_index_t font[3];\n"
        "} texture_font_atlas_t;\n\n" );
}

// ----------------------------------------------------------------------------
//
void
emit_atlas_metadata( FILE *file, texture_atlas_t *atlas )
{
    fprintf( file, "texture_font_atlas_t FONTS = {\n" );
    fprintf( file, "  %3" PRIzu ", %3" PRIzu ", %3" PRIzu ",\n", atlas->width, atlas->height, atlas->depth );
}

// ----------------------------------------------------------------------------
//
void
write_atlas_binary( char *filename, size_t texture_size, texture_atlas_t *atlas )
{
    int fd = open( filename, O_RDWR | O_CREAT | O_TRUNC, 0644 );
    if( fd < 0 ) {
        printf("Error writing atlas binary\n" );
        return;
    }
    write( fd, atlas->data, texture_size );
    close(fd);
}

// ----------------------------------------------------------------------------
//
void
emit_font( FILE *file, texture_font_t *font, int level )
{
    int glyph_count = font->glyphs->size;

    //fprintf( file, "  x{\n" );
    if(level > 0 ) fprintf( file, ",\n" );
    fprintf( file, "  {" );

    fprintf( file, "\"%s\", ", font->filename );
    fprintf( file, "%.f, ", font->size );
    fprintf( file, "%d,", glyph_count );

    fprintf( file, " {\n" );

    int i;
    for( i=0; i < glyph_count; ++i )
    {
        texture_glyph_t * glyph = *(texture_glyph_t **) vector_get( font->glyphs, i );

        fprintf( file, "    { %3d, ", glyph->codepoint );
        fprintf( file, "%3" PRIzu ", %3" PRIzu ", ", glyph->width, glyph->height );
        fprintf( file, "%3d, %3d, ", glyph->offset_x, glyph->offset_y );
        fprintf( file, "%ff, %ff, %ff, %ff", glyph->s0, glyph->t0, glyph->s1, glyph->t1 );
        fprintf( file, " },\n" );
    }

    //fprintf( file, "  z}\n" );
    fprintf( file, "    }\n" );
    fprintf( file, "  }" );
}

// ------------------------------------------------------------------- main ---
int main( int argc, char **argv )
{
    FILE* test;
    size_t i, j;
    int arg;

    float  font_size   = 0.0;
    const char * font_filename   = NULL;
    const char * header_filename = NULL;
    const char * variable_name   = "font";
    int show_help = 0;
    size_t texture_width = 128;
    rendermode_t rendermode = RENDER_NORMAL;
    const char *rendermodes[5];
    rendermodes[RENDER_NORMAL] = "normal";
    rendermodes[RENDER_OUTLINE_EDGE] = "outline edge";
    rendermodes[RENDER_OUTLINE_POSITIVE] = "outline added";
    rendermodes[RENDER_OUTLINE_NEGATIVE] = "outline removed";
    rendermodes[RENDER_SIGNED_DISTANCE_FIELD] = "signed distance field";

    for ( arg = 1; arg < argc; ++arg )
    {
        if ( 0 == strcmp( "--header", argv[arg] ) || 0 == strcmp( "-o", argv[arg] )  )
        {
            ++arg;

            if ( header_filename )
            {
                fprintf( stderr, "Multiple --header parameters.\n" );
                print_help();
                exit( 1 );
            }

            if ( arg >= argc )
            {
                fprintf( stderr, "No header file given.\n" );
                print_help();
                exit( 1 );
            }

            header_filename = argv[arg];
            continue;
        }

        if ( 0 == strcmp( "--help", argv[arg] ) || 0 == strcmp( "-h", argv[arg] ) )
        {
            show_help = 1;
            break;
        }

        if ( 0 == strcmp( "--texture", argv[arg] ) || 0 == strcmp( "-t", argv[arg] ) )
        {
            ++arg;

            if ( 128.0 != texture_width )
            {
                fprintf( stderr, "Multiple --texture parameters.\n" );
                print_help();
                exit( 1 );
            }

            if ( arg >= argc )
            {
                fprintf( stderr, "No texture size given.\n" );
                print_help();
                exit( 1 );
            }

            errno = 0;

            texture_width = atof( argv[arg] );

            if ( errno )
            {
                fprintf( stderr, "No valid texture size given.\n" );
                print_help();
                exit( 1 );
            }

            continue;
        }

        if ( 0 == strcmp( "--rendermode", argv[arg] ) || 0 == strcmp( "-r", argv[arg] ) )
        {
            ++arg;

            if ( rendermode != RENDER_NORMAL )
            {
                fprintf( stderr, "Multiple --rendermode parameters.\n" );
                print_help();
                exit( 1 );
            }

            if ( arg >= argc )
            {
                fprintf( stderr, "No texture size given.\n" );
                print_help();
                exit( 1 );
            }

            errno = 0;

            if( 0 == strcmp( "normal", argv[arg] ) )
            {
                rendermode = RENDER_NORMAL;
            }
            else if( 0 == strcmp( "outline_edge", argv[arg] ) )
            {
                rendermode = RENDER_OUTLINE_EDGE;
            }
            else if( 0 == strcmp( "outline_positive", argv[arg] ) )
            {
                rendermode = RENDER_OUTLINE_POSITIVE;
            }
            else if( 0 == strcmp( "outline_negative", argv[arg] ) )
            {
                rendermode = RENDER_OUTLINE_NEGATIVE;
            }
            else if( 0 == strcmp( "sdf", argv[arg] ) )
            {
                rendermode = RENDER_SIGNED_DISTANCE_FIELD;
            }
            else
            {
                fprintf( stderr, "No valid render mode given.\n" );
                print_help();
                exit( 1 );
            }

            continue;
        }

        fprintf( stderr, "Unknown parameter %s\n", argv[arg] );
        print_help();
        exit( 1 );
    }

    if ( show_help )
    {
        print_help();
        exit( 1 );
    }

    if ( !header_filename )
    {
        fprintf( stderr, "No header file given.\n" );
        print_help();
        exit( 1 );
    }

    texture_atlas_t * atlas = texture_atlas_new( texture_width, texture_width, 1 );
    texture_font_t  * font1  = texture_font_new_from_file( atlas, 36, "Roboto-Regular.ttf" );
    texture_font_t  * font2  = texture_font_new_from_file( atlas, 22, "Roboto-Regular.ttf" );
    texture_font_t  * font3  = texture_font_new_from_file( atlas, 22, "Roboto-Italic.ttf" );
    
    font1->rendermode = rendermode;
    font2->rendermode = rendermode;
    font3->rendermode = rendermode;


    size_t missed = 0;
    // Load the glyph SETS one at a time so that the atlas and font struct remain backwards compatible
    // when we add further glyph sets.
    //
    i = 0;
    while( *font_symbols[i] != '\0' ) {
        missed += texture_font_load_glyphs( font1, font_symbols[i] );
        missed += texture_font_load_glyphs( font2, font_symbols[i] );
        missed += texture_font_load_glyphs( font3, font_symbols[i] );
        i++;
    }

    printf( "Font filename           : %s\n"
            //"Font size               : %.1f\n"
            //"Number of glyphs        : %ld\n"
            "Number of missed glyphs : %ld\n"
            "Texture size            : %ldx%ldx%ld\n"
            "Texture occupancy       : %.2f%%\n"
            "\n"
            "Header filename         : %s\n"
            "Variable name           : %s\n"
            "Render mode             : %s\n",
            font_filename,
            //font_size,
            //strlen(font_symbols) + strlen(font_symbols_additional),
            missed,
            atlas->width, atlas->height, atlas->depth,
            100.0 * atlas->used / (float)(atlas->width * atlas->height),
            header_filename,
            variable_name,
            rendermodes[rendermode] );
    
    FILE *file = fopen( header_filename, "w" );

    size_t texture_size = atlas->width * atlas->height *atlas->depth;

    emit_structs( file, font1->glyphs->size );

    fprintf( file, "#ifdef TSF_FONT_DATA\n" );
    emit_atlas_metadata( file, atlas );

    fprintf( file, "  {\n" );

    emit_font( file, font1, 0 );
    emit_font( file, font2, 1 );
    emit_font( file, font3, 2 );

    fprintf( file, "\n  }\n};\n" );
    fprintf( file, "#endif\n#endif\n" );

    write_atlas_binary( "font_atlas.bin", texture_size, atlas );

    fclose( file );

    return 0;
}

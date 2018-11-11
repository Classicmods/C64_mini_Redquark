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
#include "videoarch.h"
#include "video.h"
#include "palette.h"
#include "viewport.h"
#include "video/render1x1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//static FLScreen   *screen   = NULL;
static unsigned char *screen = NULL;
static video_canvas_t *cached_video_canvas;

unsigned char *c64_create_screen()
{
    int tsize =  (384 * (DEPTH / 8)) * 272;
    screen = malloc( tsize );
    return screen;
}

int video_arch_resources_init(void)
{
    // Configuration resources
    return 0;
}

void video_arch_resources_shutdown(void)
{
}

int video_init_cmdline_options(void)
{
    return 0;
}

int video_init(void)
{
    return 0;
}

void video_shutdown(void)
{
}

// ------------------------------------------------------------------------------------------

video_canvas_t *
video_canvas_create(video_canvas_t *canvas, unsigned int *width, unsigned int *height, int mapped)
{
    canvas->depth = DEPTH;
    canvas->width = 320; // NOT USED - TODO Remove from canvas struct
    canvas->height =240; // NOT USED - TODO Remove from canvas struct

    draw_buffer_t *db = canvas->draw_buffer;
    db->canvas_physical_width  = db->visible_width;
    db->canvas_physical_height = 240; //db->visible_height;

    // This probably has no effect, since we hotwire straight into the RGB 1x1 renderer in
    // video_canvas_refresh()
    canvas->videoconfig->rendermode = VIDEO_RENDER_RGB_1X1;
    canvas->videoconfig->double_size_enabled = 0;
    canvas->videoconfig->doublesizex = 0;
    canvas->videoconfig->doublesizey = 0;
    canvas->videoconfig->doublescan = 0;

    video_viewport_resize(canvas, 0);

    // XXX we use 240 instead of 272 since 240 * 3 = 720 
    //     If we use 272, then OpenGLES will squash the texture vertically to fit

    // 504 x 312    384 x 272
    //canvas->texture = malloc( (384 * (DEPTH / 8)) * 272 );

    video_canvas_set_palette(canvas, canvas->palette);
    video_render_initraw(canvas->videoconfig);

    return canvas;
}

// --------------------------------------------------------------------------------------

// This is called repeatedly to render dirty rectangles
void 
video_canvas_refresh(struct video_canvas_s *canvas, unsigned int xs, unsigned int ys, unsigned int xi, unsigned int yi, unsigned int w, unsigned int h)
{
    if (canvas->width == 0) return;

    // As we've reduced the height of our canvas from 272 to 240, we need a specific check that we're not
    // drawing outside the canvas. This is standard Vice code, which takes into account screen resizing.
    if (xi + w > canvas->draw_buffer->canvas_physical_width || yi + h > canvas->draw_buffer->canvas_physical_height) {
        //printf("Attempt to draw outside canvas!\nXI%i YI%i W%i H%i CW%i CH%i\n", 
        //       xi, yi, w, h, canvas->draw_buffer->canvas_physical_width,
        //       canvas->draw_buffer->canvas_physical_height);
        return;
    }

    //static int count = 0;
    //if( ++count == 50 ) {
    //    printf( "video_canvas_refresh: w %u  h %u  xs %u  ys %u  xi %u  yi %u\n", w, h, xs, ys, xi, yi );
    //    count = 0;
    //}

    // We force an RGB 1x1 @ 32bpp render mode
    //
    viewport_t *viewport = canvas->viewport;

    if (!canvas->videoconfig->color_tables.updated) { /* update colors as necessary */
        video_color_update_palette(canvas);
    }

    if (w <= 0) return; /* some render routines don't like invalid width */

    const video_render_color_tables_t *colortab = &(canvas->videoconfig->color_tables);

    render_32_1x1_04(colortab, canvas->draw_buffer->draw_buffer, canvas->screen, //canvas->screen->texture, 
            w, h,
            xs, ys, xi, yi, canvas->draw_buffer->draw_buffer_width, 
            canvas->draw_buffer->canvas_physical_width * (DEPTH / 8));
}

// ------------------------------------------------------------------------------------------
//
int 
video_canvas_set_palette(struct video_canvas_s *canvas, struct palette_s *palette)
{
    unsigned int i, col;

    if (palette == NULL) {
        return 0; // Palette not create yet.
    }

    for (i = 0; i < palette->num_entries; i++) {
        col = (palette->entries[i].red   <<   RED_SHIFT) | 
              (palette->entries[i].green << GREEN_SHIFT) |
              (palette->entries[i].blue  <<  BLUE_SHIFT) | 0xff000000; // ff is the alpha

        video_render_setphysicalcolor(canvas->videoconfig, i, col, canvas->depth);
    }

    canvas->palette = palette;

    return 0;
}

// ------------------------------------------------------------------------------------------
static int v_adjust = 0;  // +ve means move up. -ve down  ( -16 <= adjust <= 16 )

void
ui_set_vertical_shift( int adjust )
{
    if( (adjust < -16) || (adjust > 16) ) {
        printf("Error: ui_set_vertical_shift adjust out of range: %d\n", adjust );
        return;
    }
    v_adjust = adjust;

    // this ends up calling video_canvas_resize, where we adjust the vertical position as needed.
    video_viewport_resize( cached_video_canvas, 0 );
}

// ------------------------------------------------------------------------------------------
void video_canvas_resize(struct video_canvas_s *canvas, char resize_canvas)
{
    // This is naughty... but we intercept the setup of the viewport here and tweak it if needed
    // based on the loaded game, to shift the rendered viewport up or down +/-16 lines.
    canvas->viewport->first_line += v_adjust;
    canvas->viewport->last_line  += v_adjust;
}

// ------------------------------------------------------------------------------------------
void video_arch_canvas_init(struct video_canvas_s *canvas)
{
    canvas->video_draw_buffer_callback = NULL;
    
    if( screen == NULL ) {
        printf( "Call to video_arch_canvas_init before screen was initilised (c64_create_screen())\n");
        exit(1);
    }
    canvas->screen = screen;

    cached_video_canvas = canvas;

}

// ------------------------------------------------------------------------------------------
void video_canvas_destroy(struct video_canvas_s *canvas)
{
}

// ------------------------------------------------------------------------------------------
void video_add_handlers(void)
{
}


// ------------------------------------------------------------------------------------------
char video_canvas_can_resize(video_canvas_t *canvas)
{
    return 0;
}

// --------------------------------------------------------------------------

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
// Source in a dummy local config.h or vice config.h (depending on what build
// is bringing in this file) so that USE_XAWUI gets set for the X11 Vice build
//

#ifndef THE64_VIDEOARCH_H
#define THE64_VIDEOARCH_H

struct video_canvas_s {
    unsigned int initialized;
    unsigned int created;
    //float refreshrate;

    /* Index of the canvas, needed for x128 and xcbm2 */
    int index;
    unsigned int depth;

    /* Size of the drawable canvas area, including the black borders */
    unsigned int width, height;

    /* Size of the canvas as requested by the emulator itself */
    unsigned int real_width, real_height;

    /* Actual size of the window; in most cases the same as width/height */
    unsigned int actual_width, actual_height;

    unsigned char *gone;

    struct video_render_config_s *videoconfig;
    struct draw_buffer_s *draw_buffer;
    struct viewport_s *viewport;
    struct geometry_s *geometry;
    struct palette_s *palette;

    struct video_draw_buffer_callback_s *video_draw_buffer_callback;

    unsigned char *screen;

};

    // Direct Mali memory access has texture in BRGA
#   define TEXTURE_FORMAT  "BRGA"
#   define RED_SHIFT       16
#   define GREEN_SHIFT     8
#   define BLUE_SHIFT      0

#define DEPTH 32

typedef struct video_canvas_s video_canvas_t;

#endif


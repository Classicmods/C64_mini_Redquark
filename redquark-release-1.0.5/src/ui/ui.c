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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <linux/fb.h>
#include <sys/ioctl.h>

#include "flashlight.h"

#include "user_events.h"
#include "uniforms.h"
#include "mainmenu.h"
#include "fonts.h"
#include "event.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "poweroff.h"
#include "ui_sound.h"
#include "path.h"

#include "locale.h"
#include "emucore.h"
#include "firmware.h"
#include "settings.h"
#include "ui_joystick.h"
#include "uitraps.h"

#include "flashlight.h"

#ifdef HDMI_TEST
#  define HEIGHT 576
#  define WIDTH  720
#else
#  define HEIGHT 720
#  define WIDTH  1280
#endif

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#ifdef GL_EGL_DEBUG
static int glerr = EGL_SUCCESS;
#define EGL_CHECK_ERROR(s) \
    if( (glerr = eglGetError()) != EGL_SUCCESS ) { \
        printf("%s: EGL Error 0x%x\n", s, glerr ); \
        exit(1); \
    }
#define GL_CHECK_ERROR(s) \
    if( (glerr = glGetError()) != GL_NO_ERROR ) { \
        printf("%s: GL Error 0x%x\n", s, glerr ); \
        exit(1); \
    }
#endif


static Flashlight *fl;
static MainMenu *main_menu;

static int running = 0;
static int shutdown_mode = QUIT_SHUTDOWN;

#define FRAME_PERIOD 20
static unsigned long initial_start_sec = 0;
unsigned long device_scan_period_ms = 0;
unsigned long frame_time_ms = 0;

static void process_frame( Flashlight *f );
static void ui_event( void *self, FLEvent *event );

// ----------------------------------------------------------------------------------
//
static inline void
do_glFinish()
{
    static unsigned long long next = 0xffffffffffffffff;
    unsigned long long now;
    struct timeval tv;

    gettimeofday(&tv,NULL);
    now = 1000 * tv.tv_sec + (tv.tv_usec / 1000);

    if( next > now ) { // We've not overrun, sync
        glFinish();
    } //else printf("Frame overrun 40ms by %llu\n", now - next );
    next = now + 40; // Almost 2 frames (20ms each - noting some inaccuracy in recording
}

// ----------------------------------------------------------------------------------
int throttle = 0;
unsigned long b, tmon_start, tmon_now;
//#define TIME_DEBUG
#ifdef TIME_DEBUG
#define TIME_REPORT(m) \
    gettimeofday(&tv,NULL); tmon_now = 1000000 * tv.tv_sec + tv.tv_usec; \
    if( throttle % 50 == 0 ) printf( "%s took %ld us\n", (m), (tmon_now - tmon_start)); tmon_start = tmon_now;
#else
#define TIME_REPORT(m) while(0){}
#endif

struct timeval tv;

// ----------------------------------------------------------------------------------
//
static void
process_frame( Flashlight *f )
{
    // Calculate the frametime in ms - used as a animation timing reference across the UI
    // frametime starts at 0 and for a long32, will run for 49 days before wrapping).
    gettimeofday(&tv,NULL);
    frame_time_ms = (1000L * (tv.tv_sec - initial_start_sec)) + (tv.tv_usec / 1000);

    TSFEventProcess p = {{ Process, NULL }};
    flashlight_event_publish( f,(FLEvent*)&p );  // When emu running, waits for frame done
    TIME_REPORT("\nProcess event");

    ui_read_joysticks();
    TIME_REPORT("Read joystick");

    _gles_flush_array( f->v->array );
    TIME_REPORT("Reload array buffer");

    flashlight_resize_ibo( f->v );
    flashlight_reload_dirty_ibos( f->v );
    TIME_REPORT("Reload ibo");

    glClear(GL_COLOR_BUFFER_BIT);
    TIME_REPORT("glClear");

    TSFEventRedraw e = {{ Redraw, NULL }};
    flashlight_event_publish( f,(FLEvent*)&e );
    TIME_REPORT("Redraw");

    eglSwapBuffers(fl->d->egl_display, fl->d->egl_surface);  // TODO Make flashlight
    TIME_REPORT("Swap Buffers");

    // Lock the GPU queue and the emulation state together to avoid screen buffer access race conditions.
    if( main_menu->emulator_running ) {
        do_glFinish();
    } else {
        // Scan for new devices periodically
        unsigned long let = ui_joystick_last_event_time();
        if( let && (let + device_scan_period_ms) < frame_time_ms ) {
            device_scan_period_ms = FRAME_PERIOD * 50 * (ui_get_joystick_count() > 0 ? 5 : 2 );
            ui_scan_for_joysticks();
        }
    }

    TSFEventVSyncDone v = {{ VSyncDone, NULL }}; 
    flashlight_event_publish( f,(FLEvent*)&v );
    TIME_REPORT("VsyncDoneEvent");

    tmon_start = b;
    TIME_REPORT("Frame");
    b = tmon_start;

    throttle++;
}

// ----------------------------------------------------------------------------------
//
#define LOAD_MESSAGE_FILE(l, f) \
    if( locale_load_messages( (l), (f) ) == 0 ) printf( "Load of language file %s failed\n", (f) );

static void
initialise_locale()
{
    locale_init();

    LOAD_MESSAGE_FILE( LANG_EN, "messages_en.msg" );
    LOAD_MESSAGE_FILE( LANG_DE, "messages_de.msg" );
    LOAD_MESSAGE_FILE( LANG_FR, "messages_fr.msg" );
    LOAD_MESSAGE_FILE( LANG_ES, "messages_es.msg" );
    LOAD_MESSAGE_FILE( LANG_IT, "messages_it.msg" );

    if( locale_select_language( LANG_EN ) == 0 ) printf( "Setting english defualt failed\n" );
}

// ----------------------------------------------------------------------------------
// disable blinking cursor on non X11 build with echo 0 > /sys/class/graphics/fbcon/cursor_blink
static void
disable_cursor()
{
    // Stop blinking cursor
    int ffd = open("/sys/class/graphics/fbcon/cursor_blink", O_RDWR, 0);
    if ( ffd >= 0 ) {
        write(ffd, "0\n", 2 );
        close(ffd);
    }

#ifdef HDMI_TEST
    ffd = open("/sys/class/hdmi/hdmi/attr/rgb_only", O_RDWR, 0);
    if ( ffd >= 0 ) {
        write(ffd, "1\n", 2 );
        close(ffd);
    }
#endif
}

// ----------------------------------------------------------------------------------
// 
int
main( int argc, char **argv ) 
{
    FLShader *shader;
    int test_mode = 0;

    disable_cursor();

    // Initialise the frame time before initialising any managers
    // Remember the "current time" so we can offset frame_time_ms to begin at 0.
    gettimeofday(&tv,NULL);
    initial_start_sec = tv.tv_sec - 1;

    // Do not start at ZERO! Zero is a special "time" which means no period. For example,
    // joystick "last_event_time" - if zero, there was no "lsat event", so the global time
    // used to set last_event_time must never be zero.
    frame_time_ms = 1 + (tv.tv_usec / 1000);

    fl = flashlight_init();
    if( fl == NULL ) {
        exit(1);
    }
    
    flashlight_screen_init( fl, WIDTH, HEIGHT );

    printf("Load shaders\n");
    flashlight_shader_set_path( get_path( "/usr/share/the64/ui/shaders/" ) );
    shader = flashlight_shader_load_all( fl, "vertex.glsl", "fragment.glsl" );
    if( shader == NULL ) {
        exit(1);
    }

    printf("Setup display\n");
    glClearColor(122.0f/256, 120.0f/256, 248.0f/256, 1.0);
    flashlight_vertice_assign_vertex_attribute( fl->v, fl->s, "a_position", "a_tex_position", "a_flag" );

    flashlight_create_vbo( fl->v );
    flashlight_create_ibo( fl->v );

    initialise_locale();

    // Initialise input devices
    printd("init joysticks\n");
    ui_init_joysticks( fl );
    device_scan_period_ms = FRAME_PERIOD * 50 * ( ui_get_joystick_count() > 0 ? 5 : 1 );

    printd("Init sound\n");
    ui_sound_init();
    flashlight_sound_volume(99);

    // Set up fonts
    printd("Init fonts\n");
    fonts_t *fonts = font_init( fl );

    printd("Init main menu\n");
    main_menu = mainmenu_init( fl, fonts, test_mode );
    if( main_menu == NULL ) {
        return -1;
    }

    // Flush internal mali array buffer
    _gles_flush_array( fl->v->array );

    // Make sure element array buffer is ready to go
    flashlight_resize_ibo( fl->v );

    if( !test_mode ) {
        if( settings_load( fl ) < 0 ) {
            // Force language selection -  Only done the first time, when there are no settings to load
            TSFUserEvent e = { {UserEvent, (void*)0xffff}, OpenLanguageSelection };
            flashlight_event_publish( fl,(FLEvent*)&e );
        }
    }

    flashlight_event_subscribe( fl, (void*)0xffff, ui_event, FLEventMaskAll );

    running = 1;
    while( running ) {
        process_frame( fl );
    }

    ui_sound_deinit();

    if(      shutdown_mode == QUIT_SHUTDOWN ) power_off();
    else if( shutdown_mode == QUIT_REBOOT   ) power_restart();
    else power_off();

    return 0;
}

// ----------------------------------------------------------------------------------
//
static void
ui_event( void *na, FLEvent *event )
{
    switch( event->type ) {
        case Keyboard:
            {
                int key = ((TSFEventKeyboard*)event)->key;
                int pressed = ((TSFEventKeyboard*)event)->is_pressed;

                if( key && pressed && (key == KEY_POWER) ) {
                    TSFEventQuit e = { {Quit, NULL}, QUIT_SHUTDOWN };
                    flashlight_event_publish( fl,(FLEvent*)&e );
                }
            }
            break;
        case PowerOff:
            {
                TSFEventPowerOff *pe = (TSFEventPowerOff*)event;
                running = 0;
                shutdown_mode = pe->type; // QUIT_REBOOT or QUIT_SHUTDOWN
            }
            break;
        default:
            break;
    }
}

// ----------------------------------------------------------------------------------

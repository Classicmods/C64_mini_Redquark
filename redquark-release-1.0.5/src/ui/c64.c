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

#include "mainmenu.h"
#include "machine.h"
#include "resources.h"
#include "vdrive/vdrive-internal.h"
#include "diskimage.h"
#include "cartridge.h"
#include "attach.h"
#include "tape.h"
#include "c64.h"
#include "kbdbuf.h"
#include "emucore.h"
#include "sid/sid.h"
#include "user_events.h"
#include "sem.h"
#include "emucore.h"
#include "selector.h"
#include "path.h"
#include "settings.h"
#include "ui_joystick.h"
#include "uitraps.h"
#include "usb.h"
#include "sound.h"
#include "event.h"
#include "tsf_compress.h"

//#define DEBUG 
//

#define BASIC_DISK_IMAGE_NAME "THEC64-drive8.d64"

#define DISK_INDICATOR_ENABLED

#define WIDTH 1280
#define HEIGHT 720

#define TEX_C64_WIDTH  384
#define TEX_C64_HEIGHT 240

#define CRT_WiS  ((TEX_C64_WIDTH * HEIGHT) / TEX_C64_HEIGHT) // Square pixels
#define CRT_WiP  ((int)((float)CRT_WiS * 0.9365))    // PAL Width Pixels
#define CRT_WiN  ((int)((float)CRT_WiS * 0.75  ))    // NTSC (old) Width Pixels

#define CRT_HMiS (( WIDTH - CRT_WiS ) / 2)
#define CRT_HMiP (( WIDTH - CRT_WiP ) / 2)
#define CRT_HMiN (( WIDTH - CRT_WiN ) / 2)

#define PP_X0      0 + CRT_HMiS
#define PP_X1   1280 - CRT_HMiS

#define EU_X0      0 + CRT_HMiP
#define EU_X1   1280 - CRT_HMiP
#define US_X0      0 + CRT_HMiN
#define US_X1   1280 - CRT_HMiN

#define FRAME_PERIOD     20 // In ms
extern unsigned long frame_time_ms;

static FLVertice  c64_verticies[] = {
    { EU_X0,   0,   0, 240, 0.7 },
    { EU_X0, 720,   0,   0, 0.7 },
    { EU_X1,   0, 384, 240, 0.7 },
    { EU_X1, 720, 384,   0, 0.7 },

#ifdef DISK_INDICATOR_ENABLED
    { EU_X0 - (30 + 24), 690 - 22, 434,  22, 0.2 },
    { EU_X0 - (30 + 24), 690 -  0, 434,   0, 0.2 },
    { EU_X0 - (30 +  0), 690 - 22, 456,  22, 0.2 },
    { EU_X0 - (30 +  0), 690 -  0, 456,   0, 0.2 },
#endif
};

#define SELF_COMPRESS // Letting VICE decompress leaves vice.xxx files in /tmp

static int           drive_led = 0;
static unsigned long drive_led_off_time_ms = 0;

static int autostarting = 0;
static int c64_frame_run_started = 0;

static void change_keyboard( C64Manager *g, language_t lang );

static void c64_manager_event( void *self, FLEvent *event );
static void connect_game_ports( C64Manager *g );
static void change_display_mode( C64Manager *g, display_mode_t mode );
static void stop_game( C64Manager *g );

static int configure_storage_for_BASIC();
static void detatch_usb_disk_image();

// ----------------------------------------------------------------------------------
//
static FLVerticeGroup *
add_image_quad( Flashlight *f, FLTexture *tx ) 
{
    int len = sizeof(c64_verticies) / sizeof(FLVertice);

    FLVertice vi[ 4 ];
    FLVerticeGroup *vgb = flashlight_vertice_create_group(f, 1);

    // Make sure the screen asset atlas texture is loaded
    FLTexture *atx = flashlight_load_texture( f, get_path("/usr/share/the64/ui/textures/menu_atlas.png") );
    if( atx == NULL ) { printf("Error atx\n"); exit(1); }
    flashlight_register_texture    ( atx, GL_NEAREST );
    flashlight_shader_attach_teture( f, atx, "u_DecorationUnit" );

    int tw = tx->width;
    int th = tx->height;

    int i,j, w, h;
    for(i = 0; i < len; i += 4 ) {
        for(j = 0; j < 4; j++ ) {
            // Swap between emulated texture and atlas width/height
            w = ( c64_verticies[i + j].u == 0.2f ) ? atx->width  : tw;
            h = ( c64_verticies[i + j].u == 0.2f ) ? atx->height : th;

            vi[j].x = X_TO_GL(WIDTH,  c64_verticies[i + j].x);
            vi[j].y = Y_TO_GL(HEIGHT, c64_verticies[i + j].y);
            vi[j].s = S_TO_GL(w,      c64_verticies[i + j].s);
            vi[j].t = T_TO_GL(h,      c64_verticies[i + j].t);
            vi[j].u = c64_verticies[i + j].u;
        }

        flashlight_vertice_add_quad2D( vgb, &vi[0], &vi[1], &vi[2], &vi[3] );
    }
    return vgb;
}

// ----------------------------------------------------------------------------------
// Initialise in-game shot display
C64Manager *
c64_manager_init( Flashlight *f, fonts_t *fonts  )
{
    C64Manager *g = malloc( sizeof(C64Manager) );
    if( g == NULL ) {
        return NULL;
    }
    memset(g, 0, sizeof(C64Manager) );

    g->f = f;

    FLTexture *tx = flashlight_create_null_texture( f, 384,240, GL_RGBA );

    if( tx == NULL ) { printf("Error tx\n"); exit(1); }
    g->texture = tx;

    flashlight_register_texture( g->texture, GL_NEAREST ); // The scaling filter used here is overridden in 'change_display_mode'

    g->maliFLTexture = mali_egl_image_get_texture( f->d->egl_display, g->texture->width, g->texture->height );

    flashlight_shader_attach_teture( f, tx, "u_EmulatorUnit" );

    g->vertice_group = add_image_quad(f, g->texture);
    g->fade_level    = 100;

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

    g->screen = start_c64();

    g->uniforms.texture_fade       = flashlight_shader_assign_uniform( f, "u_fade"               );
    g->uniforms.translation_matrix = flashlight_shader_assign_uniform( f, "u_translation_matrix" );

    flashlight_event_subscribe( f, (void*)g, c64_manager_event, FLEventMaskAll );

    g->focus = 0;
    g->display_on = 0;
    g->igmenu = ig_menu_init( f, fonts );

    change_keyboard( g, LANG_EN_UK );

    change_display_mode( g, PIXEL_CRT );

    return g;
}

// ----------------------------------------------------------------------------------

void
c64_finish( C64Manager **g ) {
    free(*g);
    *g = NULL;
}

// ----------------------------------------------------------------------------------
//
static void
set_target_offset( C64Manager *g , int x, int y )
{
    y = y; // Stop warnings
    g->target_offset_x   = S_TO_GL(HEIGHT, x);
    g->target_offset_x_t = frame_time_ms + FRAME_PERIOD * 10;
}

// ----------------------------------------------------------------------------------
//
static void
process_translation( C64Manager *g )
{
    if( g->target_offset_x != g->offset_x ) {
        if( g->target_offset_x_t < frame_time_ms + FRAME_PERIOD ) g->offset_x = g->target_offset_x;
        else g->offset_x += (FRAME_PERIOD * (g->target_offset_x - g->offset_x)) / (g->target_offset_x_t - frame_time_ms);
    }
}

// ----------------------------------------------------------------------------------
#ifdef TIME_DEBUG
#include <sys/time.h>
extern int throttle;
static unsigned long cmon_start, cmon_now;
static struct timeval cmon_tv;
#define TIME_REPORT(m) \
    gettimeofday(&cmon_tv,NULL); cmon_now = 1000000 * cmon_tv.tv_sec + cmon_tv.tv_usec; \
    if( throttle % 50 == 0 ) printf( "%s took %ld us\n", (m), (cmon_now - cmon_start)); cmon_start = cmon_now;
#else
#define TIME_REPORT(m) while(0){}
#endif
// ----------------------------------------------------------------------------------
//
void
c64_mute( C64Manager *g )
{
    sound_clear(); // Clear current buffer - Must be done first!

    resources_set_int("SoundVolume", 0 );
    sound_set_warp_mode(1);

    g->silence_enabled = 1;
}

// ----------------------------------------------------------------------------------
//
void
c64_unmute( C64Manager *g )
{
    sound_clear(); // Clear current buffer - Must be done first!

    resources_set_int("SoundVolume", 99 );
    sound_set_warp_mode(0);

    g->silence_enabled = 0;
}

// ----------------------------------------------------------------------------------
//
void
c64_silence_for_frames( C64Manager *g, int frames )
{
    g->silence_for_frames = frames;

    c64_mute( g );
}

// ----------------------------------------------------------------------------------
static int
c64_process( C64Manager *g )
{
    //static int reset_sound_count = 0;

    process_translation( g );

    if( g->silence_for_frames ) {
        g->silence_for_frames--;
        if( g->silence_for_frames < 5 ) {
            if( g->silence_enabled ) sound_set_warp_mode(0);
            g->silence_enabled = 0;

            resources_set_int("SoundVolume", 20 * (5 - g->silence_for_frames) );
        }
    }

    if( autostarting && !autostart_in_progress() ) {
        //printf("\n**** AUTOSTART DONE ***\n\n");

        autostarting = 0;

        g->display_on = 1;
        g->display_on_timer_ms = 0; // Cancel timer

        c64_unmute( g );

        TSFUserEvent e = { {UserEvent, (void*)g}, AutoloadComplete };
        flashlight_event_publish( g->f,(FLEvent*)&e );
    }

    if( g->display_on_timer_ms > 0 && g->display_on_timer_ms < frame_time_ms ) {
        g->display_on_timer_ms = 0;
        //printf("display on alarm.\n");
        g->display_on = 1;

        c64_unmute( g );

        TSFUserEvent e = { {UserEvent, (void*)g}, AutoloadComplete };
        flashlight_event_publish( g->f,(FLEvent*)&e );
    }

    // If all joysticks have been unplugged, exit game. The main UI will handle
    // re-detection of joysticks plugged in.
    if( ui_get_joystick_count()  == 0 ) {
        stop_game( g );
    }

    return 1;
}

// ----------------------------------------------------------------------------------
// 
static void
change_display_mode( C64Manager *g, display_mode_t mode )
{
    int x0,x1;
    GLint filter;

    if( mode & _US ) { 
        // Accurate US pixel size
        x0 = US_X0;
        x1 = US_X1;
    } else if( mode & _EU ) {
        // Accurate EU pixel size
        x0 = EU_X0;
        x1 = EU_X1;
    } else {
        // Pixel Perfect size (square pixels)
        x0 = PP_X0;
        x1 = PP_X1;
    }
    if( mode & _CRT ) {
        filter = GL_LINEAR;
    } else {
        filter = GL_NEAREST;
    }
#define PP_SHARP 0.7
#define PP_CRT   0.8

    // Alter quad size and switch fragment shader handler to add scanlines, if necessary
    FLVertice *v[4];
    flashlight_vertice_get_quad2D( g->vertice_group, 0, &v[0], &v[1], &v[2], &v[3] );
    v[0]->x = v[1]->x = X_TO_GL(WIDTH, x0);
    v[2]->x = v[3]->x = X_TO_GL(WIDTH, x1);
    v[0]->u = v[1]->u = v[2]->u = v[3]->u = (mode & _CRT) ? PP_CRT : PP_SHARP;

#ifdef DISK_INDICATOR_ENABLED
    flashlight_vertice_get_quad2D( g->vertice_group, 1, &v[0], &v[1], &v[2], &v[3] );
    v[0]->x = v[1]->x = X_TO_GL(WIDTH, x1 - (30 + 24));
    v[2]->x = v[3]->x = X_TO_GL(WIDTH, x1 -  30 );
#endif

    // Change filter
    glActiveTexture( GL_TEXTURE0 + g->texture->texture_unit );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    g->display_mode = mode;
}

// ----------------------------------------------------------------------------------
//
static void
change_keyboard( C64Manager *g, language_t lang )
{
    switch (lang ) {
        case LANG_NONE:
        case LANG_EN:
        case LANG_EN_UK: resources_set_string("KeymapSymFile", "theC64-sym-UK.vkm" ); break;
        case LANG_EN_US: resources_set_string("KeymapSymFile", "theC64-sym-US.vkm" ); break;
        case LANG_DE:    resources_set_string("KeymapSymFile", "theC64-sym-DE.vkm" ); break;
        case LANG_FR:    resources_set_string("KeymapSymFile", "theC64-sym-FR.vkm" ); break;
        case LANG_ES:    resources_set_string("KeymapSymFile", "theC64-sym-ES.vkm" ); break;
        case LANG_IT:    resources_set_string("KeymapSymFile", "theC64-sym-IT.vkm" ); break;
        default: break;
    }
}

// ----------------------------------------------------------------------------------
// Simulate the releasing of the joystick and buttons 
static void
send_joystick_release()
{
    joystick_queue_event( 1, 0 );
    joystick_queue_event( 2, 0 );
    joystick_queue_event( 3, 0 );
    joystick_queue_event( 4, 0 );
}

// ----------------------------------------------------------------------------------
//
static void
launch_game( C64Manager *g )
{
    char *game_filename = NULL;

    if( ui_is_paused() ) {
        // Make sure pause is released. Probably not necessary.
        ui_pause_emulation( 0 );
    }

    c64_mute( g ); // Mute emluator sound

    glClearColor(0, 0, 0, 1.0); // Screen is black

    // Tell everyone that the fullscreen mode has been claimed
    TSFUserEvent e = { {UserEvent, (void*)g}, FullScreenClaimed };
    flashlight_event_publish( g->f,(FLEvent*)&e );

    game_t *game = g->game;

    // Transfer the mapping of each gameport into the emulator sub-system and initialise.
    joystick_clone_port_map( 1, game->port_map[0] );
    joystick_clone_port_map( 2, game->port_map[1] );
    joystick_clone_port_map( 3, game->port_map[2] );
    joystick_clone_port_map( 4, game->port_map[3] );

    connect_game_ports( g ); // Takes care of which JS is primary, and connects it to the correct port

    if( game->is_ntsc ) switch_to_model_ntsc(); // Also ensures EXACTLY 50Hz sound generation (Yes 50!)
    else                switch_to_model_pal();  // Also ensures EXACTLY 50Hz sound generation

    // Set vertical center position of emulated screen. The 720p screen clips top and bottom borders
    // losing 16 lines top and bottom. This allows c64 screen to be repositioned  +/-16
    ui_set_vertical_shift( game->vertical_shift ); 

    file_system_detach_disk(-1);
    
    resources_set_int("CartridgeReset", 0 );

    cartridge_detach_image(0); // Detach  main slot

    int autoloading = 0;

    g->display_on_timer_ms = frame_time_ms + (FRAME_PERIOD * 50 * 5);

    do {
        if( game->filename[0] != '\0' ) {

            game_filename = game->filename;
            int flen = strlen(game_filename );

            // Handle compressed images
            //
            if( flen > 3 && 
                    game->filename[flen-3] == '.' &&
                    game->filename[flen-2] == 'g' &&
                    game->filename[flen-1] == 'z' ) {

#ifdef SELF_COMPRESS   // We either decompress ourselves, or let VICE do it - though vice leaves tmp files...
                game_filename = "/tmp/snapshot"; // A generic name
                
                int dr = tsf_decompress_file( game->filename, game_filename );
                if( dr < 0 ) {
                    autoloading = 0;
                }
#endif
                flen -= 3; // Adjust len so the following extension checks work
            }

            // XXX From now on, we continue to check the original game->filename XXX
            // XXX for extension, BUT we load game_filename                      XXX

            //g->sound_off_timer_ms  = frame_time_ms + (FRAME_PERIOD * 50 * 1);

            // Detect crt file being loaded, and attach it
            //
            // set initial RAM state to select the correct program from a multiload
            // cartridge (sepcially hacked for the64). State will come from the tsg file.

            if( flen > 4 && 
                    game->filename[flen-4] == '.' &&
                    game->filename[flen-3] == 'c' &&
                    game->filename[flen-2] == 'r' &&
                    game->filename[flen-1] == 't' ) {

                if( game->cartridge_title_id > 0 ) {
                    // Set inital RAM memory state so (patched) cartridge knows which
                    // title to auto-pick from the menu.
                    resources_set_int("RAMInitStartValue", game->cartridge_title_id );
                }

                cartridge_attach_image(0, game_filename );

                autoloading = 0;

                g->display_on_timer_ms = frame_time_ms + (FRAME_PERIOD * 50 * 4);
                //g->sound_on_timer_ms = frame_time_ms + (FRAME_PERIOD * 50 * 6.5);
            } else 
#ifdef QUICKLOAD_VSF
                // This ONLY works if the machine has finished resetting.
                // Therefore, if we're switching machine types, or atach/detach cartridge which MUST be
                // followed by a reset, then the the snap load gets reset....
                // We could do a check for cart/machine change and optionally quickload - but then the
                // loading experience is inconsistent...
                if( flen > 3 && 
                        game->filename[flen-4] == '.' &&
                        game->filename[flen-3] == 'v' &&
                        game->filename[flen-2] == 's' &&
                        game->filename[flen-1] == 'f' ) {

                    resources_set_int("RAMInitStartValue", 0 );

                    load_snapshot( game_filename );

                    g->display_on = 1;
                    autoloading = 1;

                    // We're not loading/couldn't load a file, so pretend like we just did....
                    TSFUserEvent e = { {UserEvent, (void*)g}, AutoloadComplete };
                    flashlight_event_publish( g->f,(FLEvent*)&e );
                } else
#endif
                { // Autoload
                    resources_set_int("RAMInitStartValue", 0 );

                    // Configure truedrive if needed
                    resources_set_int("AutostartHandleTrueDriveEmulation", game->needs_truedrive ? 1 : 0 );

                    //g->sound_on_timer_ms = frame_time_ms + (FRAME_PERIOD * 50 * 3.1);

                    // Incase autoload fails...
                    g->display_on_timer_ms = frame_time_ms + (FRAME_PERIOD * 50 * 8);

                    if (autostart_autodetect( game_filename, NULL, 0, 0) < 0) {
                        // Error
                        fprintf(stderr, "could not autostart image [%s]\n", game->filename );
                    } else {
                        autoloading = 1;
                        autostarting = 1;
                    }
                }
        } else {
            // We had no filename .... turn sound on in 2 seconds (in case it is off).
            //g->sound_on_timer_ms = frame_time_ms + (FRAME_PERIOD * 50 * 2);
            autoloading = 0;
        }
    } while(0);

    if( game->is_basic ) configure_storage_for_BASIC();

    if( !autoloading ) {
        // RESET THE 64
        machine_trigger_reset(MACHINE_RESET_MODE_HARD);
        // Display is enabled by display_on_timer_ms
    }

    game->duration = 0;

    e.user_type = EmulatorStarting;
    flashlight_event_publish( g->f,(FLEvent*)&e );
} 

// ----------------------------------------------------------------------------------
static void
c64_manager_event( void *self, FLEvent *event )
{
    C64Manager *g = (C64Manager *)self;

    switch( event->type ) {
        case Navigate:
            {
                //if( g->igmenu->active ) break; // Ingame menu is active
                if( ! g->got_input ) break; // Ingame menu, virtual keyboad, load/save menu is active
                if( !g->display_on ) break;    // Disable input until we are running...

                TSFEventNavigate *nav = (TSFEventNavigate*)event;

                uint32_t joy_value = nav->direction;
                jstype_t type      = nav->type;
                //int      id        = nav->id; // Not used

                if( type == JOY_JOYSTICK ) {
                    // Map virtual port to physical (may be 1:1 or ports 1 & 2 could be swapped).
                    int port = g->port_remap[ nav->vport ];
                    joystick_queue_event( port, joy_value );
                }
            }
            break;
        case Keyboard:
            {
                if( ! g->got_input ) break; // Ingame menu, virtual keyboad, load/save menu is active
                if( !g->display_on ) break;    // Disable input until we are running...

                int key = ((TSFEventKeyboard*)event)->key;
                int pressed = ((TSFEventKeyboard*)event)->is_pressed;

                keyboard_queue_key( key, pressed );
            }
            break;
        case Redraw:
            {
                if( !g->focus || !g->display_on ) break;

                FLVerticeGroup *cg = g->vertice_group;

                glUniformMatrix4fv( g->uniforms.translation_matrix,1, GL_FALSE, flashlight_translate( g->offset_x, 0.0, 0.0 ));
                glUniform1f( g->uniforms.texture_fade, 1.0 );
                flashlight_shader_blend_off(g->f);

#ifdef DISK_INDICATOR_ENABLED
                int ind_len = (g->game->needs_driveicon && drive_led && (drive_led_off_time_ms > frame_time_ms)) ? cg->indicies_len : cg->indicies_len - 4;
#else
                int ind_len = cg->indicies_len;
#endif

                glDrawElements(GL_TRIANGLES, ind_len, GL_UNSIGNED_SHORT, flashlight_get_indicies(cg) );
            }
            break;
        case Process:
            {
                if( !g->focus) break;
                c64_process( g );
            }
            break;

        case VSyncDone:
            if( !g->focus) break;

            // Only wait for a C64 frame completion if we've notified one to start
            if( c64_frame_run_started ) {
                semaphore_wait_for( SEM_FRAME_DONE ); // Wait for emu to finish (pre-vsync in vsyncarch.c)
            }

            if( ui_is_paused() ) {
                semaphore_notify( SEM_START_NEXT_FRAME );
                break;
            }

            // Frame done, sync done, so transfer the C64 frame into the texture
            mali_egl_image_sub(g->maliFLTexture, 0, 0, 384, 240, g->screen);
            semaphore_notify( SEM_START_NEXT_FRAME );
            c64_frame_run_started = 1;

            g->game->duration += 20; 

            break;

        case FocusChange:
            {
                TSFFocusChange *fce = (TSFFocusChange*)event;
                if( fce->who == g ) {

                    if( g->focus ) break;
                    
                    mainmenu_selector_push( g );
                    mainmenu_selector_set_position( -40, -40, 20, 20 );

                    joystick_clear_queues(); // Also clears vkeyboard queue

                    g->focus = 1; // We have the focus
                    g->got_input = 1; // We've got control of the joystick

                    launch_game( g );
                    
                } else {
                    // We've lost focus
                    if( g->focus ) {
                        mainmenu_selector_pop( g );

                        TSFUserEvent e = { {UserEvent, (void*)g}, EmulatorFinishing };
                        flashlight_event_publish( g->f,(FLEvent*)&e );

                        e.user_type = FullScreenReleased;
                        flashlight_event_publish( g->f,(FLEvent*)&e );

                        // Detatch tape, disk or cartridge
                        tape_image_detach(1);
                        file_system_detach_disk(-1); // All disks
                        cartridge_detach_image(0);

                        if( g->game->is_basic ) detatch_usb_disk_image();;

                        delete_tempory_files();
                    }

                    g->focus = 0;
                    g->display_on = 0;
                    autostarting = 0;
                    c64_frame_run_started = 0; // Stop waiting for frame done etc
                }
            }
            break;
        case UserEvent:
            {
                TSFUserEvent *ue = (TSFUserEvent*)event;
                switch(ue->user_type) {
                    case SelectorPop:
                        {
                            // If the selector has just been handed back to the c64 controller from
                            // the ingame menu, then set the selector offscreen (bottom) but maintain
                            // the ingame menu selector's X coord and size. When the ingame menu is
                            // re-activated, the selector will then come into view straight up...
                            // 
                            SelectorParams *param = (SelectorParams *)ue->data;
                            if( param->owner == g->igmenu->menu && mainmenu_selector_owned_by( g ) ) {
                                mainmenu_selector_set_target_position( (int)param->target_x, -45, 
                                        (int)param->target_size_x, (int)param->target_size_y, 9 );
                            }
                        }
                        break;
                       
                   case Pause: // Pause has been requested/released
                       {
                           int s = FL_EVENT_DATA_TO_INT(ue->data);

                           if(s) c64_mute( g ); // Mute emluator sound
                           else  c64_silence_for_frames( g, 6 ); // unmute fully after 10 frames, ramping up for the last 5.
                           ui_pause_emulation( s );
                       }
                       break;

                    case InputClaim:
                        {
                            int claimed = FL_EVENT_DATA_TO_INT(ue->data);

                            g->got_input = !claimed;

                            // If something has claimed the input, simulate the release of
                            // the joystick as far as the C64 is concerned.
                            if( claimed ) send_joystick_release();
                        }
                        break;

                    case VirtualKeyboardChange:
                        {
                            int open = FL_EVENT_DATA_TO_INT(ue->data);

                            // Only pixel-perfect and EU widths need the screen shifting
                            if( ! (g->display_mode & _US) ) {
                                if( g->display_mode & _PIXEL ) 
                                    set_target_offset( g, open ? -88 : 0, 0 ); // Pixel Perfext (square pix)
                                else
                                    set_target_offset( g, open ? -60 : 0, 0 ); // Accurate pixels
                            }
                        }
                        break;
                }
            }
            break;
        case Setting:
            {
                TSFEventSetting *es = (TSFEventSetting*)event;
                switch( es->id ) {
                    case DisplayMode: 
                        change_display_mode( g, FL_EVENT_DATA_TO_TYPE(display_mode_t, es->data) ); break;

                    case KeyboardRegion: 
                        change_keyboard(g, FL_EVENT_DATA_TO_TYPE(language_t, es->data)); break;

                    default: break;
                }
            }
            break;

        case Quit: // We're exiting, so do any cleanup like terminating threads
            {
                if( !g->focus ) break;

                g->focus = 0;
                g->display_on = 0;
                autostarting = 0;
                c64_frame_run_started = 0; 

                stop_c64();
            }

        default:
            break;
    }
}

// ----------------------------------------------------------------------------------
//
static void
stop_game( C64Manager *g )
{
    TSFUserEvent e = { {UserEvent, (void*)g}, EmulatorExit };
    flashlight_event_publish( g->f,(FLEvent*)&e );
}

// ----------------------------------------------------------------------------------
// This makes sure the joystick that selected the game is "wired" up to the primary
// C64 port for this game. Whatever joystick was by default allocated to the game's
// primary port (ie 1 or 2), is swapped with the joystick that was used to select
// the game.
static void
connect_game_ports( C64Manager *g )
{
    // Initialise map 1:1
    g->port_remap[1] = 1;
    g->port_remap[2] = 2;
    g->port_remap[3] = 3;
    g->port_remap[4] = 4;

    int selected_js_port = ui_joystick_get_port_for_id( g->controller_primary );

#ifdef DEBUG
    printf("Selected JS port mapping is to port %d. Need it to be to port %d\n", selected_js_port, g->game->port_primary );
#endif

    if( selected_js_port > 0 && selected_js_port != g->game->port_primary ) {
        // JS used to start game is not mapped to the primary port for this game.
        // Alter our translation map to account for this
        g->port_remap[ selected_js_port ]      = g->game->port_primary;
        g->port_remap[ g->game->port_primary ] = selected_js_port;
    }

#ifdef DEBUG
    printf("Primary port for game is %d\n", g->game->port_primary );
    printf("Selected joystick id  is %d\n", g->controller_primary );
    int i;
    for( i = 1; i <= 4; i++ ) {
        printf( "Default port %d -> Physical Port %d\n", i, g->port_remap[i] );
    }
#endif
}

// ----------------------------------------------------------------------------------
void 
ui_display_drive_led(int drive_number, unsigned int pwm1, unsigned int pwm2)
{
#ifdef DISK_INDICATOR_ENABLED
    static int LED = 0;

    uint32_t status = 0;

    if( pwm1 > 100 ) status |= 1;
    if( pwm2 > 100 ) status |= 2;

    if(  status && !LED ) { drive_led = 1; }
    if( !status &&  LED ) { drive_led = 0; }

    if( drive_led ) drive_led_off_time_ms = frame_time_ms + FRAME_PERIOD * 12; // 1/4 second

    LED = status;
#endif
}


// ----------------------------------------------------------------------------------
// dst must point to a char array long enough for the name
static void
create_disk_image_filename( char *dst )
{
    int l = strlen(USB_MOUNT_POINT);
    memcpy( dst, USB_MOUNT_POINT, l );
    dst[l] = '/';
    strcpy( dst + l + 1, BASIC_DISK_IMAGE_NAME );
}

// ----------------------------------------------------------------------------------
//
static int
attach_usb_disk_image()
{
    int  ret = 0;
    char fpath[256];

    do {
        create_disk_image_filename( fpath );

        if( filesystem_test_for_file( fpath ) == 0 ) break;
        if( file_system_attach_disk( 8, fpath ) < 0 ) break;

        ret = 1;
    } while(0);

    return ret;
}

// ----------------------------------------------------------------------------------
//
static void
detatch_usb_disk_image()
{
    file_system_detach_disk(-1); // All disks
    sync();
    unmount_usb_stick();
}

// ----------------------------------------------------------------------------------
// 
static int
configure_storage_for_BASIC()
{
    char fpath[256];

    do {
        // try and mount the USB stick
        if( mount_usb_stick( 1 ) == 0 ) break; // 1 == readwrite

        // Look for, and attach a THEC64-drive8.d64 image (if one exists)
        if( attach_usb_disk_image() ) return 1;

        // Instead create a disk image
        //
        create_disk_image_filename( fpath );

        if( vdrive_internal_create_format_disk_image( fpath, "THEC64,01", DISK_IMAGE_TYPE_D64 ) == 0 ) {

            sync();

            // Now that has been created, attach it
            if( attach_usb_disk_image() ) return 1;
        }

        // it has all failed. Unmount
        unmount_usb_stick();

    } while(0);

    // Mount a readonly disc (it is compressed)
    //
    file_system_attach_disk( 8, get_path("/usr/share/the64/ui/data/blank.d64.gz") );

    return 0;
}

// ----------------------------------------------------------------------------------

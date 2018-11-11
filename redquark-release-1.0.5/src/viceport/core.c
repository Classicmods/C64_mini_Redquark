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

//#include <dlfcn.h>

#include "videoarch.h"
#include "machine.h"
#include "sem.h"
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>

#include "emucore.h"

// Vice includes
#include "c64/c64model.h"
#include "c64/c64.h"
#include "sid/sid.h"
#include "archapi.h"
#include "maincpu.h"
#include "drive/drive.h"
#include "sysfile.h"
#include "gfxoutput.h"
#include "init.h"
#include "resources.h"
#include "log.h"
#include "initcmdline.h"
#include "video.h"
#include "sound.h"

#define PAL_LINES           C64_PAL_SCREEN_LINES        // 312
#define PAL_LINE_CLKS       C64_PAL_CYCLES_PER_LINE     // 63
#define PAL_FRAME_CLKS      (PAL_LINES * PAL_LINE_CLKS) // 312 * 63
#define PAL_CLKS_SECOND     (PAL_FRAME_CLKS * 50)       // 312 * 63 * 50

#define NTSC_LINES          C64_NTSC_SCREEN_LINES        // 263
#define NTSC_LINE_CLKS      C64_NTSC_CYCLES_PER_LINE     // 65
#define NTSC_FRAME_CLKS     (NTSC_LINES * NTSC_LINE_CLKS) // 263 * 65
#define NTSC_CLKS_SECOND    (NTSC_FRAME_CLKS * 50)       // 263 * 65 * 50 // We ALWAYS display at 50Hz

int console_mode = 0;
int video_disabled_mode = 0;

int main_program(int argc, char **argv);
void c64_exit(void);

static pthread_t emuthread;

// -------------------------------------------------------------------------------
static int
c64_init(int argc, char **argv)
{
    if( semaphore_init() < 0 ) {
        printf("Failed to init semaphores\n");
        return -1;
    }

    archdep_init(&argc, argv);

    if (atexit(c64_exit) < 0) {
        printf("atexit");
        return -1;
    }

    maincpu_early_init();
    machine_setup_context();
    drive_setup_context();
    machine_early_init();

    /* Initialize system file locator.  */
    sysfile_init(machine_name);
    
    gfxoutput_early_init();

    if (init_resources() < 0 || init_cmdline_options() < 0) {
        return -1;
    }
    
    /* Set factory defaults.  */
    if (resources_set_defaults() < 0) {
        printf("Cannot set defaults.\n");
        return -1;
    }

    /* Load the user's default configuration file.  */
    if (resources_load(NULL) < 0) {
        /* The resource file might contain errors, and thus certain
           resources might have been initialized anyway.  */
        if (resources_set_defaults() < 0) {
            archdep_startup_log_error("Cannot set defaults.\n");
            return -1;
        }
    }

    if (log_init() < 0) {
        archdep_startup_log_error("Cannot startup logging system.\n");
    }

    if (initcmdline_check_args(argc, argv) < 0) {
        return -1;
    }

    if (video_init() < 0) {
        return -1;
    }

    if (initcmdline_check_psid() < 0) {
        return -1;
    }

    // Prevent VICE from using the raw filesystem as a Disk
    resources_set_int("FileSystemDevice8",  0 );
    resources_set_int("FileSystemDevice9",  0 );
    resources_set_int("FileSystemDevice10", 0 );
    resources_set_int("FileSystemDevice11", 0 );

    resources_set_int("AutostartDelay", 3 ); // Use the default of 3
    resources_set_int("AutostartDelayRandom", 0 );

    resources_set_int("SidEngine", SID_ENGINE_RESID );
    resources_set_int("SidModel" , SID_MODEL_6581 );
    resources_set_int("SidResidSampling", 0); // Fast

    resources_set_int("MachineVideoStandard", MACHINE_SYNC_PAL ); // PAL

    if (init_main() < 0) {
        return -1;
    }

    resources_set_int("SoundVolume", 100 );
    resources_set_int("SoundOutput", 2);
    resources_set_int("SoundBufferSize", 1000);

    resources_set_int("RAMInitStartValue", 0 );

    resources_set_string("VICIIPaletteFile", "theC64-palette.vpl" );
    resources_set_int   ("VICIIExternalPalette", 1);

    initcmdline_check_attach();
}

// -------------------------------------------------------------------------------
//
void c64_exit(void)
{
    machine_shutdown();
    stop_c64();         // End thread
}

// -------------------------------------------------------------------------------
void
switch_to_model_ntsc()
{
    resources_set_int("MachineVideoStandard", MACHINE_SYNC_NTSC );

    // TheC64 always runs the display at 50Hz, so we have to do some magic to get 
    // the SID generating 882 samples (44100/50) per frame.
    //
    sound_set_machine_parameter(NTSC_CLKS_SECOND, NTSC_FRAME_CLKS);
}

// -------------------------------------------------------------------------------
void
switch_to_model_pal()
{
    resources_set_int("MachineVideoStandard", MACHINE_SYNC_PAL );

    // TheC64 always runs the display at 50Hz, so we have to do some magic to get 
    // the SID generating 882 samples (44100/50) per frame.
    //
    sound_set_machine_parameter(PAL_CLKS_SECOND, PAL_FRAME_CLKS);
}

// -------------------------------------------------------------------------------
//
static void *
the64loop( void *p )
{
    int ot;
    pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, &ot );

    maincpu_mainloop();
    return 0;
}

// -------------------------------------------------------------------------------
//
void
stop_c64()
{
    int e;
    if( (e = pthread_cancel( emuthread )) < 0 ) {
        printf("pthread_cancel failed with err %d\n", e );
    }
    // Wake up the main thread.
    semaphore_notify( SEM_FRAME_DONE );
}

// -------------------------------------------------------------------------------
//
unsigned char *
start_c64( )
{
    unsigned char *s = c64_create_screen();

    static char *av = "./the64";
    c64_init(1, &av);

    int iret = pthread_create( &emuthread, NULL, the64loop, (void*)NULL);
    if(iret)
    {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",iret);
        exit(1);
    }
    return s;
}

// -------------------------------------------------------------------------------

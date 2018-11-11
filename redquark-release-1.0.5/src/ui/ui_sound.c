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
// Sound management
//
//

#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <alsa/asoundlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "flashlight.h"

#include "path.h"

#define STEM "/usr/share/the64/ui/sounds"

static FLSoundChannel *c_music;
static FLSoundChannel *c_fx;
static FLSound         snd_music;
static FLSound         snd_flip;
static FLSound         snd_ding;
static FLSound         snd_menu_open;
static FLSound         snd_menu_close;

static void
_init_channel_fx()
{
    if( (c_fx = flashlight_sound_init_channel( "fx" )) == NULL ) {
        printf( "Failed to init_channel\n" );
        exit(1);
    }
}

static void
_init_channel_music()
{
    if( (c_music = flashlight_sound_init_channel( "music" )) == NULL ) {
        printf( "Failed to init channel\n" );
        exit(1);
    }
}

void
ui_sound_init()
{
    // For some reason, if we init ALSA and then deinit, then the mixer (specifically changing
    // the volume with snd_mixer_selem_set_playback_volume_all() does not work. Found this 
    // solution after starting the app (volume not working) stop and restart (volume working).
    // Haven't found out why this is, but this works! (not sure the flashlight_sound_volume(0) is needed
    // but I tested it like this so am leaving it!).
    _init_channel_fx();
    flashlight_sound_volume(0);
    flashlight_sound_deinit_channel( c_fx );

    // Initialise properly
    _init_channel_fx();
    _init_channel_music();

    flashlight_sound_load_wav( get_path( STEM "/flip.wav"),       c_fx,    &snd_flip       );
    flashlight_sound_load_wav( get_path( STEM "/ding.wav"),       c_fx,    &snd_ding       );
    flashlight_sound_load_wav( get_path( STEM "/menu_open.wav"),  c_fx,    &snd_menu_open  );
    flashlight_sound_load_wav( get_path( STEM "/menu_close.wav"), c_fx,    &snd_menu_close );
    flashlight_sound_load_wav( get_path( STEM "/menu.wav"),       c_music, &snd_music      );
}

void
ui_sound_deinit()
{
    flashlight_sound_deinit_channel( c_fx    );
    flashlight_sound_deinit_channel( c_music );
}

void
ui_sound_play_ding()
{
    //if( snd_ding.sound_channel == NULL ) {
    //    _init_channel_fx();
    //    snd_ding.sound_channel = c_fx;
    //}
    if( c_fx ) flashlight_sound_play( &snd_ding, SOUND_NO_FLAGS );
}

void
ui_sound_play_menu_open()
{
    if( c_fx ) flashlight_sound_play( &snd_menu_open, SOUND_NO_FLAGS );
}
    
void
ui_sound_play_menu_close()
{
    if( c_fx ) flashlight_sound_play( &snd_menu_close, SOUND_NO_FLAGS );
}

void
ui_sound_play_flip()
{
    //if( snd_flip.sound_channel == NULL ) {
    //    _init_channel_fx();
    //    snd_flip.sound_channel = c_fx;
    //}
    if( c_fx ) flashlight_sound_play( &snd_flip, SOUND_NO_FLAGS );
}

void
ui_sound_play_flip_delayed()
{
    //if( snd_flip.sound_channel == NULL ) {
    //    _init_channel_fx();
    //    snd_flip.sound_channel = c_fx;
    //}
    if( c_fx ) flashlight_sound_play_delay( &snd_flip, SOUND_NO_FLAGS, 80 );
}

void
ui_sound_play_music()
{
    //if( snd_music.sound_channel == NULL ) {
    //    _init_channel_music();
    //    snd_music.sound_channel = c_music;
    //}
    if( c_music ) flashlight_sound_play( &snd_music, SOUND_REPEAT );
}

void
ui_sound_stop_ding()
{
    if( c_fx ) flashlight_sound_stop( &snd_ding );
    //flashlight_sound_deinit_channel( c_fx );
    //snd_ding.sound_channel = NULL;
}

void
ui_sound_stop_flip()
{
    if( c_fx ) flashlight_sound_stop( &snd_flip );
    //flashlight_sound_deinit_channel( c_fx );
    //snd_flip.sound_channel = NULL;
}

void
ui_sound_stop_music()
{
    if( c_music ) flashlight_sound_stop( &snd_music );
    //flashlight_sound_deinit_channel( c_music ); // FIXME There is a difference between dropping the thread and closing the channel
    //snd_music.sound_channel = NULL;
}

void
ui_sound_wait_for_ding()
{
    if( c_fx ) flashlight_sound_finish_current_sound( &snd_ding );
}

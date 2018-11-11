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
#include "tsf_joystick.h"

#include "poweroff.h"
#include "mainmenu.h"
#include "user_events.h"
#include "ui_sound.h"

static void mainmenu_event( void *self, FLEvent *event );
static void focus_to_gamelist( MainMenu *m );

static MainMenu     *mm = NULL;

void _set_selector_owner( void* o ) { mm = (MainMenu*)o; }

// ----------------------------------------------------------------------------------
// Initialise in-game shot display
MainMenu *
mainmenu_init( Flashlight *f, fonts_t *fonts, int test_mode )
{
    if( mm != NULL ) return mm; // There can only be one mainmenu

    mm = malloc( sizeof(MainMenu) );
    if( mm == NULL ) {
    }
    memset( mm, 0, sizeof(MainMenu) );

    mm->f = f;

    mm->poweroff_mode = QUIT_SHUTDOWN;

    // The order of initialisation governs the render order (event order)
    flashlight_event_subscribe( f, (void*)mm, mainmenu_event, FLEventMaskAll );

    mm->gameshot   = gameshot_init   ( f );
    mm->summary    = gamesummary_init( f, fonts );
    mm->decoration = decoration_init ( f );
    mm->gamelist   = gamelist_init   ( f );
    mm->toolbar    = toolbar_init    ( f, fonts );

    mm->c64        = c64_manager_init( f, fonts );

    mm->selector   = selector_init   ( f );

    // Fader is last as it overlays everything
    mm->fader      = fader_init      ( f, fonts );

    focus_to_gamelist( mm );

    fader_set_level( mm->fader, FADE_BLACK, 0 ); // Force initial black level
    fader_set_level( mm->fader, FADE_OFF, 15 ); // Fade up to transparent in 20 steps

    mm->state = MM_Wait_For_Start_Fade;

    return mm;
}

// ----------------------------------------------------------------------------------

void
mainmenu_finish( MainMenu **m ) {
    free(*m);
    *m = mm = NULL;
}

// ----------------------------------------------------------------------------------
int
mainmenu_process( MainMenu *g )
{
    return 1;
}

// ----------------------------------------------------------------------------------
//
static void
focus_to_gamelist( MainMenu *m )
{
    static int have_had_focus = 0;
    if( !mainmenu_selector_owned_by( m->toolbar->menu ) && 
        !mainmenu_selector_owned_by( NULL ) &&
        !mainmenu_selector_owned_by( m->c64 ) 
        ) return;


    if( have_had_focus ) ui_sound_play_flip(); // if() stops sound playing on startup focus

    TSFFocusChange e = { {FocusChange, (void*)m}, m->gamelist->carousel };
    flashlight_event_publish( m->f, (FLEvent*)&e );

    have_had_focus = 1;
}

// ----------------------------------------------------------------------------------
//
static void
focus_to_toolbar( MainMenu *m )
{
    if( !mainmenu_selector_owned_by( m->gamelist->carousel ) ) return;

    ui_sound_play_flip();

    TSFFocusChange e = { {FocusChange, (void*)m}, m->toolbar };
    flashlight_event_publish( m->f, (FLEvent*)&e );
}

// ----------------------------------------------------------------------------------
//
static void
toggle_music( MainMenu *m )
{
    if( !mainmenu_selector_owned_by( m->gamelist->carousel ) &&
        !mainmenu_selector_owned_by( m->toolbar->menu ) ) return;

    m->music_mute = (m->music_mute + 1) % 2;

    if( m->music_mute ) ui_sound_stop_music();
    else                ui_sound_play_music();

    if( m->music_mute ) decorate_music_disable( mm->decoration );
    else                decorate_music_enable ( mm->decoration );
}

// ----------------------------------------------------------------------------------
static void
mainmenu_event( void *self, FLEvent *event )
{
    MainMenu *m = (MainMenu *)self;

    switch( event->type ) {
        case Navigate:
            {
                if( m->emulator_running ) break;

                if( mm->state != MM_State_None ) break;

                if( selector_moving( mm->selector ) ) break;

                //if( !m->emulator_running ) 
                {
                    int direction = ((TSFEventNavigate*)event)->direction;
                    jstype_t type      = ((TSFEventNavigate*)event)->type;
                    if( type == JOY_JOYSTICK ) {
                        if( direction && !(direction & _INPUT_DIR_X )) {
                            if(      direction & INPUT_UP )                       focus_to_gamelist( m );
                            else if( direction & (INPUT_START & ~_INPUT_BUTTON) ) focus_to_gamelist( m );
                            else if( direction & INPUT_DOWN)                      focus_to_toolbar ( m ); 
                            else if( direction & INPUT_GUIDE & ~_INPUT_BUTTON)    toggle_music( m );
                        }
                    }
                    if( type == JOY_KEYBOARD ) { }
                }
                break;
            }
        case Quit:
            {
                TSFEventQuit *qe = (TSFEventQuit*)event;
                // Display shutdown message
                fader_set_level( mm->fader, FADE_BLACK, 1 );

                if( qe->type == QUIT_REBOOT ) fader_reboot_message  ( mm->fader );
                else                          fader_shutdown_message( mm->fader );

                mm->poweroff_mode = (qe->type == QUIT_REBOOT) ? QUIT_REBOOT : QUIT_SHUTDOWN;

                // Signal that we're wating for the fade to complete for the quit message
                mm->state = MM_Wait_For_Quit_Message;
            }
            break;

        case UserEvent:
            {
                TSFUserEvent *ue = (TSFUserEvent*)event;
                switch(ue->user_type) {
                    case EmulatorStarting:
                        m->emulator_running = 1;
                        break;

                    case EmulatorFinishing:
                        glClearColor(122.0f/256, 120.0f/256, 248.0f/256, 1.0); // Default screen colour

                        m->emulator_running = 0;

                        selector_set_lowres( m->selector );

                        fader_set_level( mm->fader, FADE_BLACK,  0 ); // Force initial black level XXX Probably won't need this XXX
                        fader_set_level( mm->fader, FADE_OFF,   6 ); 

                        mm->state = MM_Wait_For_Start_Fade;

                        break;

                    case FadeComplete:
                        if( mm->state == MM_Wait_For_Start_Fade ) {

                            if( !m->music_mute ) ui_sound_play_music();
                            mm->state = MM_State_None;
                        }
                        else if( mm->state == MM_Wait_For_Fade_Out ) {

                            // XXX Start the C64 XXX
                            TSFFocusChange e = { {FocusChange, (void*)m}, m->c64 };
                            flashlight_event_publish( m->f, (FLEvent*)&e );

                            mm->state = MM_State_None;
                        }
                        else if( mm->state == MM_Wait_For_Quit_Message ) {

                            TSFEventPowerOff e = { {PowerOff, (void*)m}, mm->poweroff_mode };
                            flashlight_event_publish( m->f, (FLEvent*)&e );
                        }
                        else mm->state = MM_State_None;

                        break;

                    case EmulatorExit:
                        {
                            // Change focus to back to game menu
                            focus_to_gamelist( m );
                            m->emulator_running = 0;

                            usleep(250000);
                        }
                        break;

                    case AutoloadComplete:
                        {
                            fader_set_level( mm->fader, FADE_BLACK, 0 );
                            fader_set_level( mm->fader, FADE_OFF,   1 );
            
                            flashlight_sound_volume(99);
                            usleep(250000); // Short pause to avoid missing too much of the sound start.
                        }
                        break;

                    case GameSelected:
                        {
                            mm->state = MM_Wait_For_Fade_Out;

                            fader_set_level( mm->fader, FADE_BLACK, 10 );

                            SelectedGame *selected = (SelectedGame *)ue->data;
                            game_t *game = selected->game;

                            mm->c64->game = game;
                            mm->c64->controller_primary = selected->primary_controller_id;

                            ui_sound_wait_for_ding();
                            
                            if( !m->music_mute ) ui_sound_stop_music();
                            flashlight_sound_volume(0);

                            // Steal the focus while we wait for the fade out to complete
                            // to prevent the carousel from getting any more inputs.
                            // When fade out is complete, focus is given to the emulator.
                            TSFFocusChange e = { {FocusChange, (void*)m}, m };
                            flashlight_event_publish( m->f, (FLEvent*)&e );
                        }
                        break;

                    default:
                        break;
                }
            }
            break;
        default:
            break;
    }
}

// ----------------------------------------------------------------------------------
//
void
mainmenu_selector_set_target_position( int ox, int oy, int sx, int sy, uint32_t move_rate )
{
    if( mm->selector == NULL ) return;
    selector_set_target_position( mm->selector, ox, oy, sx, sy, move_rate );
}

// ----------------------------------------------------------------------------------
void
mainmenu_selector_set_position( int ox, int oy, int sx, int sy )
{
    if( mm->selector == NULL ) return;
    selector_set_position( mm->selector, ox, oy, sx, sy );
}

// ----------------------------------------------------------------------------------

void
mainmenu_selector_reset_to_target()
{
    if( mm->selector == NULL ) return;
    selector_reset_to_target( mm->selector );
}

// ----------------------------------------------------------------------------------
//
int
xmainmenu_selector_enable()
{
    if( !mm->selector) return 0;
    return selector_enable( mm->selector );
}

// ----------------------------------------------------------------------------------
//
int
xmainmenu_selector_disable()
{
    if( !mm->selector) return 0;
    return selector_disable( mm->selector );
}

// ----------------------------------------------------------------------------------
//
void
mainmenu_selector_set_hires()
{
    if( !mm->selector) return;
     selector_set_hires( mm->selector );
}

// ----------------------------------------------------------------------------------
//
void
mainmenu_selector_set_lowres()
{
    if( !mm->selector) return;
    selector_set_lowres( mm->selector );
}

// ----------------------------------------------------------------------------------
int
mainmenu_selector_push( void *owner )
{
    if( !mm->selector) return 0;
    return selector_stack_push( mm->selector, owner );
}

// ----------------------------------------------------------------------------------
int
mainmenu_selector_pop( void *owner )
{
    if( !mm->selector) return 0;

    return selector_stack_pop( mm->selector, owner );
}

int
mainmenu_selector_owned_by( void *owner )
{
    if( !mm->selector) return 0;
    return selector_owned_by( mm->selector, owner );
}

int
mainmenu_selector_owned_by_and_ready( void *owner )
{
    if( !mm->selector) return 0;
    return selector_owned_by_and_ready( mm->selector, owner );
}

int
mainmenu_selector_moving()
{
    return selector_moving( mm->selector );
}

// end

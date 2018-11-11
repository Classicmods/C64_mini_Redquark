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
#include "uniforms.h"

// -----------------------------------------------------------------------------------------
// Returns the previous blend state
int
uni_blending( Uniforms *u, int state )
{
    if( u->blending == !!state ) return state;
    u->blending = state;
    return !state;
}

// -----------------------------------------------------------------------------------------
// Returns the previous blend state
float
uni_set_fade( Uniforms *u, float fade )
{
    if( u->fade == fade ) return fade;

    float ret = u->fade;
    u->fade   = fade;

    glUniform1f( u->texture_fade, u->fade );

    return ret;
}

// -----------------------------------------------------------------------------------------

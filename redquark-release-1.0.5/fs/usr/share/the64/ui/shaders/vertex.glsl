#version 100 

precision highp float;

uniform mat4 u_rotation_matrix;
uniform mat4 u_translation_matrix;
uniform mat4 u_projection_matrix;

attribute vec2 a_tex_position;
attribute vec2 a_position;
attribute float a_flag;

varying   vec2  v_tex_position;
varying   float v_tunit;
varying   float v_color;

void main()
{
    v_tunit = fract(a_flag);
    v_tex_position = a_tex_position;

    gl_Position = vec4(a_position, 0.0, 1.0);
    gl_Position  = gl_Position * u_translation_matrix;
}

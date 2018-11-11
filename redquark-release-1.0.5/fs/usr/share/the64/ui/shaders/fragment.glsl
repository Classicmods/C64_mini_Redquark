#version 100

#extension GL_OES_standard_derivatives : enable

precision mediump float;

uniform sampler2D u_CarouselUnit;
uniform sampler2D u_DecorationUnit;
uniform sampler2D u_GameshotUnit;
uniform sampler2D u_TextPaneUnit;
uniform sampler2D u_EmulatorUnit;
uniform sampler2D u_SuspendUnit;

uniform int       u_grey;
uniform float     u_fade;

varying vec2      v_tex_position;
varying float     v_tunit;
varying float     v_color;

#define EMULATOR_FRAGMENT_CRT 0.8
#define EMULATOR_FRAGMENT   0.7
#define TEXT_FRAGMENT_WHITE 0.6
#define SUSPEND_FRAGMENT    0.5
#define TEXT_FRAGMENT       0.4
#define CAROUSEL_FRAGMENT   0.3
#define DECORATION_FRAGMENT 0.2
#define GAMESHOT_FRAGMENT   0.1
//#define CRT_MODE

vec4 scanlinefilter();

void main()
{
	 vec4 color;

     if( v_tunit == EMULATOR_FRAGMENT ) {
        
         color = texture2D(u_EmulatorUnit, v_tex_position);

     } else if( v_tunit == EMULATOR_FRAGMENT_CRT ) {

         float light_row = step(1.0,mod(gl_FragCoord.y, 3.0));
         color = texture2D(u_EmulatorUnit, v_tex_position);
         color = color * (((1.0 - light_row) * 0.8) + light_row);

     } else if( v_tunit == TEXT_FRAGMENT || v_tunit == TEXT_FRAGMENT_WHITE ) {
        vec3 gcolor;
        if( v_tunit == TEXT_FRAGMENT_WHITE ) {
            gcolor = vec3(0.88,0.88,0.88);
        } else {
            gcolor = vec3(0.0,0.0,0.0);
        }

        // Distance Field font rendering
        float dist  = texture2D(u_TextPaneUnit, v_tex_position).r;
        float width = fwidth(dist);
        float alpha = smoothstep(0.50-width, 0.50+width, dist);
        color = vec4(gcolor, alpha * u_fade);

     } else if( v_tunit == CAROUSEL_FRAGMENT ) {
        color = texture2D(u_CarouselUnit, v_tex_position);

     } else if( v_tunit == DECORATION_FRAGMENT ) {
        color = texture2D(u_DecorationUnit, v_tex_position);
        color.a = color.a * u_fade;

     } else if( v_tunit == GAMESHOT_FRAGMENT ) {
        color = texture2D(u_GameshotUnit, v_tex_position);

        // Depending on fade level, slide between full colour and faded grey.
        if( u_grey != 0 ) {
            float gr = (color.r * 0.3 + color.g * 0.59 + color.b * 0.11) * 0.4;
            color.r = color.r * u_fade + gr * (1.0 - u_fade);
            color.g = color.g * u_fade + gr * (1.0 - u_fade);
            color.b = color.b * u_fade + gr * (1.0 - u_fade);
        } else {
            color.a = u_fade;
        }

     } else if( v_tunit == SUSPEND_FRAGMENT ) {
        color = texture2D(u_SuspendUnit, v_tex_position);

     } else {
        //color = vec4(0.40,0.40,0.40,1.0);
     }

     gl_FragColor = color;
}

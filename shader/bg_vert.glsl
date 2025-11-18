#version 100

attribute vec4 a_pos;
attribute vec2 a_uv;

varying vec2 v_uv; // passthrough to bg_frag

void main(void) {
    gl_Position = a_pos;
    v_uv = a_uv; // apply parallax effect to texture
}

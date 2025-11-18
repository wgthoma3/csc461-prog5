#version 100
precision mediump float;

uniform sampler2D s_texture;
uniform vec2 u_offset;
varying vec2 v_uv;

void main(void) {
    vec2 shifted = v_uv + u_offset;
    gl_FragColor = texture2D(s_texture, shifted);
}

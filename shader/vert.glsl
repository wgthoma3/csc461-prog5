
#version 100

precision mediump float; // for some reason my compiler complains about not having this in the vertex shader

// Get per-vertex //
attribute vec3 a_pos; // get position from buffer
attribute vec3 a_norm; // get normal from buffer
attribute vec2 a_uv; // get uv coordinate
// View Matrices //
uniform mat4 u_mvp; // model-view-projection matrix of camera
uniform mat3 u_normal; // view normal
uniform mat4 u_model_view; // projection matrix
// Passed to frag shader //
varying vec3 v_view_pos; // position of vertex in view space
varying vec3 v_norm; // normal of vertex in view space
varying vec2 v_uv; // texture uv coordinate

// Infitite plane
uniform bool u_plane_toggle;
uniform vec3 u_eye_pos; // camera origin in world coords
uniform mat4 u_ivp; // inverse of view-projection matrix
varying vec3 v_plane_ray; // direction from camera to plane in world-space

void main(void) {
    if(u_plane_toggle) { // Drawing the plane quad
        vec4 clip = vec4(a_pos.xy, 0.0, 1.0); // plane quad corner
        gl_Position = clip;
        vec4 world = u_ivp * clip; // corner in world-space
        v_plane_ray = normalize(world.xyz / world.w - u_eye_pos);
        // all other values can be ignored
        // TODO is it more efficient to use existing ones?
        v_view_pos = vec3(0.0);
        v_norm = vec3(0.0);
    } else { // This is a normal object
        // convert object space -> view space -> screen space
        vec4 pos = vec4(a_pos, 1.0);
        gl_Position = u_mvp * pos;
        v_view_pos = (u_model_view * pos).xyz;
        v_norm = normalize(u_normal * a_norm);
    }
    v_uv = a_uv; // plane and objects both can be textured
}

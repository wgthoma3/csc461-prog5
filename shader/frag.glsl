#version 100

#define MAX_LIGHT 16
#define FLOAT_THRESH 0.0001

#extension GL_EXT_frag_depth : require // lets us set frag_depth manually

precision mediump float; // "Satisfies the minimum requirements above for the fragment language" - GLSL manual

uniform vec3 u_eye_pos; // camera origin (used for inf plane)
uniform vec3 u_plane_point;
uniform vec3 u_plane_normal;
uniform bool u_plane_toggle;
// -- View --
uniform mat4 u_model_view; // projection matrix
uniform mat4 u_mvp; // model-view-projection matrix of camera
// -- LIGHTS --
uniform int u_light_cnt; // local light count
uniform vec3 u_light_pos[MAX_LIGHT]; // light positions
uniform vec3 u_light_view_pos[MAX_LIGHT]; // view-space positions
// light materials
uniform vec3 u_light_amb; // global ambient component
uniform vec3 u_light_diff[MAX_LIGHT]; // light diffuse components
uniform vec3 u_light_spec[MAX_LIGHT]; // light specular components
uniform bool u_light_state[MAX_LIGHT]; // whether light is on or off
// -- OBJECT --
// uniform mat4 u_view;
uniform vec3 u_mat_amb; // ambience component
uniform vec3 u_mat_dif; // diffuse component
uniform vec3 u_mat_spec; // specular component
uniform float u_mat_n; // shinniness factor
uniform float u_mat_alpha; // transparency
uniform sampler2D s_texture; // object texture
// -- FROM VERT SHADER --
varying vec3 v_view_pos; // vertex position in view space
varying vec3 v_norm; // interpolated normal in view space
varying vec2 v_uv; // texture uv coordinate
varying vec3 v_plane_ray; // direction from camera to plane in world-space (inf plane)

void main(void) {
    vec3 N;
    vec2 uv = v_uv;
    vec3 view_pos;

    if(u_plane_toggle) { // -- Infinite plane
        // Raycast Problem (from prog1) -> t = ((p1 - p0) . N) / (L . N)
        // t = (plane_pos - cam_pos) . plane_norm ) / (direction . plane_normal)  //
        float LN = dot(v_plane_ray, u_plane_normal); // check plane intersection
        if(abs(LN) < FLOAT_THRESH || LN > 0.0) { // near zero -> parallel, negative -> facing away
            discard; // ignore this plane fragment
        }
        float t = dot(u_plane_point - u_eye_pos, u_plane_normal);
        if(t < 0.0) { // negative -> behind camera
            discard;
        }
        vec3 world = u_eye_pos + t * v_plane_ray; // world position of intersection
        N = u_plane_normal; // use plane normal
        vec4 view = u_model_view * vec4(world, 1.0); // convert to view-space
        view_pos = view.xyz;
        // TODO Test values for better results
        // uv = world.xz * 0.1; // use world pos as UV coordinate (with some scaling)
        vec4 clip = u_mvp * vec4(world, 1.0); // convert to clip space
        gl_FragDepthEXT = clip.z / clip.w; // use perspective divide to interpolate depth
        
    } else { // -- Normal object
        N = normalize(v_norm); // interpolated normal will likely need to be normalized again
        // camera is (0,0,0) in view space
        // so direction vector is the inverse of fragment position
        view_pos = v_view_pos;
        // uv = v_uv;
    }
    vec3 V = normalize(-v_view_pos);
    // Flip normal to light back-faces
    if(!gl_FrontFacing) {
        N = -N;
    }
    // // Global Ambience (this was missing from prog3)
    vec3 acolor = u_mat_amb * u_light_amb;
    vec3 lcolor = vec3(0.0);
    
    // Blinn-Phong
    // consider every light in scene
    for(int i = 0; i < u_light_cnt; ++i) {
        if(u_light_state[i]) { // only process light if active
            vec3 pt2light = u_light_view_pos[i] - view_pos; // vector from pt to light
            float distance = length(pt2light); // get distance from light
            // light distance based on attenuation
            float att = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
            if(att > 0.001) { // process light if within range
                vec3 L = normalize(pt2light); // light direction
                vec3 H = normalize(L + V); // calculate halfway vector
                float d_term = max(0.0, dot(N, L)); // diffuse intensity from light
                float s_term = max(0.0, dot(N, H)); // specular intensity from light
                s_term = pow(s_term, u_mat_n); // use greatest specular factor
                // calculate color due to light
                vec3 diffuse = u_mat_dif * u_light_diff[i] * d_term; // diffuse color
                vec3 specular = u_mat_spec * u_light_spec[i] * s_term; // phong specular color
                lcolor += (diffuse + specular) * att;
            }
        }
    }

    vec4 tcolor = texture2D(s_texture, uv); // get texture color
    vec3 color = acolor + (tcolor.rgb * lcolor); // calculate final color
    color = clamp(color, 0.0, 1.0); // clamp to 0-255
    float alpha = tcolor.a * u_mat_alpha;

    gl_FragColor = vec4(color, alpha); // for now, assign alpha directly
}

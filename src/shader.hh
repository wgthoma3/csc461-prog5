#pragma once

// System libs
#include <GLES2/gl2.h>
// Local libs
// Local includes
#include "defs.hh"

// Shader Uniforms
extern GLint   u_ambient,
        u_diffuse,
        u_specular,
        u_exponent,
        u_alpha,
        u_normal, // view normal
        // u_proj, // view projection matrix
        // u_view,
        u_model_view, // model transformation matrix
        u_mvp; // model-view projection matrix
        // Infinite Plane
extern GLint u_ivp, // inverse projection-view matrix
        u_plane_toggle, // toggle floor drawing in shader
        u_plane_point, // any point on the plane, used as an anchor
        u_plane_normal, // normal of the plane
        u_eye_pos; // camera position in world coordinates
        // Lights
extern GLint u_light_cnt, // total number of lights loaded
        u_light_pos[MAX_LIGHTS],
        u_light_view_pos[MAX_LIGHTS], // light position in view space
        u_light_amb,
        u_light_diff[MAX_LIGHTS],
        u_light_spec[MAX_LIGHTS],
        u_light_state[MAX_LIGHTS]; // whether light is on/off
extern GLint u_bg_offset; // used for parallax effect
// Texture Uniforms
extern GLint   s_background; // background texture
extern GLint   s_object; // object texture
// Vertex Attributes
extern GLint   a_pos, // vertex position
        a_norm, // vertex normal
        a_uv; // uv coordinate
// BG Attributes //
extern GLint   a_bg_pos, // bg position attr
        a_bg_uv; // bg uv coord attr

#define USE_BG_SHADER \
{ \
    glDisableVertexAttribArray(a_pos); \
    glDisableVertexAttribArray(a_norm); \
    glDisableVertexAttribArray(a_uv); \
\
    glUseProgram(bg_shader); \
    glEnableVertexAttribArray(a_pos); \
    glEnableVertexAttribArray(a_norm); \
    glBindBuffer(GL_ARRAY_BUFFER, bg_vbo); \
    glVertexAttribPointer(a_bg_pos, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0); \
    glVertexAttribPointer(a_bg_uv, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float) * 3)); \
    glDisable(GL_DEPTH_TEST); \
}

// DOES NOT BIND BUFFERS
#define USE_OBJ_SHADER \
{ \
    glDisableVertexAttribArray(a_bg_pos); \
    glDisableVertexAttribArray(a_bg_uv); \
\
    glUseProgram(object_shader); \
    glEnableVertexAttribArray(a_pos); \
    glEnableVertexAttribArray(a_norm); \
    glEnableVertexAttribArray(a_uv); \
    glEnable(GL_DEPTH_TEST); \
    glEnable(GL_BLEND); \
}

void print_gl_status(GLuint id);

GLuint compile_shader(GLenum type, const char* src);

#define get_src_from_file(FILE, SRC_PTR) \
    if((FILE).is_open()) { \
        size_t fsize; \
        (FILE).seekg(0, std::ios::end); \
        fsize = static_cast<size_t>((FILE).tellg()); \
        (FILE).seekg(0, std::ios::beg); \
        (SRC_PTR) = static_cast<char*>(malloc(fsize + 1)); \
        (FILE).read((SRC_PTR), fsize); \
        (SRC_PTR)[fsize] = '\0'; \
        (FILE).close(); \
    }

GLint create_shader_program(const char* vert_src, const char* frag_src);


